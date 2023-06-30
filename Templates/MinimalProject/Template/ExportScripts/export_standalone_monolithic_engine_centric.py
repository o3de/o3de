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
import o3de.export_project as exp
import pathlib
import platform
import shutil
import sys
import time

from o3de.export_project import process_command
from o3de import manifest
from typing import List

# This script is meant to be run via o3de cli export-project command, not from Python directly!
# In order to use this script, the command should look like the following (on windows use o3de.bat, on linux use o3de.sh):

# cd path\to\o3de\engine
# .\scripts\o3de.bat  export-project --export-scripts path\to\o3de\ExportScripts\export_standalone_monolithic_engine_centric.py \
# --project-path path\to\project --log-level INFO --output-path path\to\output --build-non-mono-tools --config release --archive-output zip \
# --seedlist path\to\seedlist1.seed --seedlist path\to\seedlist2.seed <etc..> 

# The following parameters are optional, but may be helpful:
#   seedlist: an O3DE seedlist for describing what assets of the project should be bundled
#   gem-folder-name-to-copy: A Gem Folder from <ProjectRoot>/Gems that should be copied to the output location of the exported project
#                               Files found in these folders will go into <output-path>/Gems/GemFolderName
#   project-file-pattern-to-copy: Any regex pattern starting from <ProjectRoot> that indicates what files should be copied to output location.
#                                   Files found in these patterns will go into <output-path>

# For more information on the available parameters for this script, check the parse_arguments function defined at the bottom of the file

def build_non_monolithic_tools_and_assets(engine_path: pathlib.Path,
                                          project_path: pathlib.Path,
                                          project_name: str,
                                          non_mono_build_path: pathlib.Path):
    process_command(['cmake', '-S', engine_path, '-B', str(non_mono_build_path), 
                        '-DLY_DISABLE_TEST_MODULES=TRUE', '-DLY_STRIP_DEBUG_SYMBOLS=TRUE', f'-DLY_PROJECTS={project_path}'],
                        cwd=engine_path)
    process_command(['cmake', '--build', str(non_mono_build_path), '--target', 'AssetBundlerBatch', 'AssetProcessorBatch', '--config','profile'],
                    cwd=engine_path)
    process_command(['cmake', '--build', str(non_mono_build_path), '--target', f'{project_name}.Assets', '--config', 'profile'],
                    cwd=engine_path)

def build_monolithic_project(engine_path: pathlib.Path,
                             project_path: pathlib.Path,
                             project_name: str,
                             build_config: str,
                             mono_build_path: pathlib.Path,
                             should_build_game: bool = True,
                             should_build_server:bool = True,
                             should_build_unified:bool = True):
    process_command(['cmake', '-S', '.', '-B', str(mono_build_path), '-DLY_MONOLITHIC_GAME=1', '-DALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES=0', f'-DLY_PROJECTS={project_path}'],
                    cwd=engine_path)
    if should_build_game:
        process_command(['cmake', '--build', str(mono_build_path), '--target', f'{project_name}.GameLauncher', '--config', build_config],
                        cwd=engine_path) 
    if should_build_server:
        process_command(['cmake', '--build', str(mono_build_path), '--target', f'{project_name}.ServerLauncher', '--config', build_config],
                        cwd=engine_path)
    if should_build_unified:
        process_command(['cmake', '--build', str(mono_build_path), '--target', f'{project_name}.UnifiedLauncher', '--config', build_config],
                        cwd=engine_path)

def bundle_project_and_engine_assets(engine_path: pathlib.Path,
                          project_path: pathlib.Path,
                          asset_bundler_batch_path: pathlib.Path,
                          seedlist_paths: List[pathlib.Path],
                          output_cache_path: pathlib.Path,
                          platform: str):
    engine_asset_list_path = project_path / 'AssetBundling' /  'AssetLists' / f'engine_{platform}.assetlist'

    process_command([asset_bundler_batch_path, 'assetLists','--addDefaultSeedListFiles', '--assetListFile', engine_asset_list_path, '--project-path', project_path, '--allowOverwrites' ],
                    cwd=engine_path)


    game_asset_list_path = project_path /'AssetBundling'/'AssetLists'/ f'game_{platform}.assetlist'

    game_asset_list_command = [asset_bundler_batch_path, 'assetLists', '--assetListFile', game_asset_list_path]

    for sp in seedlist_paths:
        game_asset_list_command += ['--seedListFile', sp]

    game_asset_list_command += ['--project-path', project_path, '--allowOverwrites']

    process_command(game_asset_list_command, cwd=engine_path)

    engine_bundle_path = output_cache_path / f'engine_{platform}.pak'
    process_command([asset_bundler_batch_path, 'bundles', '--assetListFile', engine_asset_list_path, '--outputBundlePath', engine_bundle_path, '--project-path', project_path, '--allowOverwrites'],
                    cwd=engine_path)

    # This is to prevent any accidental file locking mechanism from failing subsequent bundling operations
    # See https://github.com/o3de/o3de/issues/16167
    time.sleep(1)

    game_bundle_path = output_cache_path / f'game_{platform}.pak'
    process_command([asset_bundler_batch_path, 'bundles', '--assetListFile', game_asset_list_path, '--outputBundlePath', game_bundle_path, '--project-path', project_path, '--allowOverwrites'],
                    cwd=engine_path)

