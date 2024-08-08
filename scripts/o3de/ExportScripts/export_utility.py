#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


import o3de.command_utils as command_utils
import o3de.export_project as exp
import o3de.manifest as manifest

from o3de import android, android_support


import argparse
import logging
import pathlib
import sys

from typing import List

def process_level_names(ctx, seedfile_paths, level_names, platform):
    seedfile_set = set(seedfile_paths)
    for level in level_names:
        new_level_path = ctx.project_path / f'Cache/{platform}/levels' / level.lower() / (level.lower() + ".spawnable")
        if new_level_path.is_file():
            seedfile_set.add(new_level_path)
    
    seedfile_paths.clear()
    seedfile_paths.extend([file for  file in seedfile_set])

  
def compute_tools_build_path(ctx, is_installer_sdk, selected_platform, tool_config, tools_build_path, default_base_path, logger=None):
    # Note, when resolving the relative paths used for the export process, there is a debate about 
    # whether to use the current working directory, or to forcefully set the base path to an O3DE directory.
    # In practice, there was noticeable surprise whenever the relative folder wasn't found in an O3DE directory,
    # and sometimes commands are carelessly run from sensitive directories (such as C:\\Windows\\System32)
    
    # For this reason, all deliberately external paths must be specified using absolute paths
    if is_installer_sdk:    
        tools_build_path = ctx.engine_path / 'bin' / exp.get_platform_installer_folder_name(selected_platform) / tool_config / 'Default'
    
    if not tools_build_path:
        tools_build_path = default_base_path / 'build/tools'
    elif not tools_build_path.is_absolute():
        tools_build_path = default_base_path / tools_build_path
    
    if logger:
        logger.info(f"Tools build path set to {tools_build_path}")
    
    return tools_build_path


def handle_tools(ctx: exp.O3DEScriptExportContext,
                 should_build_tools: bool,
                 is_installer_sdk: bool,
                 tool_config: str,
                 selected_platform: str,
                 tools_build_path: pathlib.Path,
                 default_base_path: pathlib.Path,
                 engine_centric: bool,
                 logger: logging.Logger|None ):
    tools_build_path = compute_tools_build_path(ctx, is_installer_sdk, selected_platform, tool_config, tools_build_path, default_base_path)

    if should_build_tools and not is_installer_sdk:
        exp.build_export_toolchain(ctx=ctx,
                                   tools_build_path=tools_build_path,
                                   engine_centric=engine_centric,
                                   tool_config=tool_config,
                                   logger=logger)
    return tools_build_path


def build_and_bundle_assets(ctx: exp.O3DEScriptExportContext,
                            should_build_all_assets: bool,
                            tools_build_path: pathlib.Path,
                            is_installer_sdk: bool,
                            tool_config: str,
                            engine_centric: bool,
                            fail_on_asset_errors: bool,
                            asset_platforms: List[str],
                            seedlist_paths: List[pathlib.Path],
                            seedfile_paths: List[pathlib.Path],
                            asset_bundling_path: pathlib.Path,
                            max_bundle_size: int,
                            logger: logging.Logger|None =None):
    # Optionally build the assets
    if should_build_all_assets:
        asset_processor_path = exp.get_asset_processor_batch_path(tools_build_path=tools_build_path,
                                                                  using_installer_sdk=is_installer_sdk,
                                                                  tool_config=tool_config,
                                                                  required=True)
        if logger:
            logger.info(f"Using '{asset_processor_path}' to process the assets.")
        exp.build_assets(ctx=ctx,
                         tools_build_path=tools_build_path,
                         engine_centric=engine_centric,
                         fail_on_ap_errors=fail_on_asset_errors,
                         using_installer_sdk=is_installer_sdk,
                         tool_config=tool_config,
                         selected_platforms=asset_platforms,
                         logger=logger)
    
    expected_bundles_path = exp.bundle_assets(ctx=ctx,
                                              selected_platforms=asset_platforms,
                                              seedlist_paths=seedlist_paths,
                                              seedfile_paths=seedfile_paths,
                                              tools_build_path=tools_build_path,
                                              engine_centric=engine_centric,
                                              asset_bundling_path=asset_bundling_path,
                                              using_installer_sdk=is_installer_sdk,
                                              tool_config=tool_config,
                                              max_bundle_size=max_bundle_size)
    return expected_bundles_path


