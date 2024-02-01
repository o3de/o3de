#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import logging
import os
import sys
import shutil

import o3de.export_project as exp
import export_utility as eutil
import o3de.command_utils as command_utils
import pathlib

ASSET_PLATFORM = 'ios'

from typing import List

def export_ios_xcode_project(ctx: exp.O3DEScriptExportContext,
                             target_ios_project_path: pathlib.Path,
                             seedlist_paths: List[pathlib.Path],
                             seedfile_paths: List[pathlib.Path],
                             level_names: List[str],
                             should_build_tools:bool,
                             should_build_all_assets: bool,
                             build_config,
                             tool_config,
                             tools_build_path: pathlib.Path,
                             max_bundle_size: int,
                             fail_on_asset_errors: bool,
                             logger: logging.Logger|None = None):
    """
    This function serves as an initial exporter for project to the iOS platform. The steps in this code will generate
    an Xcode project file containing all necessary build information to produce iOS executables. 
    In order to build and deploy projects, it is recommended to open the project file in Xcode to configure and build accordingly.

    Note: In order to use this functionality, you must be running this script from a MacOS machine with a valid copy of Xcode. 
    This will also require setting up an AppleID and provisioning profile in Xcode when the resulting xcode-project file is generated before deploying to device.

    Instructions to handle iOS projects in Xcode will be provided soon. This export function is currently experimental.

    :param ctx:                                     The O3DE Script context provided by the export-command
    :param tools_build_path:                        Optional build path to build the tools. (see default_tools_path below)
    :param ios_build_folder:                        The base output path of the generated Xcode Project file for iOS
    :param should_build_tools:                      Option to build the export process dependent tools (AssetProcessor, AssetBundlerBatch, and dependencies)
    :param should_build_all_assets:                 Option to process all the assets for the game
    :param logger:                                  Optional logger to use to log the process and errors
    """
    
    # Use a provided logger or get the current system one
    if not logger:
        logger = logging.getLogger()
        logger.setLevel(logging.ERROR)

    tools_build_path_str = str(tools_build_path)
    ios_build_folder_str = str(target_ios_project_path)

    default_base_path =  ctx.project_path

    # Resolve (if possible) and validate any provided seedlist files
    validated_seedslist_paths = exp.validate_project_artifact_paths(project_path=ctx.project_path,
                                                                    artifact_paths=seedlist_paths)
    
    # Preprocess the seed file paths in case there are any wildcard patterns to use
    preprocessed_seedfile_paths = exp.preprocess_seed_path_list(project_path=ctx.project_path,
                                                                paths=seedfile_paths)
    
    eutil.process_level_names(ctx, preprocessed_seedfile_paths, level_names, ASSET_PLATFORM)

    # Optionally build the toolchain needed to process the assets
    # note: For now, I am keeping the tool building logic restricted on iOS, b/c atm it's unreliable for most configurations besides this one
    if should_build_tools:
        exp.process_command(["cmake", "-B", tools_build_path_str, '-G', "Xcode"], cwd=ctx.project_path)

        exp.process_command(["cmake", "--build", tools_build_path_str, "--target", "Editor", "AssetProcessor", "AssetBundler","AssetProcessorBatch", "AssetBundlerBatch", "--config", "profile"],
                    cwd=ctx.project_path)
        
    # Optionally process the assets
    eutil.build_and_bundle_assets(ctx, should_build_all_assets, tools_build_path, False, 
                                  tool_config, False, fail_on_asset_errors,
                                  [ASSET_PLATFORM], validated_seedslist_paths, preprocessed_seedfile_paths, 
                                  ctx.project_path/'AssetBundling', max_bundle_size,
                                  logger)

    # Generate the Xcode project file for the O3DE project
    cmake_toolchain_path = ctx.engine_path / 'cmake/Platform/iOS/Toolchain_ios.cmake'

    exp.process_command(['cmake', '-B', ios_build_folder_str, '-G', "Xcode", f'-DCMAKE_TOOLCHAIN_FILE={str(cmake_toolchain_path)}', '-DLY_MONOLITHIC_GAME=1'],
                    cwd= ctx.project_path)


    logger.info(f"Xcode project file should be generated now. Please check {ios_build_folder_str}")

    xcode_build_common_args = ['xcodebuild', '-project', str(target_ios_project_path / f'{ctx.project_name}.xcodeproj'),
                        '-scheme', f'{ctx.project_name}.GameLauncher', '-configuration', build_config, '-sdk',
                        'iphoneos', '-allowProvisioningUpdates']

    exp.process_command(xcode_build_common_args + ['clean'], cwd=ctx.project_path)
    
    exp.process_command(xcode_build_common_args + ['build'], cwd=ctx.project_path)
    
    # Note: This workaround enables the creation of an IPA file, even if xcode archive doesn't work, but will require manual upload
    payload_path = target_ios_project_path / 'bin'/ build_config /'Payload'
    payload_path_str = str(payload_path)
    exp.process_command(['mkdir', payload_path_str], cwd=ctx.project_path)
    exp.process_command(['cp', '-r', str(target_ios_project_path / 'bin'/ build_config / f'{ctx.project_name}.GameLauncher.app'), 
                        str( payload_path / f'{ctx.project_name}.GameLauncher.app')], cwd=ctx.project_path)
    
    shutil.make_archive(payload_path, 'zip', root_dir=(payload_path.parent.resolve()))
    ipa_path_str = str(target_ios_project_path / 'bin'/ build_config /f'{ctx.project_name}.GameLauncher.ipa')
    exp.process_command(['mv', str(payload_path)+".zip", ipa_path_str])

    logger.info(f"iOS IPA file should be generated now. Please check {ipa_path_str}")


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
        o3de_logger.debug(f"No project detected at {os.getcwd()}, default settings from global config.")
        project_name = None
        export_config = exp.get_export_project_config(project_path=None)

    def parse_args(o3de_context: exp.O3DEScriptExportContext):
        parser = argparse.ArgumentParser(
                    prog=f'o3de.py export-project -es {__file__}',
                    description="Exports a project as a generated iOS Xcode project in the project directory. "
                                "In order to use this script, the engine and project must be setup and registered beforehand. ",
                    epilog=exp.CUSTOM_CMAKE_ARG_HELP_EPILOGUE,
                    formatter_class=argparse.RawTextHelpFormatter,
                    add_help=False
        )

        eutil.create_common_export_params_and_config(parser, export_config)

        default_ios_path = export_config.get_value(exp.SETTINGS_DEFAULT_IOS_BUILD_PATH.key, exp.SETTINGS_DEFAULT_IOS_BUILD_PATH.default)
        parser.add_argument('-ibp', '--ios-build-path', type=pathlib.Path, default=pathlib.Path(default_ios_path),
                            help=f'Designates where the build files for the O3DE toolchain are generated. If not specified, default is {default_ios_path}')

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

    if args.quiet:
        o3de_logger.setLevel(logging.ERROR)
    try:
        export_ios_xcode_project(ctx=o3de_context,
                                 target_ios_project_path=args.ios_build_path,
                                 seedlist_paths=args.seedlist_paths,
                                 seedfile_paths=args.seedfile_paths,
                                 level_names=args.level_names,
                                 should_build_tools=option_build_tools,
                                 should_build_all_assets=option_build_assets,
                                 build_config=args.config,
                                 tool_config=args.tool_config,
                                 tools_build_path=args.tools_build_path,
                                 max_bundle_size=args.max_bundle_size,
                                 fail_on_asset_errors=args.fail_on_asset_errors,
                                 logger=o3de_logger)
    except exp.ExportProjectError as err:
        print(err)
        sys.exit(1)