def create_launcher_layout_directory(project_path: pathlib.Path,
                                     output_path: pathlib.Path,
                                     mono_build_path: pathlib.Path,
                                     gem_folder_names_to_copy: List[str],
                                     project_file_patterns_to_copy: List[str],
                                     build_config: str):
    for file in glob.glob(str(pathlib.PurePath(mono_build_path / 'bin' / build_config / '*.*'))):
        shutil.copy(file, output_path)

    for gem_folder_name in gem_folder_names_to_copy:
        output_gem_path = output_path / 'Gems'/ gem_folder_name
        os.makedirs(output_gem_path, exist_ok=True)
        for file in glob.glob(str(pathlib.PurePath(mono_build_path / 'bin' / build_config / 'Gems' / gem_folder_name / '*.*'))):
            shutil.copy(file, output_gem_path)

    for project_file_pattern in project_file_patterns_to_copy:
        for file in glob.glob(str(pathlib.PurePath(project_path / project_file_pattern))):
            shutil.copy(file, output_path)


def export_standalone_monolithic_engine_centric(engine_path: pathlib.Path,
                                                project_path: pathlib.Path,
                                                selected_platform: str,
                                                output_path: pathlib.Path,
                                                should_build_non_mono_tools: bool,
                                                build_config: str,
                                                seedlist_paths: List[pathlib.Path],
                                                gem_folder_names_to_copy: List[str],
                                                project_file_patterns_to_copy: List[str],
                                                should_build_game_launcher: bool = True,
                                                should_build_server_launcher: bool = True,
                                                should_build_unified_launcher: bool = True,
                                                non_mono_build_path: pathlib.Path|None =None,
                                                mono_build_path: pathlib.Path|None =None,
                                                archive_output_format: str = "none",
                                                logger: logging.Logger|None =None):
    if not logger:
        logger = logging.getLogger()
        logger.setLevel(logging.ERROR)

    project_json_data = manifest.get_project_json_data(project_path=project_path)
    project_name = project_json_data.get('project_name')
    
    if not non_mono_build_path:
        non_mono_build_path = (engine_path) / 'build' / 'non_mono'
    if not mono_build_path:
        mono_build_path = (engine_path) / 'build' / 'mono' 

    logger.info(f"Selected Project path                     : {project_path}")
    logger.info(f"Selected Engine path                      : {engine_path}")
    logger.info(f"Build path for non-monolithic executables : {non_mono_build_path}")
    logger.info(f"Build path for monolithic executables     : {mono_build_path}")

    output_cache_path = output_path / 'Cache' / selected_platform
    os.makedirs(output_cache_path, exist_ok=True)

    if should_build_non_mono_tools:
        build_non_monolithic_tools_and_assets(engine_path, project_path, project_name, non_mono_build_path)
        
    build_monolithic_project(engine_path, project_path, 
                             project_name, 
                             build_config, 
                             mono_build_path, 
                             should_build_game_launcher,
                             should_build_server_launcher,
                             should_build_unified_launcher)

    # Before bundling content, make sure that the necessary executables exist
    asset_bundler_batch_path = non_mono_build_path / 'bin' / 'profile' / ('AssetBundlerBatch' + ('.exe' if platform.system().lower()=='windows' else ''))
    if not asset_bundler_batch_path.is_file():
        logger.error(f"AssetBundlerBatch not found at path '{asset_bundler_batch_path}'. In order to bundle the data for project, this executable must be present!")
        logger.error("To correct this issue, do 1 of the following: \n"
                        "1) Use the --build-non-mono-tools flag in the CLI parameters\n"
                        "2) If you are trying to build in a project-centric fashion, please switch to engine-centric for this export script\n"
                        f"3) Build AssetBundlerBatch by hand and make sure it is available at {asset_bundler_batch_path}\n"
                        "4) Set the --non-mono-build-path to point at a directory which contains this executable\n")
        sys.exit(1)

    logger.info("Begin bundling assets...")
    bundle_project_and_engine_assets(engine_path, 
                                     project_path, 
                                     asset_bundler_batch_path, 
                                     seedlist_paths, 
                                     output_cache_path, 
                                     selected_platform)

    logger.info("Copy artifacts to launcher layout directory...")
    create_launcher_layout_directory(project_path, 
                                     output_path, 
                                     mono_build_path, 
                                     gem_folder_names_to_copy, 
                                     project_file_patterns_to_copy, 
                                     build_config)
    
    # Optionally compress the layout directory into an archive if the user requests
    if archive_output_format != "none":
        logger.info("Archiving output directory (this may take a while)...")
        shutil.make_archive(output_path, archive_output_format, root_dir = output_path)

    logger.info(f"Exporting project is complete! Release Directory can be found at {output_path}")

