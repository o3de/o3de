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

import o3de.export_project as exp
import o3de.command_utils as command_utils
import pathlib

def export_ios_xcode_project(ctx: exp.O3DEScriptExportContext,
                             tools_build_folder: pathlib.Path,
                             ios_build_folder: pathlib.Path,
                             should_build_tools:bool = True,
                             skip_asset_processing: bool = False,
                             logger: logging.Logger|None = None):
    """
    This function serves as an initial exporter for project to the iOS platform. The steps in this code will generate
    an Xcode project file containing all necessary build information to produce iOS executables. 
    In order to build and deploy projects, it is recommended to open the project file in Xcode to configure and build accordingly.

    Note: In order to use this functionality, you must be running this script from a MacOS machine with a valid copy of Xcode. 
    This will also require setting up an AppleID and provisioning profile in Xcode when the resulting xcode-project file is generated before deploying to device.

    Instructions to handle iOS projects in Xcode will be provided soon. This export function is currently experimental.

    :param ctx:                                     The O3DE Script context provided by the export-command
    :param tools_build_folder:                      Optional build path to build the tools. (see default_tools_path below)
    :param ios_build_folder:                        The base output path of the generated Xcode Project file for iOS
    :param should_build_tools:                      Option to build the export process dependent tools (AssetProcessor, AssetBundlerBatch, and dependencies)
    :param should_build_all_assets:                 Option to process all the assets for the game
    :param logger:                                  Optional logger to use to log the process and errors
    """
    
    # Use a provided logger or get the current system one
    if not logger:
        logger = logging.getLogger()
        logger.setLevel(logging.ERROR)

    tools_build_folder_str = str(tools_build_folder)
    ios_build_folder_str = str(ios_build_folder)

    # Optionally build the toolchain needed to process the assets
    if should_build_tools:
        exp.process_command(["cmake", "-B", tools_build_folder_str, '-G', "Xcode"], cwd=ctx.project_path)

        exp.process_command(["cmake", "--build", tools_build_folder_str, "--target", "AssetProcessorBatch", "AssetBundlerBatch", "--config", "profile"],
                    cwd=ctx.project_path)
        
    # Optionally process the assets
    if not skip_asset_processing:
        asset_processor_batch_path = exp.get_asset_processor_batch_path(tools_build_folder, required=True)
        exp.process_command([ str(asset_processor_batch_path), '--platforms=ios',
                        '--project-path', ctx.project_path ], cwd=ctx.project_path)

    # Generate the Xcode project file for the O3DE project
    cmake_toolchain_path = ctx.engine_path / 'cmake/Platform/iOS/Toolchain_ios.cmake'

    exp.process_command(['cmake', '-B', ios_build_folder_str, '-G', "Xcode", f'-DCMAKE_TOOLCHAIN_FILE={str(cmake_toolchain_path)}', '-DLY_MONOLITHIC_GAME=1'],
                    cwd= ctx.project_path)


    logger.info(f"Xcode project file should be generated now. Please check {ios_build_folder_str}")



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

        parser.add_argument(exp.CUSTOM_SCRIPT_HELP_ARGUMENT, default=False, action='store_true', help='Show this help message and exit.')

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

        default_ios_path = export_config.get_value(exp.SETTINGS_DEFAULT_IOS_BUILD_PATH.key, exp.SETTINGS_DEFAULT_IOS_BUILD_PATH.default)
        parser.add_argument('-ibp', '--ios-build-path', type=pathlib.Path, default=pathlib.Path(default_ios_path),
                            help=f'Designates where the build files for the O3DE toolchain are generated. If not specified, default is {default_ios_path}')

        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_ASSETS.key,
                                           enable_override_arg=['-assets', '--should-build-assets'],
                                           enable_override_desc='Build and update all iOS assets before bundling.',
                                           disable_override_arg=['-noassets', '--skip-build-assets'],
                                           disable_override_desc='Skip building of iOS assets and use assets that were already built.')

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

    if args.quiet:
        o3de_logger.setLevel(logging.ERROR)
    try:
        export_ios_xcode_project(ctx=o3de_context,
                                 tools_build_folder=args.tools_build_path,
                                 ios_build_folder=args.ios_build_path,
                                 should_build_tools=option_build_tools,
                                 skip_asset_processing=not option_build_assets,
                                 logger=o3de_logger)
    except exp.ExportProjectError as err:
        print(err)
        sys.exit(1)
