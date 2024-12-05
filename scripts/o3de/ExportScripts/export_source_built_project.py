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
import export_utility as eutil
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
                              should_build_headless_server_launcher: bool = True,
                              monolithic_build: bool = True,
                              allow_registry_overrides: bool = False,
                              tools_build_path: pathlib.Path | None =None,
                              launcher_build_path: pathlib.Path | None =None,
                              archive_output_format: str = "none",
                              fail_on_asset_errors: bool = False,
                              engine_centric: bool = False,
                              kill_o3de_processes_before_running: bool = False,
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

    # If the output path is a relative path, convert it to an absolute path using the project path as the base
    if not output_path.is_absolute():
        output_path = ctx.project_path / str(output_path)

    # Use a provided logger or get the current system one
    if not logger:
        logger = logging.getLogger()
        logger.setLevel(logging.ERROR)

    # Calculate the tools and game build paths
    default_base_path = ctx.engine_path if engine_centric else ctx.project_path
    
    # For installed SDKs, the monolithic option depends on the build configuration that is selected since only profile shared objects and release static libraries
    # are produced. Debug is not allowed.
    if is_installer_sdk:
        if build_config == exp.BUILD_CONFIG_DEBUG:
            raise exp.ExportProjectError("Exporting debug projects not supported with the O3DE SDK.")

        if build_config == exp.BUILD_CONFIG_RELEASE:

            # Check if we have any monolithic entries if building monolithic
            if not exp.has_monolithic_artifacts(ctx):
                logger.error("No monolithic artifacts are detected in the engine installation.")
                raise exp.ExportProjectError("Trying to build monolithic without libraries.")

            logger.info(f"Preparing monolithic build for export.")
            monolithic_build = True

        else:
            logger.info(f"Preparing non-monolithic build for export.")
            monolithic_build = False
    
    if not launcher_build_path:
        launcher_build_path = default_base_path / 'build/launcher'
    elif not launcher_build_path.is_absolute():
        launcher_build_path = default_base_path / launcher_build_path
    
    logger.info(f"Launcher build path set to {launcher_build_path}")
    
    if not asset_bundling_path:
        asset_bundling_path = default_base_path / 'build/asset_bundling'
    elif not asset_bundling_path.is_absolute():
        asset_bundling_path = default_base_path / asset_bundling_path
    
    logger.info(f"Asset bundling path set to {asset_bundling_path}")

    # Resolve (if possible) and validate any provided seedlist files
    validated_seedslist_paths = exp.validate_project_artifact_paths(project_path=ctx.project_path,
                                                                    artifact_paths=seedlist_paths)

    # Preprocess the seed file paths in case there are any wildcard patterns to use
    preprocessed_seedfile_paths = exp.preprocess_seed_path_list(project_path=ctx.project_path,
                                                                paths=seedfile_paths)
    
    # Convert level names into seed files that the asset bundler can utilize for packaging
    eutil.process_level_names(ctx, preprocessed_seedfile_paths, level_names, selected_platform)

    # Make sure there are no running processes for the current project before continuing
    if kill_o3de_processes_before_running:
        exp.kill_existing_processes(ctx.project_name)

    tools_build_path = eutil.handle_tools(ctx,
                                          should_build_tools,
                                          is_installer_sdk,
                                          tool_config,
                                          selected_platform,
                                          tools_build_path,
                                          default_base_path,
                                          engine_centric,
                                          logger)

    # Generate the bundle
    expected_bundles_path = eutil.build_and_bundle_assets(ctx,
                                                          should_build_all_assets,
                                                          tools_build_path,
                                                          is_installer_sdk,
                                                          tool_config, 
                                                          engine_centric,
                                                          fail_on_asset_errors,
                                                          [selected_platform],
                                                          validated_seedslist_paths, 
                                                          preprocessed_seedfile_paths,
                                                          asset_bundling_path,
                                                          max_bundle_size,
                                                          logger)

    # Build the requested game launcher types (if any)
    launcher_type = 0
    if should_build_game_launcher:
        launcher_type |= exp.LauncherType.GAME
    if should_build_server_launcher:
        launcher_type |= exp.LauncherType.SERVER
    if should_build_unified_launcher:
        launcher_type |= exp.LauncherType.UNIFIED
    if should_build_headless_server_launcher:
        launcher_type |= exp.LauncherType.HEADLESS_SERVER

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
                                            project_name=ctx.project_name,
                                            asset_platform=selected_platform,
                                            launcher_build_path=launcher_build_path,
                                            build_config=build_config,
                                            bundles_to_copy=[expected_bundles_path / f'game_{selected_platform}.pak',
                                                             expected_bundles_path / f'engine_{selected_platform}.pak'],
                                            export_layout=export_layout,
                                            archive_output_format=archive_output_format,
                                            logger=logger)
    
    logger.info(f"Project exported to '{output_path}'.")

