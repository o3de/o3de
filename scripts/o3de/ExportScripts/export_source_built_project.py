#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import glob
import logging
import os
import sys

import o3de.export_project as exp
import o3de.manifest as manifest
import o3de.command_utils as command_utils
import pathlib

from typing import List


def export_standalone_project(ctx: exp.O3DEScriptExportContext,
                              selected_platform: str,
                              output_path: pathlib.Path,
                              should_build_tools: bool,
                              build_config: str,
                              tool_config: str,
                              seedlist_paths: List[pathlib.Path],
                              seedfile_paths: List[pathlib.Path],
                              level_names: List[str],
                              game_project_file_patterns_to_copy: List[str] = [],
                              server_project_file_patterns_to_copy: List[str] = [],
                              project_file_patterns_to_copy: List[str] = [],
                              asset_bundling_path: pathlib.Path|None = None,
                              max_bundle_size: int = 2048,
                              should_build_all_assets: bool = True,
                              should_build_game_launcher: bool = True,
                              should_build_server_launcher: bool = True,
                              should_build_unified_launcher: bool = True,
                              monolithic_build: bool = True,
                              allow_registry_overrides: bool = False,
                              tools_build_path: pathlib.Path | None =None,
                              launcher_build_path: pathlib.Path | None =None,
                              archive_output_format: str = "none",
                              fail_on_asset_errors: bool = False,
                              engine_centric: bool = False,
                              logger: logging.Logger|None = None) -> None:
    """
    This function serves as the generic, general workflow for project exports. The steps in this code will generate
    export packages based on all the configurable options that is supported by the project export command. This function
    will serve as a general blueprint to create any custom export scripts for specific projects.

    :param ctx:                                     The O3DE Script context provided by the export-command
    :param selected_platform:                       The asset platform to use for the packaging of assets.
    :param output_path:                             The base output path of the exported package(s)
    :param should_build_tools:                      Option to build the export process dependent tools (AssetProcessor, AssetBundlerBatch, and dependencies)
    :param build_config:                            The build configuration to use for the export package launchers
    :param tool_config:                             The build configuration to refer to for tool binaries
    :param seedlist_paths:                          List of seed list files to optionally include in the asset bundling process
    :param seedfile_paths:                          List of seed files to optionally include in the asset bundling process. These can be individual O3DE assets, such as levels or prefabs.
    :param level_names:                             List of individual level names. These are assumed to follow standard O3DE level convention, and will be used in the asset bundler.
    :param game_project_file_patterns_to_copy:      List of game (or unified) launcher specific files to include in the game/unified package
    :param server_project_file_patterns_to_copy:    List of server (or unified) launcher specific files to include in the server/unified package
    :param project_file_patterns_to_copy:           List of general file patterns to include in all packages
    :param asset_bundling_path:                     Optional path to write the artifacts from the asset bundling process
    :param max_bundle_size:                         Maximum size to set for the archived bundle files
    :param should_build_all_assets:                 Option to build/process all the assets for the game
    :param should_build_game_launcher:              Option to build the game launcher package
    :param should_build_server_launcher:            Option to build the server launcher package
    :param should_build_unified_launcher:           Option to build the unified launcher package
    :param monolithic_build:                        Option to build the game binaries monolithically
    :param allow_registry_overrides:                Option to allow registry overrides in the build process
    :param tools_build_path:                        Optional build path to build the tools. (Will default to build/tools if not supplied)
    :param launcher_build_path:                     Optional build path to build the game launcher(s). (Will default to build/launcher if not supplied)
    :param archive_output_format:                   Optional archive format to use for archiving the final package(s)
    :param fail_on_asset_errors:                    Option to fail the process on any asset processing error
    :param engine_centric:                          Option to use an engine-centric workflow or not
    :param logger:                                  Optional logger to use to log the process and errors
    """

    is_installer_sdk = manifest.is_sdk_engine(engine_path=ctx.engine_path)

    # Use a provided logger or get the current system one
    if not logger:
        logger = logging.getLogger()
        logger.setLevel(logging.ERROR)

    # Calculate the tools and game build paths
    default_base_build_path = ctx.engine_path / 'build' if engine_centric else ctx.project_path / 'build'
    
    if is_installer_sdk:
            # Check if we have any monolithic entries if building monolithic
            monolithic_artifacts = glob.glob(f'cmake/Platform/{exp.get_platform_installer_folder_name()}/Monolithic/ConfigurationTypes_*.cmake',
                      root_dir=ctx.engine_path)
            has_monolithic_artifacts = len(monolithic_artifacts) > 0

            if monolithic_build and not has_monolithic_artifacts:
                logger.error("Trying to create monolithic build, but no monolithic artifacts are detected in the engine installation! Please re-run the script with '-nomonolithic' and '-config profile'.")
                return 1
            tools_build_path = ctx.engine_path / 'bin' / exp.get_platform_installer_folder_name(selected_platform) / tool_config / 'Default'
    
    if not tools_build_path:
        tools_build_path = default_base_build_path / 'tools'
    if not launcher_build_path or not launcher_build_path.is_absolute() or \
        not (launcher_build_path.is_relative() and pathlib.Path.cwd() in [ctx.engine_path, ctx.project_path]):
        launcher_build_path = default_base_build_path / 'launcher'
    
    if not asset_bundling_path or not asset_bundling_path.is_absolute() or \
        not (asset_bundling_path.is_relative() and pathlib.Path.cwd() in [ctx.engine_path, ctx.project_path]):
        asset_bundling_path = default_base_build_path / 'asset_bundling'

    # Resolve (if possible) and validate any provided seedlist files
    validated_seedslist_paths = exp.validate_project_artifact_paths(project_path=ctx.project_path,
                                                                    artifact_paths=seedlist_paths)
    
    # Convert level names into seed files that the asset bundler can utilize for packaging
    for level in level_names:
        seedfile_paths.append(ctx.project_path / f'Cache/{selected_platform}/levels' / level.lower() / (level.lower() + ".spawnable"))

    # Make sure there are no running processes for the current project before continuing
    exp.kill_existing_processes(ctx.project_name)

    # Optionally build the toolchain needed to bundle the assets
    # Do not run this logic if we're using an installer SDK engine. Tool binaries should already exist
    if should_build_tools and not is_installer_sdk:
        exp.build_export_toolchain(ctx=ctx,
                                   tools_build_path=tools_build_path,
                                   engine_centric=engine_centric,
                                   tool_config=tool_config,
                                   logger=logger)

    # Build the requested game launcher types (if any)
    launcher_type = 0
    if should_build_game_launcher:
        launcher_type |= exp.LauncherType.GAME
    if should_build_server_launcher:
        launcher_type |= exp.LauncherType.SERVER
    if should_build_unified_launcher:
        launcher_type |= exp.LauncherType.UNIFIED

    if launcher_type != 0:
        exp.build_game_targets(ctx=ctx,
                               build_config=build_config,
                               game_build_path=launcher_build_path,
                               engine_centric=engine_centric,
                               launcher_types=launcher_type,
                               allow_registry_overrides=allow_registry_overrides,
                               tool_config=tool_config,
                               monolithic_build=monolithic_build,
                               logger=logger)

    # Optionally build the assets
    if should_build_all_assets:
        asset_processor_path = exp.get_asset_processor_batch_path(tools_build_path=tools_build_path,
                                                                  using_installer_sdk=is_installer_sdk,
                                                                  tool_config=tool_config,
                                                                  required=True)
        logger.info(f"Using '{asset_processor_path}' to process the assets.")
        exp.build_assets(ctx=ctx,
                         tools_build_path=tools_build_path,
                         engine_centric=engine_centric,
                         fail_on_ap_errors=fail_on_asset_errors,
                         using_installer_sdk=is_installer_sdk,
                         tool_config=tool_config,
                         logger=logger)

    # Generate the bundle
    asset_bundler_path = exp.get_asset_bundler_batch_path(tools_build_path=tools_build_path,
                                                          using_installer_sdk=is_installer_sdk,
                                                          tool_config=tool_config,
                                                          required=True)
    logger.info(f"Using '{asset_bundler_path}' to bundle the assets.")
    expected_bundles_path = exp.bundle_assets(ctx=ctx,
                                              selected_platform=selected_platform,
                                              seedlist_paths=validated_seedslist_paths,
                                              seedfile_paths=seedfile_paths,
                                              tools_build_path=tools_build_path,
                                              engine_centric=engine_centric,
                                              asset_bundling_path=asset_bundling_path,
                                              using_installer_sdk=is_installer_sdk,
                                              tool_config=tool_config,
                                              max_bundle_size=max_bundle_size)

    # Prepare the different layouts based on the desired launcher types
    export_layouts = []
    if should_build_game_launcher:
        export_layouts.append(exp.ExportLayoutConfig(output_path=output_path / f'{ctx.project_name}GamePackage',
                                                     project_file_patterns=project_file_patterns_to_copy + game_project_file_patterns_to_copy,
                                                     ignore_file_patterns=[f'*.ServerLauncher{exp.EXECUTABLE_EXTENSION}', f'*.UnifiedLauncher{exp.EXECUTABLE_EXTENSION}']))

    if should_build_server_launcher:
        export_layouts.append(exp.ExportLayoutConfig(output_path=output_path / f'{ctx.project_name}ServerPackage',
                                                     project_file_patterns=project_file_patterns_to_copy + server_project_file_patterns_to_copy,
                                                     ignore_file_patterns=[f'*.GameLauncher{exp.EXECUTABLE_EXTENSION}', f'*.UnifiedLauncher{exp.EXECUTABLE_EXTENSION}']))

    if should_build_unified_launcher:
        export_layouts.append(exp.ExportLayoutConfig(output_path=output_path / f'{ctx.project_name}UnifiedPackage',
                                                     project_file_patterns=project_file_patterns_to_copy + game_project_file_patterns_to_copy + server_project_file_patterns_to_copy,
                                                     ignore_file_patterns=[f'*.ServerLauncher{exp.EXECUTABLE_EXTENSION}', f'*.GameLauncher{exp.EXECUTABLE_EXTENSION}']))

    # Generate the layouts and archive the packages based on the desired launcher types
    for export_layout in export_layouts:
        exp.setup_launcher_layout_directory(project_path=ctx.project_path,
                                            project_name=ctx.project_name,
                                            asset_platform=selected_platform,
                                            launcher_build_path=launcher_build_path,
                                            build_config=build_config,
                                            bundles_to_copy=[expected_bundles_path / f'game_{selected_platform}.pak',
                                                             expected_bundles_path / f'engine_{selected_platform}.pak'],
                                            export_layout=export_layout,
                                            archive_output_format=archive_output_format,
                                            logger=logger)

