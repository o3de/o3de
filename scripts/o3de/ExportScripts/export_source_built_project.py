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
import pathlib

from typing import List


def export_standalone_monolithic_project_centric(ctx: exp.O3DEScriptExportContext,
                                                 selected_platform: str,
                                                 output_path: pathlib.Path,
                                                 should_build_tools: bool,
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

    validated_seedslist_paths = exp.validate_project_artifact_paths(project_path=ctx.project_path,
                                                                    artifact_paths=seedlist_paths)

    exp.kill_existing_processes(ctx.project_name)

    # Optionally build the toolchain needed to bundle the assets
    if should_build_tools:
        exp.build_export_toolchain(ctx=ctx,
                                   tools_build_path=tools_build_path,
                                   logger=logger)
    else:
        exp.validate_export_toolchain(tools_build_path=tools_build_path)

    if should_build_all_code:

        launcher_type = 0
        if should_build_game_launcher:
            launcher_type |= exp.LauncherType.GAME
        if should_build_server_launcher:
            launcher_type |= exp.LauncherType.SERVER
        if should_build_unified_launcher:
            launcher_type |= exp.LauncherType.UNIFIED

        exp.build_game_targets(ctx=ctx,
                               build_config=build_config,
                               game_build_path=game_build_path,
                               launcher_types=launcher_type,
                               allow_registry_overrides=allow_registry_overrides,
                               logger=logger)

    if should_build_all_assets:
        exp.build_assets(ctx=ctx,
                         tools_build_path=tools_build_path,
                         fail_on_ap_errors=fail_on_asset_errors,
                         logger=logger)

        expected_bundles_path = exp.bundle_assets(ctx=ctx,
                                                  selected_platform=selected_platform,
                                                  seedlist_paths=validated_seedslist_paths,
                                                  tools_build_path=tools_build_path,
                                                  custom_asset_list_path=preferred_asset_list_path,
                                                  max_bundle_size=max_bundle_size)
    else:
        expected_bundles_path = ctx.project_path / 'AssetBundling' / 'Bundles'

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

    for export_layout in export_layouts:
        exp.setup_launcher_layout_directory(project_path=ctx.project_path,
                                            asset_platform=selected_platform,
                                            game_build_path=game_build_path,
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

    def parse_args(o3de_context: exp.O3DEScriptExportContext):

        parser = argparse.ArgumentParser(
                    prog=f'o3de.py export-project -es {__file__}',
                    description="Exports a project as standalone to the desired output directory with release layout. "
                                "In order to use this script, the engine and project must be setup and registered beforehand. ",
                    epilog="Note: You can pass additional arguments to the cmake command to build the projects during the export"
                           "process by adding a '/' between the arguments for this script and the additional arguments you would"
                           "like to pass down to cmake build.",
                    add_help=False
        )
        parser.add_argument(exp.CUSTOM_SCRIPT_HELP_ARGUMENT,default=False,action='store_true',help='Show this help message and exit.')
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
        parser.add_argument('-bt', '--build-tools', default=True, action='store_true',
                            help="Specifies whether to build O3DE toolchain executables. This will build AssetBundlerBatch, AssetProcessorBatch.")
        parser.add_argument('-tbp', '--tools-build-path', type=pathlib.Path, default=None,
                            help='Designates where O3DE toolchain executables go. If not specified, default is <o3de_project_path>/build/tools.')
        parser.add_argument('-gbp', '--game-build-path', type=pathlib.Path, default=None,
                            help="Designates where project executables (like Game/Server Launcher) go."
                            " If not specified, default is <o3de_project_path>/build/game.")
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
        export_standalone_monolithic_project_centric(ctx=o3de_context,
                                                     selected_platform=args.platform,
                                                     output_path=args.output_path,
                                                     should_build_tools=args.build_tools,
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
                                                     tools_build_path=args.tools_build_path,
                                                     game_build_path=args.game_build_path,
                                                     archive_output_format=args.archive_output,
                                                     logger=o3de_logger)
    except exp.ExportProjectError as err:
        print(err)
        sys.exit(1)
