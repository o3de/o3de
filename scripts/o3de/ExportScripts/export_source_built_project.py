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


def export_standalone_project(ctx: exp.O3DEScriptExportContext,
                              selected_platform: str,
                              output_path: pathlib.Path,
                              should_build_tools: bool,
                              build_config: str,
                              seedlist_paths: List[pathlib.Path],
                              game_project_file_patterns_to_copy: List[str] = [],
                              server_project_file_patterns_to_copy: List[str] = [],
                              project_file_patterns_to_copy: List[str] = [],
                              asset_bundling_path: pathlib.Path|None = None,
                              max_bundle_size: int = 2048,
                              should_build_all_assets: bool = True,
                              should_build_game_launcher: bool = True,
                              should_build_server_launcher: bool = True,
                              should_build_unified_launcher: bool = True,
                              should_build_headless_server_launcher: bool = True,
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
    :param seedlist_paths:                          List of seed list files to optionally include in the asset bundling process
    :param game_project_file_patterns_to_copy:      List of game (or unified) launcher specific files to include in the game/unified package
    :param server_project_file_patterns_to_copy:    List of server (or unified) launcher specific files to include in the server/unified package
    :param project_file_patterns_to_copy:           List of general file patterns to include in all packages
    :param asset_bundling_path:                     Optional path to write the artifacts from the asset bundling process
    :param max_bundle_size:                         Maximum size to set for the archived bundle files
    :param should_build_all_assets:                 Option to build/process all the assets for the game
    :param should_build_game_launcher:              Option to build the game launcher package
    :param should_build_server_launcher:            Option to build the server launcher package
    :param should_build_unified_launcher:           Option to build the unified launcher package
    :param should_build_headless_server_launcher:   Option to build the headless server launcher package
    :param allow_registry_overrides:                Option to allow registry overrides in the build process
    :param tools_build_path:                        Optional build path to build the tools. (Will default to build/tools if not supplied)
    :param launcher_build_path:                     Optional build path to build the game launcher(s). (Will default to build/launcher if not supplied)
    :param archive_output_format:                   Optional archive format to use for archiving the final package(s)
    :param fail_on_asset_errors:                    Option to fail the process on any asset processing error
    :param engine_centric:                          Option to use an engine-centric workflow or not
    :param logger:                                  Optional logger to use to log the process and errors
    """

    # Use a provided logger or get the current system one
    if not logger:
        logger = logging.getLogger()
        logger.setLevel(logging.ERROR)

    # Calculate the tools and game build paths
    default_base_build_path = ctx.engine_path / 'build' if engine_centric else ctx.project_path / 'build'
    if not tools_build_path:
        tools_build_path = default_base_build_path / 'tools'
    if not launcher_build_path:
        launcher_build_path = default_base_build_path / 'launcher'
    if not asset_bundling_path:
        asset_bundling_path = default_base_build_path / 'asset_bundling'

    # Resolve (if possible) and validate any provided seedlist files
    validated_seedslist_paths = exp.validate_project_artifact_paths(project_path=ctx.project_path,
                                                                    artifact_paths=seedlist_paths)

    # Make sure there are no running processes for the current project before continuing
    exp.kill_existing_processes(ctx.project_name)

    # Optionally build the toolchain needed to bundle the assets
    if should_build_tools:
        exp.build_export_toolchain(ctx=ctx,
                                   tools_build_path=tools_build_path,
                                   engine_centric=engine_centric,
                                   logger=logger)

    # Build the requested game launcher types (if any)
    launcher_type = 0
    if should_build_game_launcher:
        launcher_type |= exp.LauncherType.GAME
    if should_build_server_launcher:
        launcher_type |= exp.LauncherType.SERVER
    if should_build_unified_launcher:
        launcher_type |= exp.LauncherType.UNIFIED
    if should_build_headless_server_launcher:
        launcher_type |= exp.LauncherType.HEADLESS

    if launcher_type != 0:
        exp.build_game_targets(ctx=ctx,
                               build_config=build_config,
                               game_build_path=launcher_build_path,
                               engine_centric=engine_centric,
                               launcher_types=launcher_type,
                               allow_registry_overrides=allow_registry_overrides,
                               logger=logger)

    # Optionally build the assets
    if should_build_all_assets:
        asset_processor_path = exp.get_asset_processor_batch_path(tools_build_path=tools_build_path,
                                                                  required=True)
        logger.info(f"Using '{asset_processor_path}' to process the assets.")
        exp.build_assets(ctx=ctx,
                         tools_build_path=tools_build_path,
                         engine_centric=engine_centric,
                         fail_on_ap_errors=fail_on_asset_errors,
                         logger=logger)

    # Generate the bundle
    asset_bundler_path = exp.get_asset_bundler_batch_path(tools_build_path=tools_build_path,
                                                          required=True)
    logger.info(f"Using '{asset_bundler_path}' to bundle the assets.")
    expected_bundles_path = exp.bundle_assets(ctx=ctx,
                                              selected_platform=selected_platform,
                                              seedlist_paths=validated_seedslist_paths,
                                              tools_build_path=tools_build_path,
                                              engine_centric=engine_centric,
                                              asset_bundling_path=asset_bundling_path,
                                              max_bundle_size=max_bundle_size)

    # Prepare the different layouts based on the desired launcher types
    export_layouts = []
    if should_build_game_launcher:
        export_layouts.append(exp.ExportLayoutConfig(output_path=output_path / f'{ctx.project_name}GamePackage',
                                                     project_file_patterns=project_file_patterns_to_copy + game_project_file_patterns_to_copy,
                                                     ignore_file_patterns=[f'*.ServerLauncher{exp.EXECUTABLE_EXTENSION}',
                                                                           f'*.HeadlessServerLauncher{exp.EXECUTABLE_EXTENSION}',
                                                                           f'*.UnifiedLauncher{exp.EXECUTABLE_EXTENSION}']))

    if should_build_server_launcher:
        export_layouts.append(exp.ExportLayoutConfig(output_path=output_path / f'{ctx.project_name}ServerPackage',
                                                     project_file_patterns=project_file_patterns_to_copy + server_project_file_patterns_to_copy,
                                                     ignore_file_patterns=[f'*.GameLauncher{exp.EXECUTABLE_EXTENSION}',
                                                                           f'*.HeadlessServerLauncher{exp.EXECUTABLE_EXTENSION}',
                                                                           f'*.UnifiedLauncher{exp.EXECUTABLE_EXTENSION}']))

    if should_build_unified_launcher:
        export_layouts.append(exp.ExportLayoutConfig(output_path=output_path / f'{ctx.project_name}UnifiedPackage',
                                                     project_file_patterns=project_file_patterns_to_copy + game_project_file_patterns_to_copy + server_project_file_patterns_to_copy,
                                                     ignore_file_patterns=[f'*.ServerLauncher{exp.EXECUTABLE_EXTENSION}',
                                                                           f'*.HeadlessServerLauncher{exp.EXECUTABLE_EXTENSION}',
                                                                           f'*.GameLauncher{exp.EXECUTABLE_EXTENSION}']))

    if should_build_headless_server_launcher:
        export_layouts.append(exp.ExportLayoutConfig(output_path=output_path / f'{ctx.project_name}HeadlessServerPackage',
                                                     project_file_patterns=project_file_patterns_to_copy + server_project_file_patterns_to_copy,
                                                     ignore_file_patterns=[f'*.ServerLauncher{exp.EXECUTABLE_EXTENSION}',
                                                                           f'*.GameLauncher{exp.EXECUTABLE_EXTENSION}',
                                                                           f'*.UnifiedLauncher{exp.EXECUTABLE_EXTENSION}']))

    # Generate the layouts and archive the packages based on the desired launcher types
    for export_layout in export_layouts:
        exp.setup_launcher_layout_directory(project_path=ctx.project_path,
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
        parser.add_argument('-cfg', '--config', type=str, default='profile', choices=['release', 'profile'],
                            help='The CMake build configuration to use when building project binaries.  Tool binaries built with this script will always be built with the profile configuration.')
        parser.add_argument('-a', '--archive-output',  type=str,
                            help="Option to create a compressed archive the output. "
                                 "Specify the format of archive to create from the output directory. If 'none' specified, no archiving will occur.",
                            choices=["none", "zip", "gzip", "bz2", "xz"], default="none")
        parser.add_argument('-assets', '--should-build-assets', default=False, action='store_true',
                            help='Toggles building all assets for the bundle.')
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
                            help='Designates where the build files for the O3DE toolchain are generated. If not specified, default is <o3de_project_path>/build/tools.')
        parser.add_argument('-lbp', '--launcher-build-path', type=pathlib.Path, default=None,
                            help="Designates where the launcher build files (Game/Server/Unified) are generated. If not specified, default is <o3de_project_path>/build/launcher.")
        parser.add_argument('-regovr', '--allow-registry-overrides', default=False, type = bool,
                            help="When configuring cmake builds, this determines if the script allows for overriding registry settings from external sources.")
        parser.add_argument('-abp', '--asset-bundling-path', type=pathlib.Path, default=None,
                            help="Designates where the artifacts from the asset bundling process will be written to before creation of the package. If not specified, default is <o3de_project_path>/build/asset_bundling.")
        parser.add_argument('-maxsize', '--max-bundle-size', type=int, default=2048, help='Specify the maximum size of a given asset bundle.')
        parser.add_argument('-nogame', '--no-game-launcher', action='store_true', help='This flag skips building the Game Launcher on a platform if not needed.')
        parser.add_argument('-noserver', '--no-server-launcher', action='store_true', help='This flag skips building the Server Launcher on a platform if not needed.')
        parser.add_argument('-noheadless', '--no-headless-server-launcher', action='store_true', help='This flag skips building the Headless Server Launcher on a platform if not needed.')
        parser.add_argument('-nounified', '--no-unified-launcher', action='store_true', help='This flag skips building the Unified Launcher on a platform if not needed.')
        parser.add_argument('-pl', '--platform', type=str, default=exp.get_default_asset_platform(), choices=['pc', 'linux', 'mac'])
        parser.add_argument('-ec', '--engine-centric', action='store_true', default=False, help='Option use the engine-centric work flow to export the project.')
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
        export_standalone_project(ctx=o3de_context,
                                  selected_platform=args.platform,
                                  output_path=args.output_path,
                                  should_build_tools=args.build_tools,
                                  build_config=args.config,
                                  seedlist_paths=[] if not args.seedlist_paths else args.seedlist_paths,
                                  game_project_file_patterns_to_copy=[] if not args.game_project_file_patterns_to_copy else args.game_project_file_patterns_to_copy,
                                  server_project_file_patterns_to_copy=[] if not args.server_project_file_patterns_to_copy else args.server_project_file_patterns_to_copy,
                                  project_file_patterns_to_copy=[] if not args.project_file_patterns_to_copy else args.project_file_patterns_to_copy,
                                  asset_bundling_path=args.asset_bundling_path,
                                  max_bundle_size=args.max_bundle_size,
                                  should_build_all_assets=args.should_build_assets,
                                  fail_on_asset_errors=args.fail_on_asset_errors,
                                  should_build_game_launcher=not args.no_game_launcher,
                                  should_build_server_launcher=not args.no_server_launcher,
                                  should_build_unified_launcher=not args.no_unified_launcher,
                                  should_build_headless_server_launcher=not args.no_headless_server_launcher,
                                  engine_centric=args.engine_centric,
                                  allow_registry_overrides=args.allow_registry_overrides,
                                  tools_build_path=args.tools_build_path,
                                  launcher_build_path=args.launcher_build_path,
                                  archive_output_format=args.archive_output,
                                  logger=o3de_logger)
    except exp.ExportProjectError as err:
        print(err)
        sys.exit(1)