# This code is only run by the 'export-project' O3DE CLI command
if "o3de_context" in globals():

    global o3de_context
    global o3de_logger

    # Resolve the export config
    try:
        if o3de_context is None:
            project_name, project_path = command_utils.resolve_project_name_and_path()
            export_config = exp.get_export_project_config(project_path=project_name)
        else:
            export_config = exp.get_export_project_config(project_path=o3de_context.project_path)
    except command_utils.O3DEConfigError:
        o3de_logger.debug(f"No project detected at {os.getcwd()}, getting default settings from global config instead.")
        project_name = None
        export_config = exp.get_export_project_config(project_path=None)

    def parse_args(o3de_context: exp.O3DEScriptExportContext):

        parser = argparse.ArgumentParser(
                    prog=f'o3de.py export-project -es {__file__}',
                    description="Exports a project as standalone to the desired output directory with release layout. "
                                "In order to use this script, the engine and project must be setup and registered beforehand. ",
                    epilog=exp.CUSTOM_CMAKE_ARG_HELP_EPILOGUE,
                    formatter_class=argparse.RawTextHelpFormatter,
                    add_help=False
        )
        parser.add_argument(exp.CUSTOM_SCRIPT_HELP_ARGUMENT,default=False,action='store_true',help='Show this help message and exit.')
        parser.add_argument('-out', '--output-path', type=pathlib.Path, required=True, help='Path that describes the final resulting Release Directory path location.')

        default_project_build_config = export_config.get_value(key=exp.SETTINGS_PROJECT_BUILD_CONFIG.key,
                                                               default=exp.SETTINGS_PROJECT_BUILD_CONFIG.default)
        parser.add_argument('-cfg', '--config', type=str, default=default_project_build_config, choices=[exp.BUILD_CONFIG_RELEASE, exp.BUILD_CONFIG_PROFILE],
                            help='The CMake build configuration to use when building project binaries.')

        # tool.build.config / SETTINGS_TOOL_BUILD_CONFIG
        default_tool_build_config = export_config.get_value(key=exp.SETTINGS_TOOL_BUILD_CONFIG.key,
                                                            default=exp.SETTINGS_TOOL_BUILD_CONFIG.default)
        parser.add_argument('-tcfg', '--tool-config', type=str, default=default_tool_build_config, choices=[exp.BUILD_CONFIG_RELEASE, exp.BUILD_CONFIG_PROFILE, exp.BUILD_CONFIG_DEBUG],
                            help='The CMake build configuration to use when building tool binaries.')

        # archive.output.format / SETTINGS_ARCHIVE_OUTPUT_FORMAT
        default_archive_output_format = export_config.get_value(key=exp.SETTINGS_ARCHIVE_OUTPUT_FORMAT.key,
                                                                default=exp.SETTINGS_ARCHIVE_OUTPUT_FORMAT.default)
        parser.add_argument('-a', '--archive-output',  type=str,
                            help="Option to create a compressed archive the output. "
                                 "Specify the format of archive to create from the output directory. If 'none' specified, no archiving will occur.",
                            choices=[exp.ARCHIVE_FORMAT_NONE, exp.ARCHIVE_FORMAT_ZIP, exp.ARCHIVE_FORMAT_GZIP, exp.ARCHIVE_FORMAT_BZ2, exp.ARCHIVE_FORMAT_XZ],
                            default=default_archive_output_format)

        # option.build.assets / SETTINGS_OPTION_BUILD_ASSETS
        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_ASSETS.key,
                                           enable_override_arg=['-assets', '--should-build-assets'],
                                           enable_override_desc='Build and update all assets before bundling.',
                                           disable_override_arg=['-noassets', '--skip-build-assets'],
                                           disable_override_desc='Skip building of assets and use assets that were already built.')

        # option.fail.on.asset.errors / SETTINGS_OPTION_FAIL_ON_ASSET_ERR
        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_FAIL_ON_ASSET_ERR.key,
                                           enable_override_arg=['-foa', '--fail-on-asset-errors'],
                                           enable_override_desc='Fail the export if there are errors during the building of assets. (Only relevant if assets are set to be built).',
                                           disable_override_arg=['-coa', '--continue-on-assets-errors'],
                                           disable_override_desc='Continue export even if there are errors during the building of assets. (Only relevant if assets are set to be built).')

        export_config.add_multi_part_argument(argument=['-sl', '--seedlist'],
                                              parser=parser,
                                              key=exp.SETTINGS_SEED_LIST_PATHS.key,
                                              dest='seedlist_paths',
                                              description='Path to a seed list file for asset bundling. Specify multiple times for each seed list.',
                                              is_path_type=True)

        export_config.add_multi_part_argument(argument=['-sf', '--seedfile'],
                                              parser=parser,
                                              key=exp.SETTINGS_SEED_FILE_PATHS.key,
                                              dest='seedfile_paths',
                                              description='Path to a seed file for asset bundling. Example seed files are levels or prefabs.',
                                              is_path_type=True)

        export_config.add_multi_part_argument(argument=['-lvl', '--level-name'],
                                              parser=parser,
                                              key=exp.SETTINGS_DEFAULT_LEVEL_NAMES.key,
                                              dest='level_names',
                                              description='The name of the level you want to export. This will look in <o3de_project_path>/Levels to fetch the right level prefab.',
                                              is_path_type=False)

        export_config.add_multi_part_argument(argument=['-gpfp', '--game-project-file-pattern-to-copy'],
                                              parser=parser,
                                              key=exp.SETTINGS_ADDITIONAL_GAME_PROJECT_FILE_PATTERN.key,
                                              dest='game_project_file_patterns_to_copy',
                                              description='Any additional file patterns located in the project directory. File patterns will be relative to the project path. Will be included in GameLauncher.',
                                              is_path_type=False)

        export_config.add_multi_part_argument(argument=['-spfp', '--server-project-file-pattern-to-copy'],
                                              parser=parser,
                                              key=exp.SETTINGS_ADDITIONAL_SERVER_PROJECT_FILE_PATTERN.key,
                                              dest='server_project_file_patterns_to_copy',
                                              description='Any additional file patterns located in the project directory. File patterns will be relative to the project path. Will be included in ServerLauncher.',
                                              is_path_type=False)

        export_config.add_multi_part_argument(argument=['-pfp', '--project-file-pattern-to-copy'],
                                              parser=parser,
                                              key=exp.SETTINGS_ADDITIONAL_PROJECT_FILE_PATTERN.key,
                                              dest='project_file_patterns_to_copy',
                                              description='Any additional file patterns located in the project directory. File patterns will be relative to the project path.',
                                              is_path_type=False)

        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_TOOLS.key,
                                           enable_override_arg=['-bt', '--build-tools'],
                                           enable_override_desc="Enable the building of the O3DE toolchain executables (AssetBundlerBatch, AssetProcessorBatch) as part of the export process. "
                                                                "The tools will be built into the location specified by the '--tools-build-path' argument",
                                           disable_override_arg=['-sbt', '--skip-build-tools'],
                                           disable_override_desc="Skip the building of the O3DE toolchain executables (AssetBundlerBatch, AssetProcessorBatch) as part of the export process. "
                                                                 "The tools must be already available at the location specified by the '--tools-build-path' argument.")

        default_build_tools_path = export_config.get_value(exp.SETTINGS_DEFAULT_BUILD_TOOLS_PATH.key, exp.SETTINGS_DEFAULT_BUILD_TOOLS_PATH.default)
        parser.add_argument('-tbp', '--tools-build-path', type=pathlib.Path, default=pathlib.Path(default_build_tools_path),
                            help=f'Designates where the build files for the O3DE toolchain are generated. If not specified, default is {default_build_tools_path}')

        default_launcher_build_path = export_config.get_value(exp.SETTINGS_DEFAULT_LAUNCHER_TOOLS_PATH.key, exp.SETTINGS_DEFAULT_LAUNCHER_TOOLS_PATH.default)
        parser.add_argument('-lbp', '--launcher-build-path', type=pathlib.Path, default=pathlib.Path(default_launcher_build_path),
                            help=f"Designates where the launcher build files (Game/Server/Unified) are generated. If not specified, default is {default_launcher_build_path}.")

        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_ALLOW_REGISTRY_OVERRIDES.key,
                                           enable_override_arg=['-regovr', '--allow-registry-overrides'],
                                           enable_override_desc="Allow overriding registry settings from external sources during the cmake build configuration.",
                                           disable_override_arg=['-noregovr', '--disallow-registry-overrides'],
                                           disable_override_desc="Disallow overriding registry settings from external sources during the cmake build configuration.")

        default_asset_bundling_path = export_config.get_value(exp.SETTINGS_DEFAULT_ASSET_BUNDLING_PATH.key, exp.SETTINGS_DEFAULT_ASSET_BUNDLING_PATH.default)
        parser.add_argument('-abp', '--asset-bundling-path', type=pathlib.Path, default=pathlib.Path(default_asset_bundling_path),
                            help=f"Designates where the artifacts from the asset bundling process will be written to before creation of the package. If not specified, default is {default_asset_bundling_path}.")

        default_max_size = export_config.get_value(exp.SETTINGS_MAX_BUNDLE_SIZE.key, exp.SETTINGS_MAX_BUNDLE_SIZE.default)
        parser.add_argument('-maxsize', '--max-bundle-size', type=int, default=int(default_max_size),
                            help=f"Specify the maximum size of a given asset bundle.. If not specified, default is {default_max_size}.")

        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_GAME_LAUNCHER.key,
                                           enable_override_arg=['-game', '--game-launcher'],
                                           enable_override_desc="Enable the building and inclusion of the Game Launcher.",
                                           disable_override_arg=['--nogame', '--no-game-launcher'],
                                           disable_override_desc="Disable the building and inclusion of the Game Launcher if not needed.")

        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_SERVER_LAUNCHER.key,
                                           enable_override_arg=['-server', '--server-launcher'],
                                           enable_override_desc="Enable the building and inclusion of the Server Launcher.",
                                           disable_override_arg=['--noserver', '--no-server-launcher'],
                                           disable_override_desc="Disable the building and inclusion of the Server Launcher if not needed.")


        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_UNIFIED_SERVER_LAUNCHER.key,
                                           enable_override_arg=['-unified', '--unified-launcher'],
                                           enable_override_desc="Enable the building and inclusion of the Unified Launcher.",
                                           disable_override_arg=['--nounified', '--no-unified-launcher'],
                                           disable_override_desc="Disable the building and inclusion of the Unified Launcher if not needed.")

        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_ENGINE_CENTRIC.key,
                                           enable_override_arg=['-ec', '--engine-centric'],
                                           enable_override_desc="Enable the engine-centric work flow to export the project.",
                                           disable_override_arg=['-pc', '--project-centric'],
                                           disable_override_desc="Enable the project-centric work flow to export the project.")

        parser.add_argument('-nomonolithic', '--no-monolithic-build', action='store_true', help='Build the project binaries as shared libraries (as opposed to default monolithic build).')
        parser.add_argument('-pl', '--platform', type=str, default=exp.get_default_asset_platform(), choices=['pc', 'linux', 'mac'])

        parser.add_argument('-q', '--quiet', action='store_true', help='Suppresses logging information unless an error occurs.')
        if o3de_context is None:
            parser.print_help()
            exit(0)
        
        parsed_args = parser.parse_args(o3de_context.args)
        if parsed_args.script_help:
            parser.print_help()
            exit(0)

        return parsed_args
    
    args = parse_args(o3de_context)

    option_build_tools = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                 key=exp.SETTINGS_OPTION_BUILD_TOOLS.key,
                                                                 enable_attribute='build_tools',
                                                                 disable_attribute='skip_build_tools')

    option_build_assets = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                  key=exp.SETTINGS_OPTION_BUILD_ASSETS.key,
                                                                  enable_attribute='build_assets',
                                                                  disable_attribute='skip_build_assets')

    fail_on_asset_errors = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                   key=exp.SETTINGS_OPTION_FAIL_ON_ASSET_ERR.key,
                                                                   enable_attribute='stop_on_asset_errors',
                                                                   disable_attribute='continue_on_assets_errors')

    option_build_game_launcher = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                         key=exp.SETTINGS_OPTION_BUILD_GAME_LAUNCHER.key,
                                                                         enable_attribute='game_launcher',
                                                                         disable_attribute='no_game_launcher')

    option_build_server_launcher = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                           key=exp.SETTINGS_OPTION_BUILD_SERVER_LAUNCHER.key,
                                                                           enable_attribute='server_launcher',
                                                                           disable_attribute='no_server_launcher')


    option_build_unified_launcher = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                            key=exp.SETTINGS_OPTION_BUILD_UNIFIED_SERVER_LAUNCHER.key,
                                                                            enable_attribute='unified_launcher',
                                                                            disable_attribute='no_unified_launcher')

    option_build_engine_centric = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                          key=exp.SETTINGS_OPTION_ENGINE_CENTRIC.key,
                                                                          enable_attribute='engine_centric',
                                                                          disable_attribute='project_centric')

    option_allow_registry_overrrides = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                               key=exp.SETTINGS_OPTION_ALLOW_REGISTRY_OVERRIDES.key,
                                                                               enable_attribute='allow_registry_overrides',
                                                                               disable_attribute='disallow_registry_overrides')

    if args.quiet:
        o3de_logger.setLevel(logging.ERROR)
    else:
        o3de_logger.setLevel(logging.INFO)

    try:
        export_standalone_project(ctx=o3de_context,
                                  selected_platform=args.platform,
                                  output_path=args.output_path,
                                  should_build_tools=option_build_tools,
                                  build_config=args.config,
                                  tool_config=args.tool_config,
                                  seedlist_paths=args.seedlist_paths,
                                  seedfile_paths=args.seedfile_paths,
                                  level_names=args.level_names,
                                  game_project_file_patterns_to_copy=args.game_project_file_patterns_to_copy,
                                  server_project_file_patterns_to_copy=args.server_project_file_patterns_to_copy,
                                  project_file_patterns_to_copy=args.project_file_patterns_to_copy,
                                  asset_bundling_path=args.asset_bundling_path,
                                  max_bundle_size=args.max_bundle_size,
                                  should_build_all_assets=option_build_assets,
                                  fail_on_asset_errors=fail_on_asset_errors,
                                  should_build_game_launcher=option_build_game_launcher,
                                  should_build_server_launcher=option_build_server_launcher,
                                  should_build_unified_launcher=option_build_unified_launcher,
                                  engine_centric=option_build_engine_centric,
                                  allow_registry_overrides=option_allow_registry_overrrides,
                                  tools_build_path=args.tools_build_path,
                                  launcher_build_path=args.launcher_build_path,
                                  archive_output_format=args.archive_output,
                                  monolithic_build=not args.no_monolithic_build,
                                  logger=o3de_logger)
    except exp.ExportProjectError as err:
        print(err)
        sys.exit(1)