#This code is only run by the 'export-project' O3DE CLI command
if "o3de_context" in globals():
    def parse_arguments(unprocessed_args: List[str]):
        parser = argparse.ArgumentParser(prog='Exporter for standalone builds',
                                        description = "Exports a project as standalone to the desired output directory with release layout. "
                                                        "In order to use this script, the engine and project must be setup and registered beforehand. ")
        parser.add_argument('-out', '--output-path', type=pathlib.Path, required=True, help='Path that describes the final resulting Release Directory path location.')
        parser.add_argument('-cfg', '--config', type=str, default='profile', choices=['release', 'profile'],
                                help='The CMake build configuration to use when building project binaries. If tool binaries are built with this script, they will use profile mode.')
        parser.add_argument('-a', '--archive-output',
                                type=str,
                                help="Option to create a compressed archive the output. "
                                "Specify the format of archive to create from the output directory. If 'none' specified, no archiving will occur.",
                                choices=["none", "zip", "gzip", "bz2", "xz"], default="none")
        parser.add_argument('-sl', '--seedlist', type=pathlib.Path, dest='seedlist_paths', action='append',
                                help='Path to a seed list file for asset bundling. Specify multiple times for each seed list.')
        parser.add_argument('-gf', '--gem-folder-name-to-copy', type=str, dest='gem_folder_names_to_copy', action='append',
                                help="The name of a Gem folder to include in release. Only specify folder name."
                                    "This should match what is in the project directory.")
        parser.add_argument('-pfp', '--project-file-pattern-to-copy', type=str, dest='project_file_patterns_to_copy', action='append',
                                help="Any additional file patterns located in the project directory. File patterns will be relative to the project path.")
        parser.add_argument('-bnmt', '--build-non-mono-tools', action='store_true',
                            help="Specifies whether to build O3DE toolchain executables. This will build AssetBundlerBatch, AssetProcessorBatch.")
        parser.add_argument('-nmbp', '--non-mono-build-path', type=pathlib.Path, default=None,
                            help='Designates where O3DE toolchain executables go. If not specified, default is <o3de_engine_path>/build/non_mono.')
        parser.add_argument('-mbp', '--mono-build-path', type=pathlib.Path, default=None,
                            help="Designates where project executables (like Game/Server Launcher) go."
                            " If not specified, default is <o3de_engine_path>/build/mono.")
        parser.add_argument('-nogame', '--no-game-launcher', action='store_true', help='This flag skips building the Game Launcher on a platform if not needed.')
        parser.add_argument('-noserver', '--no-server-launcher', action='store_true', help='This flag skips building the Server Launcher on a platform if not needed.')
        parser.add_argument('-nounified', '--no-unified-launcher', action='store_true', help='This flag skips building the Unified Launcher on a platform if not needed.')
        parser.add_argument('-pl', '--platform', type=str, default=exp.get_default_asset_platform(), choices=['pc', 'linux', 'mac'])
        parser.add_argument('-q', '--quiet', action='store_true', help='Suppresses logging information unless an error occurs.')

        return parser.parse_args(unprocessed_args)
    
    args = parse_arguments(o3de_context.args)
    if args.quiet:
        o3de_logger.setLevel(logging.ERROR)
    export_standalone_monolithic_engine_centric(o3de_context.engine_path,
                                                o3de_context.project_path,
                                                args.platform,
                                                args.output_path,
                                                args.build_non_mono_tools,
                                                args.config,
                                                args.seedlist_paths,
                                                args.gem_folder_names_to_copy,
                                                args.project_file_patterns_to_copy,
                                                not args.no_game_launcher,
                                                not args.no_server_launcher,
                                                not args.no_unified_launcher,
                                                args.non_mono_build_path,
                                                args.mono_build_path, 
                                                args.archive_output,
                                                o3de_logger)