def create_common_export_params_and_config(parser, export_config):
    parser.add_argument(exp.CUSTOM_SCRIPT_HELP_ARGUMENT,default=False,action='store_true',help='Show this help message and exit.')
    
    default_project_build_config = export_config.get_value(key=exp.SETTINGS_PROJECT_BUILD_CONFIG.key,
                                                            default=exp.SETTINGS_PROJECT_BUILD_CONFIG.default)
    parser.add_argument('-cfg', '--config', type=str, default=default_project_build_config, choices=[exp.BUILD_CONFIG_RELEASE, exp.BUILD_CONFIG_PROFILE, exp.BUILD_CONFIG_DEBUG],
                        help='The CMake build configuration to use when building project binaries.')
    
    default_tool_build_config = export_config.get_value(key=exp.SETTINGS_TOOL_BUILD_CONFIG.key,
                                                            default=exp.SETTINGS_TOOL_BUILD_CONFIG.default)
    parser.add_argument('-tcfg', '--tool-config', type=str, default=default_tool_build_config, choices=[exp.BUILD_CONFIG_RELEASE, exp.BUILD_CONFIG_PROFILE, exp.BUILD_CONFIG_DEBUG],
                        help='The CMake build configuration to use when building tool binaries.')
    
    default_tools_build_path = export_config.get_value(exp.SETTINGS_DEFAULT_BUILD_TOOLS_PATH.key, exp.SETTINGS_DEFAULT_BUILD_TOOLS_PATH.default)
    parser.add_argument('-tbp', '--tools-build-path', type=pathlib.Path, default=pathlib.Path(default_tools_build_path),
                        help=f"Designates where the build files for the O3DE toolchain are generated. If not specified, default is '{default_tools_build_path}'.")
    
    default_max_size = export_config.get_value(exp.SETTINGS_MAX_BUNDLE_SIZE.key, exp.SETTINGS_MAX_BUNDLE_SIZE.default)
    parser.add_argument('-maxsize', '--max-bundle-size', type=int, default=int(default_max_size),
                        help=f"Specify the maximum size of a given asset bundle in MB.")
    
    export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_TOOLS.key,
                                           enable_override_arg=['-bt', '--build-tools'],
                                           enable_override_desc="Enable the building of the O3DE toolchain executables (AssetBundlerBatch, AssetProcessorBatch) as part of the export process. "
                                                                "The tools will be built into the location specified by the '--tools-build-path' argument",
                                           disable_override_arg=['-sbt', '--skip-build-tools'],
                                           disable_override_desc="Skip the building of the O3DE toolchain executables (AssetBundlerBatch, AssetProcessorBatch) as part of the export process. "
                                                                 "The tools must be already available at the location specified by the '--tools-build-path' argument.")

    export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_FAIL_ON_ASSET_ERR.key,
                                           enable_override_arg=['-foa', '--fail-on-asset-errors'],
                                           enable_override_desc='Fail the export if there are errors during the building of assets. (Only relevant if assets are set to be built).',
                                           disable_override_arg=['-coa', '--continue-on-asset-errors'],
                                           disable_override_desc='Continue export even if there are errors during the building of assets. (Only relevant if assets are set to be built).')
    
    export_config.add_multi_part_argument(argument=['-sl', '--seedlist'],
                                              parser=parser,
                                              key=exp.SETTINGS_SEED_LIST_PATHS.key,
                                              dest='seedlist_paths',
                                              description='Path to a seed list file for asset bundling. Specify multiple times for each seed list. You can also specify wildcard patterns.',
                                              is_path_type=True)
    
    export_config.add_multi_part_argument(argument=['-sf', '--seedfile'],
                                            parser=parser,
                                            key=exp.SETTINGS_SEED_FILE_PATHS.key,
                                            dest='seedfile_paths',
                                            description='Path to a seed file for asset bundling. Example seed files are levels or prefabs. You can also specify wildcard patterns.',
                                            is_path_type=True)
    
    export_config.add_multi_part_argument(argument=['-lvl', '--level-name'],
                                            parser=parser,
                                            key=exp.SETTINGS_DEFAULT_LEVEL_NAMES.key,
                                            dest='level_names',
                                            description='The name of the level you want to export. This will look in <o3de_project_path>/Levels to fetch the right level prefab.',
                                            is_path_type=False)
    
    export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_ENGINE_CENTRIC.key,
                                           enable_override_arg=['-ec', '--engine-centric'],
                                           enable_override_desc="Enable the engine-centric work flow to export the project.",
                                           disable_override_arg=['-pc', '--project-centric'],
                                           disable_override_desc="Enable the project-centric work flow to export the project.")
    
    export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_ASSETS.key,
                                           enable_override_arg=['-assets', '--build-assets'],
                                           enable_override_desc='Build and update all assets before bundling.',
                                           disable_override_arg=['-noassets', '--skip-build-assets'],
                                           disable_override_desc='Skip building of assets and use assets that were already built.')
    parser.add_argument('-q', '--quiet', action='store_true', help='Suppresses logging information unless an error occurs.')
    