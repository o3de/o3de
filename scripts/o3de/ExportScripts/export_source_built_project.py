#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import fnmatch
import glob
import logging
import os
import sys

import o3de.export_project as exp
import pathlib
import shutil

from o3de.export_project import process_command
from o3de import manifest
from typing import List

# This script is meant to be run via o3de cli export-project command, not from Python directly!
# This export script supports optional building of asset tools, so it is only compatible with the source distribution
# of O3DE (https://github.com/o3de/o3de)

# Example invocation on Linux
'''
$ cd <project-path>
$ <engine-path>/scripts/o3de.sh export-project --export-scripts ExportScripts/export_source_built_project.py \
 --project-path . --log-level INFO --output-path <install-path> --build-non-mono-tools --config release --archive-output zip \
 --seedlist ./Assets/seedlist1.seed --seedlist ./Assets/seedlist2.seed <etc..> 
'''

# Example invocation on Windows
'''
$ cd <project-path>
$ <engine-path>\scripts\o3de.bat  export-project --export-scripts ExportScripts\eexport_source_built_project.py \
 --project-path . --log-level INFO --output-path <install-path> --build-non-mono-tools --config release --archive-output zip \
 --seedlist .\Assets\seedlist1.seed --seedlist .\Assets\seedlist2.seed <etc..> 
''' 

# The following parameters are optional, but may be helpful:
#   seedlist: an O3DE seedlist for describing what assets of the project should be bundled
#   project-file-pattern-to-copy: Any regex pattern starting from <ProjectRoot> that indicates what files should be copied to output location.
#                                   Files found in these patterns will go into <output-path>
#                                   Variants for game/server launcher only are game-file-pattern-to-copy/server-file-pattern-to-copy resp.

# For more information on the available parameters for this script, check the parse_arguments function defined at the bottom of the file


def export_standalone_monolithic_project_centric(ctx: exp.O3DEScriptExportContext,
                                                 selected_platform: str,
                                                 output_path: pathlib.Path,
                                                 should_export_toolchain: bool,
                                                 build_config: str,
                                                 seedlist_paths: List[pathlib.Path],
                                                 game_project_file_patterns_to_copy: List[str] = [],
                                                 server_project_file_patterns_to_copy: List[str] = [],
                                                 project_file_patterns_to_copy: List[str] = [],
                                                 preferred_asset_list_path: pathlib.Path|None = None,
                                                 max_bundle_size: int = 2048,
                                                 should_build_all_code: bool = True,
                                                 should_build_all_assets: bool = True,
                                                 should_build_game_launcher: bool = True,
                                                 should_build_server_launcher: bool = True,
                                                 should_build_unified_launcher: bool = True,
                                                 allow_registry_overrides: bool = False,
                                                 tools_build_path: pathlib.Path | None =None,
                                                 game_build_path: pathlib.Path | None =None,
                                                 archive_output_format: str = "none",
                                                 fail_on_asset_errors: bool = False,
                                                 logger: logging.Logger|None = None):
    if not logger:
        logger = logging.getLogger()
        logger.setLevel(logging.ERROR)

    default_base_build_path = ctx.engine_path / 'build' if ctx.is_engine_centric else ctx.project_path / 'build'

    if not tools_build_path:
        tools_build_path = default_base_build_path / 'tools'
    if not game_build_path:
        game_build_path = default_base_build_path / 'game'

    exp.kill_existing_processes(ctx.project_name)

    # Optionally build the toolchain needed to bundle the assets
    if should_export_toolchain:
        exp.build_export_toolchain(ctx=ctx,
                                   tools_build_path=tools_build_path,
                                   logger=logger)
    else:
        # Otherwise make sure the tools exist already
        build_tools = [exp.get_asset_bundler_batch_path(tools_build_path), exp.get_asset_processor_batch_path(tools_build_path)]
        tools_missing = [b for b in build_tools if not b.exists()]
        if len(tools_missing) > 0:
            raise exp.ExportProjectError(f"Necessary Build Tools have not been created! The following are missing: {', '.join(tools_missing)}"
                                         "Please ensure that these tools exist before proceeding on with the build!")

    if should_build_all_code:
        exp.build_game_targets(ctx=ctx,
                               build_config=build_config,
                               game_build_path=game_build_path,
                               should_build_game_launcher=should_build_game_launcher,
                               should_build_server_launcher=should_build_server_launcher,
                               should_build_unified_launcher=should_build_unified_launcher,
                               allow_registry_overrides=allow_registry_overrides,
                               logger=logger)

    if should_build_all_assets:
        exp.build_assets(ctx=ctx,
                         tools_build_path=tools_build_path,
                         fail_on_ap_errors=fail_on_asset_errors,
                         logger=logger)

        expected_bundles_path = exp.bundle_assets(ctx=ctx,
                                                  selected_platform=selected_platform,
                                                  seedlist_paths=seedlist_paths,
                                                  non_mono_build_path=tools_build_path,
                                                  custom_asset_list_path=preferred_asset_list_path,
                                                  max_bundle_size=max_bundle_size)
    else:
        expected_bundles_path = ctx.project_path / "AssetBundling/Bundles"

    for should_do, expected_output_dir, project_file_patterns, ignore_patterns in zip(
                                              [should_build_game_launcher, should_build_server_launcher, should_build_unified_launcher],
                                              [output_path / f'{ctx.project_name}GamePackage',
                                               output_path / f'{ctx.project_name}ServerPackage',
                                               output_path / f'{ctx.project_name}UnifiedPackage'],
                                              [project_file_patterns_to_copy + game_project_file_patterns_to_copy,
                                               project_file_patterns_to_copy + server_project_file_patterns_to_copy,
                                               project_file_patterns_to_copy + game_project_file_patterns_to_copy + server_project_file_patterns_to_copy],
                                              [['*.ServerLauncher', '*.UnifiedLauncher'],
                                               ['*.GameLauncher', '*.UnifiedLauncher'],
                                               ['*.ServerLauncher', '*.GameLauncher']]
    ):
        if should_do:
            exp.setup_launcher_layout_directory(project_path=ctx.project_path,
                                                output_path=expected_output_dir,
                                                asset_platform=selected_platform,
                                                mono_build_path=game_build_path,
                                                build_config=build_config,
                                                bundles_to_copy=[expected_bundles_path / f'game_{selected_platform}.pak', expected_bundles_path / f'engine_{selected_platform}.pak'],
                                                project_file_patterns_to_copy=project_file_patterns,
                                                archive_output_format=archive_output_format,
                                                logger=logger,
                                                ignore_file_patterns=ignore_patterns)


