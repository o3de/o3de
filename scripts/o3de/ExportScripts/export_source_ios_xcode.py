#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import logging
import sys

import o3de.export_project as exp
from o3de.export_project import process_command
import pathlib

def export_ios_Xcode_project(ctx: exp.O3DEScriptExportContext,
                             tools_build_folder: pathlib.Path,
                             ios_build_folder: pathlib.Path,
                             should_build_tools:bool = True,
                             skip_asset_processing: bool = False,
                             logger: logging.Logger|None = None):
    """
    This function serves as an initial exporter for project to the iOS platform. The steps in this code will generate
    an Xcode project file containing all necessary build information to produce iOS executables. 
    In order to build and deploy projects, it is recommended to open the project file in Xcode to configure and build accordingly.

    Note: In order to use this functionality, you must be running this script from a MacOS machine with a valid copy of Xcode

    Instructions to handle iOS projects in Xcode will be provided soon.

    :param ctx:                                     The O3DE Script context provided by the export-command
    :param tools_build_folder:                      Optional build path to build the tools. (Will default to build/tools if not supplied)
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
        process_command(["cmake", "-B", tools_build_folder_str, '-G', "Xcode","-DLY_UNITY_BUILD=ON"], cwd=ctx.project_path)

        process_command(["cmake", "--build", tools_build_folder_str, "--target", "Editor", "AssetProcessorBatch", "--config", "profile"],
                    cwd=ctx.project_path)
        
    # Optionally process the assets
    if not skip_asset_processing:
        asset_processor_batch_path = exp.get_asset_processor_batch_path(tools_build_folder, True)
        process_command([ str(asset_processor_batch_path), '--platforms=ios' ,
                        '--project-path', ctx.project_path ], cwd=ctx.project_path)

    # Generate the Xcode project file for the O3DE project
    cmake_toolchain_path = ctx.engine_path / 'cmake/Platform/iOS/Toolchain_ios.cmake'

    process_command(['cmake', '-B', ios_build_folder_str, '-G', "Xcode", f'-DCMAKE_TOOLCHAIN_FILE={str(cmake_toolchain_path)}', '-DLY_UNITY_BUILD=ON', '-DLY_MONOLITHIC_GAME=1'],
                    cwd= ctx.project_path)


    logger.info(f"Xcode project file should be generated now. Please check {ios_build_folder_str}")



# This code is only run by the 'export-project' O3DE CLI command
if "o3de_context" in globals():
    global o3de_context
    global o3de_logger

    def parse_args(o3de_context: exp.O3DEScriptExportContext):
        parser = argparse.ArgumentParser(
                    prog=f'o3de.py export-project -es {__file__}',
                    description="Exports a project as a generated iOS Xcode project in the project directory. "
                                "In order to use this script, the engine and project must be setup and registered beforehand. ",
                    epilog=exp.CUSTOM_CMAKE_ARG_HELP_EPILOGUE,
                    formatter_class=argparse.RawTextHelpFormatter,
                    add_help=False
        )

        default_tools_path = o3de_context.project_path / 'build/Mac'
        default_ios_path = o3de_context.project_path / 'build/iOS'
        parser.add_argument(exp.CUSTOM_SCRIPT_HELP_ARGUMENT,default=False,action='store_true',help='Show this help message and exit.')
        parser.add_argument('-bt', '--build-tools', default=True, action='store_true',
                            help="Specifies whether to build O3DE toolchain executables. This will build AssetBundlerBatch, AssetProcessorBatch.")
        parser.add_argument('-tbp', '--tools-build-path', type=pathlib.Path, default=default_tools_path,
                                help=f'Designates where the build files for the O3DE toolchain are generated. If not specified, default is {default_tools_path}.')
        parser.add_argument('-ibp', '--ios-build-path', type=pathlib.Path, default=default_ios_path,
                                help=f'Designates where the build files for the O3DE toolchain are generated. If not specified, default is {default_ios_path}.')
        parser.add_argument('-assets', '--skip-asset-processing', default=False, action='store_true',
                                help='Toggles processing all assets for the iOS build.')
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
    if args.quiet:
        o3de_logger.setLevel(logging.ERROR)
    try:
        export_ios_Xcode_project(ctx = o3de_context,
                            tools_build_folder= args.tools_build_path,
                            ios_build_folder=args.ios_build_path,
                            should_build_tools = args.build_tools,
                            skip_asset_processing = args.skip_asset_processing,
                            logger = o3de_logger)
    except exp.ExportProjectError as err:
        print(err)
        sys.exit(1)