def export_standalone_parse_args(o3de_context: exp.O3DEScriptExportContext, export_config: command_utils.O3DEConfig):

        parser = argparse.ArgumentParser(
                    prog=f'o3de.py export-project -es {__file__}',
                    description="Exports a project as standalone to the desired output directory with release layout. "
                                "In order to use this script, the engine and project must be setup and registered beforehand. ",
                    epilog=exp.CUSTOM_CMAKE_ARG_HELP_EPILOGUE,
                    formatter_class=argparse.RawTextHelpFormatter,
                    add_help=False
        )
        eutil.create_common_export_params_and_config(parser, export_config)
        
        default_output_path = pathlib.Path(export_config.get_value(key=exp.SETTINGS_DEFAULT_OUTPUT_PATH.key,
                                                                   default=exp.SETTINGS_DEFAULT_OUTPUT_PATH.default))
        if not default_output_path.is_absolute():
            default_output_path = pathlib.Path(o3de_context.project_path) / default_output_path

        parser.add_argument('-out', '--output-path',
                            type=pathlib.Path,
                            required=False,
                            help='Path that describes the final resulting Output Directory path location.',
                            default=default_output_path)
        
        default_archive_output_format = export_config.get_value(key=exp.SETTINGS_ARCHIVE_OUTPUT_FORMAT.key,
                                                                default=exp.SETTINGS_ARCHIVE_OUTPUT_FORMAT.default)
        parser.add_argument('-a', '--archive-output',  type=str,
                            help="Option to create a compressed archive the output. "
                                 "Specify the format of archive to create from the output directory. If 'none' specified, no archiving will occur.",
                            choices=[exp.ARCHIVE_FORMAT_NONE, exp.ARCHIVE_FORMAT_ZIP, exp.ARCHIVE_FORMAT_GZIP, exp.ARCHIVE_FORMAT_BZ2, exp.ARCHIVE_FORMAT_XZ],
                            default=default_archive_output_format)


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

        
        default_launcher_build_path = export_config.get_value(exp.SETTINGS_DEFAULT_LAUNCHER_TOOLS_PATH.key, exp.SETTINGS_DEFAULT_LAUNCHER_TOOLS_PATH.default)
        parser.add_argument('-lbp', '--launcher-build-path', type=pathlib.Path, default=pathlib.Path(default_launcher_build_path),
                            help=f"Designates where the launcher build files (Game/Server/Unified) are generated. If not specified, default is '{default_launcher_build_path}'.")

        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_ALLOW_REGISTRY_OVERRIDES.key,
                                           enable_override_arg=['-regovr', '--allow-registry-overrides'],
                                           enable_override_desc="Allow overriding registry settings from external sources during the cmake build configuration.",
                                           disable_override_arg=['-noregovr', '--disallow-registry-overrides'],
                                           disable_override_desc="Disallow overriding registry settings from external sources during the cmake build configuration.")

        default_asset_bundling_path = export_config.get_value(exp.SETTINGS_DEFAULT_ASSET_BUNDLING_PATH.key, exp.SETTINGS_DEFAULT_ASSET_BUNDLING_PATH.default)
        parser.add_argument('-abp', '--asset-bundling-path', type=pathlib.Path, default=pathlib.Path(default_asset_bundling_path),
                            help=f"Designates where the artifacts from the asset bundling process will be written to before creation of the package. If not specified, default is '{default_asset_bundling_path}'.")

        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_GAME_LAUNCHER.key,
                                           enable_override_arg=['-game', '--game-launcher'],
                                           enable_override_desc="Enable the building and inclusion of the Game Launcher.",
                                           disable_override_arg=['-nogame', '--no-game-launcher'],
                                           disable_override_desc="Disable the building and inclusion of the Game Launcher if not needed.")

        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_SERVER_LAUNCHER.key,
                                           enable_override_arg=['-server', '--server-launcher'],
                                           enable_override_desc="Enable the building and inclusion of the Server Launcher.",
                                           disable_override_arg=['-noserver', '--no-server-launcher'],
                                           disable_override_desc="Disable the building and inclusion of the Server Launcher if not needed.")

        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_HEADLESS_SERVER_LAUNCHER.key,
                                           enable_override_arg=['-headless', '--headless-server-launcher'],
                                           enable_override_desc="Enable the building and inclusion of the Headless Server Launcher.",
                                           disable_override_arg=['--noheadless', '--no-headless-server-launcher'],
                                           disable_override_desc="Disable the building and inclusion of the Headless Server Launcher if not needed.")


        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_UNIFIED_SERVER_LAUNCHER.key,
                                           enable_override_arg=['-unified', '--unified-launcher'],
                                           enable_override_desc="Enable the building and inclusion of the Unified Launcher.",
                                           disable_override_arg=['-nounified', '--no-unified-launcher'],
                                           disable_override_desc="Disable the building and inclusion of the Unified Launcher if not needed.")

        export_config.add_boolean_argument(parser=parser,
                                           key=exp.SETTINGS_OPTION_BUILD_MONOLITHICALLY.key,
                                           enable_override_arg=['-mono', '--monolithic'],
                                           enable_override_desc="Build the launchers monolithically into a launcher executable.",
                                           disable_override_arg=['-nomono', '--non-monolithic'],
                                           disable_override_desc="Build the launchers non-monolithically, i.e. a launcher executable alongside individual modules per Gem.")


        parser.add_argument('-pl', '--platform', type=str, default=exp.get_default_asset_platform(), choices=['pc', 'linux', 'mac'])

        if o3de_context is None:
            parser.print_help()
            exit(0)
        
        parsed_args = parser.parse_args(o3de_context.args)
        if parsed_args.script_help:
            parser.print_help()
            exit(0)

        return parsed_args