# This code is only run by the 'export-project' O3DE CLI command
if "o3de_context" in globals():

    global o3de_context
    global o3de_logger

    def parse_args(unprocessed_args: List[str]):

        parser = argparse.ArgumentParser(
                    prog='Exporter for standalone builds',
                    description="Exports a project as standalone to the desired output directory with release layout. "
                                "In order to use this script, the engine and project must be setup and registered beforehand. ",
                    epilog="Note: You can pass additional arguments to the cmake command to build the projects during the export"
                           "process by adding a '/' between the arguments for this script and the additional arguments you would"
                           "like to pass down to cmake build."
        )
        parser.add_argument('-out', '--output-path', type=pathlib.Path, required=True, help='Path that describes the final resulting Release Directory path location.')
        parser.add_argument('-cfg', '--config', type=str, default='profile', choices=['release', 'profile'],
                            help='The CMake build configuration to use when building project binaries.  Tool binaries built with this script will always be built with the profile configuration.')
        parser.add_argument('-a', '--archive-output',  type=str,
                            help="Option to create a compressed archive the output. "
                                 "Specify the format of archive to create from the output directory. If 'none' specified, no archiving will occur.",
                            choices=["none", "zip", "gzip", "bz2", "xz"], default="none")
        parser.add_argument('-code', '--should-build-code', default=False,action='store_true',
                            help='Toggles building all code for the project by launcher type (game, server, unified).')
        parser.add_argument('-assets', '--should-build-assets', default=False, action='store_true',
                            help='Toggles building all assets for the project by launcher type (game, server, unified).')
        parser.add_argument('-foa', '--fail-on-asset-errors', default=False, action='store_true',
                            help='Option to fail the project export process on any failed asset during asset building (applicable if --should-build-assets is true)')
        parser.add_argument('-sl', '--seedlist', type=pathlib.Path, dest='seedlist_paths', action='append',
                                help='Path to a seed list file for asset bundling. Specify multiple times for each seed list.')
        parser.add_argument('-gpfp', '--game-project-file-pattern-to-copy', type=str, dest='game_project_file_patterns_to_copy', action='append',
                                help="Any additional file patterns located in the project directory. File patterns will be relative to the project path. Will be included in GameLauncher.")
        parser.add_argument('-spfp', '--server-project-file-pattern-to-copy', type=str, dest='server_project_file_patterns_to_copy', action='append',
                                help="Any additional file patterns located in the project directory. File patterns will be relative to the project path. Will be included in ServerLauncher.")
        parser.add_argument('-pfp', '--project-file-pattern-to-copy', type=str, dest='project_file_patterns_to_copy', action='append',
                                help="Any additional file patterns located in the project directory. File patterns will be relative to the project path.")
        parser.add_argument('-bnmt', '--build-non-mono-tools', default=True, action='store_true',
                            help="Specifies whether to build O3DE toolchain executables. This will build AssetBundlerBatch, AssetProcessorBatch.")
        parser.add_argument('-nmbp', '--non-mono-build-path', type=pathlib.Path, default=None,
                            help='Designates where O3DE toolchain executables go. If not specified, default is <o3de_project_path>/build/non_mono.')
        parser.add_argument('-mbp', '--mono-build-path', type=pathlib.Path, default=None,
                            help="Designates where project executables (like Game/Server Launcher) go."
                            " If not specified, default is <o3de_project_path>/build/mono.")
        parser.add_argument('-regovr', '--allow-registry-overrides', default=False, type = bool,
                            help="When configuring cmake builds, this determines if the script allows for overriding registry settings from external sources.")
        parser.add_argument('-assetlistpath', '--preferred-asset-list-path', type=pathlib.Path, default=None,
                            help="Specify a custom location to place asset list files during the bundling process.")
        parser.add_argument('-maxsize', '--max-bundle-size', type=int, default=2048, help='Specify the maximum size of a given asset bundle.')
        parser.add_argument('-nogame', '--no-game-launcher', action='store_true', help='This flag skips building the Game Launcher on a platform if not needed.')
        parser.add_argument('-noserver', '--no-server-launcher', action='store_true', help='This flag skips building the Server Launcher on a platform if not needed.')
        parser.add_argument('-nounified', '--no-unified-launcher', action='store_true', help='This flag skips building the Unified Launcher on a platform if not needed.')
        parser.add_argument('-pl', '--platform', type=str, default=exp.get_default_asset_platform(), choices=['pc', 'linux', 'mac'])
        parser.add_argument('-q', '--quiet', action='store_true', help='Suppresses logging information unless an error occurs.')
        
        return parser.parse_args(unprocessed_args)
    
    args = parse_args(o3de_context.args)
    if args.quiet:
        o3de_logger.setLevel(logging.ERROR)
    try:
        export_standalone_monolithic_project_centric(ctx=o3de_context,
                                                     selected_platform=args.platform,
                                                     output_path=args.output_path,
                                                     should_export_toolchain=args.build_non_mono_tools,
                                                     build_config=args.config,
                                                     seedlist_paths=[] if not args.seedlist_paths else args.seedlist_paths,
                                                     game_project_file_patterns_to_copy=[] if not args.game_project_file_patterns_to_copy else args.game_project_file_patterns_to_copy,
                                                     server_project_file_patterns_to_copy=[] if not args.server_project_file_patterns_to_copy else args.server_project_file_patterns_to_copy,
                                                     project_file_patterns_to_copy=[] if not args.project_file_patterns_to_copy else args.project_file_patterns_to_copy,
                                                     preferred_asset_list_path=args.preferred_asset_list_path,
                                                     max_bundle_size=args.max_bundle_size,
                                                     should_build_all_code=args.should_build_code,
                                                     should_build_all_assets=args.should_build_assets,
                                                     fail_on_asset_errors=args.fail_on_asset_errors,
                                                     should_build_game_launcher=not args.no_game_launcher,
                                                     should_build_server_launcher=not args.no_server_launcher,
                                                     should_build_unified_launcher=not args.no_unified_launcher,
                                                     allow_registry_overrides=args.allow_registry_overrides,
                                                     tools_build_path=args.non_mono_build_path,
                                                     game_build_path=args.mono_build_path,
                                                     archive_output_format=args.archive_output,
                                                     logger=o3de_logger)
    except exp.ExportProjectError as err:
        print(exp)
        sys.exit(1)