def export_standalone_run_command(o3de_context, args, export_config: command_utils.O3DEConfig, o3de_logger):
    option_build_assets = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                  key=exp.SETTINGS_OPTION_BUILD_ASSETS.key,
                                                                  enable_attribute='build_assets',
                                                                  disable_attribute='skip_build_assets')

    option_build_tools = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                 key=exp.SETTINGS_OPTION_BUILD_TOOLS.key,
                                                                 enable_attribute='build_tools',
                                                                 disable_attribute='skip_build_tools')    

    fail_on_asset_errors = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                   key=exp.SETTINGS_OPTION_FAIL_ON_ASSET_ERR.key,
                                                                   enable_attribute='fail_on_asset_errors',
                                                                   disable_attribute='continue_on_asset_errors')

    option_build_game_launcher = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                         key=exp.SETTINGS_OPTION_BUILD_GAME_LAUNCHER.key,
                                                                         enable_attribute='game_launcher',
                                                                         disable_attribute='no_game_launcher')

    option_build_server_launcher = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                           key=exp.SETTINGS_OPTION_BUILD_SERVER_LAUNCHER.key,
                                                                           enable_attribute='server_launcher',
                                                                           disable_attribute='no_server_launcher')

    option_build_headless_server_launcher = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                                    key=exp.SETTINGS_OPTION_BUILD_HEADLESS_SERVER_LAUNCHER.key,
                                                                                    enable_attribute='headless_server_launcher',
                                                                                    disable_attribute='no_headless_server_launcher')

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

    option_build_monolithically = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                          key=exp.SETTINGS_OPTION_BUILD_MONOLITHICALLY.key,
                                                                          enable_attribute='monolithic',
                                                                          disable_attribute='non_monolithic')

    option_kill_prior_processes = export_config.get_parsed_boolean_option(parsed_args=args,
                                                                          key=exp.SETTINGS_OPTION_KILL_PRIOR_PROCESSES.key,
                                                                          enable_attribute='kill_processes',
                                                                          disable_attribute='no_kill_processes')

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
                                  should_build_headless_server_launcher=option_build_headless_server_launcher,
                                  should_build_unified_launcher=option_build_unified_launcher,
                                  engine_centric=option_build_engine_centric,
                                  allow_registry_overrides=option_allow_registry_overrrides,
                                  tools_build_path=args.tools_build_path,
                                  launcher_build_path=args.launcher_build_path,
                                  archive_output_format=args.archive_output,
                                  monolithic_build=option_build_monolithically,
                                  kill_o3de_processes_before_running=option_kill_prior_processes,
                                  logger=o3de_logger)
    except exp.ExportProjectError as err:
        print(err)
        sys.exit(1)

# This code is only run by the 'export-project' O3DE CLI command

if "o3de_context" in globals():
    global o3de_context
    global o3de_logger

    # Resolve the export config
    export_config = None
    try:
        if o3de_context is None:
            project_name, project_path = command_utils.resolve_project_name_and_path()
            export_config = exp.get_export_project_config(project_path=project_path)
        else:
            export_config = exp.get_export_project_config(project_path=o3de_context.project_path)
    except command_utils.O3DEConfigError:
        o3de_logger.debug(f"No project detected at {os.getcwd()}, getting default settings from global config instead.")
        project_name = None
        export_config = exp.get_export_project_config(project_path=None)

    assert export_config
    args = export_standalone_parse_args(o3de_context, export_config)

    export_standalone_run_command(o3de_context, args, export_config, o3de_logger)
    o3de_logger.info(f"Finished exporting.")
    sys.exit(0)
