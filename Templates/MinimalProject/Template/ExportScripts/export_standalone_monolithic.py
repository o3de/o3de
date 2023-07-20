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
import psutil

from o3de.export_project import process_command
from o3de import manifest
from typing import List

# This script is meant to be run via o3de cli export-project command, not from Python directly!
# This export script operates in a project-centric manner, meaning all build artifacts are centered around the project folder. 
# In order to use this script, the command should look like the following (on windows use o3de.bat, on linux use o3de.sh):

# Example invocation on Linux
'''
$ cd <project-path>
$ <engine-path>/scripts/o3de.sh export-project --export-scripts ExportScripts/export_standalone_monolithic.py \
 --project-path . --log-level INFO --output-path <install-path> --build-non-mono-tools --config release --archive-output xz \
 --seedlist ./Assets/seedlist1.seed --seedlist ./Assets/seedlist2.seed <etc..> 
'''

# Example invocation on Windows
'''
$ cd <project-path>
$ <engine-path>\scripts\o3de.bat  export-project --export-scripts ExportScripts\export_standalone_monolithic_project_centric.py \
 --project-path . --log-level INFO --output-path <install-path> --build-non-mono-tools --config release --archive-output zip \
 --seedlist .\Assets\seedlist1.seed --seedlist .\Assets\seedlist2.seed <etc..> 
''' 

# The following parameters are optional, but may be helpful:
#   seedlist: an O3DE seedlist for describing what assets of the project should be bundled
#   project-file-pattern-to-copy: Any regex pattern starting from <ProjectRoot> that indicates what files should be copied to output location.
#                                   Files found in these patterns will go into <output-path>
#                                   Variants for game/server launcher only are game-file-pattern-to-copy/server-file-pattern-to-copy resp.

# For more information on the available parameters for this script, check the parse_arguments function defined at the bottom of the file


def build_non_monolithic_code_modules( project_path: pathlib.Path,
                                        non_mono_build_path: pathlib.Path,
                                        force_rebuild_all = False):
    #build the pre_requisite tools if needed
    os.makedirs(non_mono_build_path, exist_ok=True)

    bundler_path = get_asset_bundler_batch_path(non_mono_build_path)
    processor_path = get_asset_processor_batch_path(non_mono_build_path)

    process_command(["cmake", "-B", non_mono_build_path, "-S", project_path])

    if not processor_path.exists() or force_rebuild_all:
        process_command(["cmake", "--build", non_mono_build_path, "--target", "AssetProcessorBatch", "--config", "profile"])
    if not bundler_path.exists() or force_rebuild_all:
        process_command(["cmake", "--build", non_mono_build_path, "--target", "AssetBundlerBatch", "--config", "profile"])

def build_monolithic_code_modules( project_path: pathlib.Path,
                        project_name: str,
                        build_config: str,
                        mono_build_path: pathlib.Path,
                        should_build_game_launcher: bool,
                        should_build_server_launcher: bool,
                        should_build_unified_launcher: bool,
                        allow_registry_overrides: bool):
    
    process_command(["cmake", "-B", mono_build_path, "-S", project_path, "-DLY_MONOLITHIC_GAME=1",
                     f"-DALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES={'0' if not allow_registry_overrides else '1'}"])

    os.makedirs(mono_build_path, exist_ok=True)
    if should_build_server_launcher:

        process_command(["cmake", "--build", mono_build_path, "--target", f"{project_name}.ServerLauncher", "--config", build_config, "--", "/m"])

    if should_build_game_launcher:
        process_command(["cmake", "--build", mono_build_path, "--target", f"{project_name}.GameLauncher", "--config", build_config, "--", "/m"])
    
    if should_build_unified_launcher:
        process_command(["cmake", "--build", mono_build_path, "--target", f"{project_name}.UnifiedLauncher", "--config", build_config, "--", "/m"])


def kill_existing_processes(project_name: str):
    o3de_process_names = ['o3de', 'editor', 'assetprocessor', f'{project_name.lower()}.serverlauncher', f'{project_name.lower()}.gamelauncher' ]
    for process in psutil.process_iter():
        processes_to_kill = []
        # strip off .exe and check process name
        if os.path.splitext(process.name())[0].lower() in o3de_process_names:
            processes_to_kill.append(process)
        
        if processes_to_kill:
            logging.error(f"The following processes are still running: {', '.join([process.name() for process in processes_to_kill])}")
            user_input = input(f'Continuing may cause build errors.\nStop them before continuing? (y/n). Quit(q)')
            if user_input.lower() == 'y':
                for p in processes_to_kill:
                    p.terminate()
                    p.wait()
            elif user_input.lower() == 'q':
                quit()


def process_assets( project_name: str,
                    project_path: pathlib.Path,
                    non_mono_build_path: pathlib.Path):
    build_non_monolithic_code_modules(project_path, non_mono_build_path)
    process_command(["cmake", "--build", non_mono_build_path, "--target", f"{project_name}.Assets", "--config", "profile"])

def get_asset_processor_batch_path(non_mono_build_path: pathlib.Path):
    return non_mono_build_path / 'bin/profile' / ('AssetProcessorBatch' + ('.exe' if platform.system().lower()=='windows' else ''))

def get_asset_bundler_batch_path(non_mono_build_path: pathlib.Path):
    return non_mono_build_path / 'bin/profile' / ('AssetBundlerBatch' + ('.exe' if platform.system().lower()=='windows' else ''))

def bundle_assets(project_path: pathlib.Path,
                  selected_platform: str,
                  seedlist_paths: List[str],
                  non_mono_build_path: pathlib.Path,
                  custom_asset_list_path: pathlib.Path|None = None,
                  max_bundle_size: int = 2048):
    asset_bundler_batch_path = get_asset_bundler_batch_path(non_mono_build_path)
    asset_list_path = (project_path / 'AssetBundling/AssetLists') if not custom_asset_list_path else custom_asset_list_path

    game_asset_list_path = asset_list_path /  f'game_{selected_platform}.assetlist'
    engine_asset_list_path = asset_list_path / f'engine_{selected_platform}.assetlist'
    bundles_path = project_path / 'AssetBundling/Bundles'

    #generate the asset lists for the project
    gen_game_asset_list_command = [
        asset_bundler_batch_path,
        'assetLists',
        '--assetListFile',
        game_asset_list_path,
        '--platform',
        selected_platform,
        '--allowOverwrites']
 
    for seed in seedlist_paths:
        gen_game_asset_list_command.append("--seedListFile")
        gen_game_asset_list_command.append(str(seed))
    process_command(gen_game_asset_list_command)

    gen_engine_asset_list_command = [
        asset_bundler_batch_path,
        "assetLists",
        "--assetListFile",
        engine_asset_list_path,
        "--platform",
        selected_platform,
        "--allowOverwrites",
        "--addDefaultSeedListFiles"
    ]
    process_command(gen_engine_asset_list_command)

    #generate the bundles. We will place it in the project directory for now, since the files need to be copied multiple times (one for each separate launcher distribution)
    process_command([asset_bundler_batch_path, "bundles", "--maxSize", str(max_bundle_size), "--platform", selected_platform, "--allowOverwrites",
                         "--outputBundlePath", bundles_path / f"game_{selected_platform}.pak",
                         "--assetListFile", game_asset_list_path])
    
    process_command([asset_bundler_batch_path, "bundles", "--maxSize", str(max_bundle_size), "--platform", selected_platform, "--allowOverwrites",
                         "--outputBundlePath", bundles_path / f"engine_{selected_platform}.pak",
                         "--assetListFile", engine_asset_list_path])
    return bundles_path

def setup_launcher_layout_directory(project_path: pathlib.Path,
                                     output_path: pathlib.Path,
                                     platform: str,
                                     mono_build_path:pathlib.Path,
                                     build_config: str,
                                     bundles_to_copy: List[pathlib.Path],
                                     project_file_patterns_to_copy: List[str],
                                     archive_output_format: str = "none",
                                     logger: logging.Logger|None = None):
    
    if output_path.exists():
        shutil.rmtree(output_path)
    
    output_cache_path = output_path / 'Cache' / platform
        
    os.makedirs(output_cache_path, exist_ok=True)

    for bundle in bundles_to_copy:
        shutil.copy(bundle, output_cache_path)

    for file in glob.glob(str(mono_build_path/f'bin/{build_config}/*')):
        file_path = pathlib.Path(file)
        if file_path.is_dir():
            shutil.copytree(file, output_path / file_path.name, dirs_exist_ok=True)
        else:
            shutil.copy(file, output_path)

    for project_file_pattern in project_file_patterns_to_copy:
        for file in glob.glob(str(pathlib.PurePath(project_path / project_file_pattern))):
            shutil.copy(file, output_path)
    
    # Optionally compress the layout directory into an archive if the user requests
    if archive_output_format != "none":
        if logger:
            logger.info(f"Archiving output directory {output_path} (this may take a while)...")
        shutil.make_archive(output_path, archive_output_format, root_dir = output_path)


def export_standalone_monolithic_project_centric(project_path: pathlib.Path,
                                                selected_platform: str,
                                                output_path: pathlib.Path,
                                                should_build_non_mono_tools: bool,
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
                                                non_mono_build_path: pathlib.Path|None =None,
                                                mono_build_path: pathlib.Path|None =None,
                                                archive_output_format: str = "none",
                                                logger: logging.Logger|None = None):
    if not logger:
        logger = logging.getLogger()
        logger.setLevel(logging.ERROR)
    
    project_json_data = manifest.get_project_json_data(project_path= o3de_context.project_path)
    project_name = project_json_data.get('project_name')

    if not non_mono_build_path:
        non_mono_build_path = (project_path) / 'build/non_mono'
    if not mono_build_path:
        mono_build_path = (project_path) / 'build/mono' 

    kill_existing_processes(project_name)

    if should_build_all_code:
        if should_build_non_mono_tools:
            build_non_monolithic_code_modules(project_path, non_mono_build_path, force_rebuild_all=True)
        
        build_tools = [get_asset_bundler_batch_path(non_mono_build_path),get_asset_processor_batch_path(non_mono_build_path)]
        tools_missing = [b for b in build_tools if not b.exists()]
        if len(tools_missing) > 0:
            logger.error(f"Necessary Build Tools have not been created! The following are missing: {', '.join(tools_missing)}")
            logger.error("Please ensure that these tools exist before proceeding on with the build!")
            quit()

        build_monolithic_code_modules(project_path, project_name, build_config, mono_build_path,
                                      should_build_game_launcher, should_build_server_launcher, should_build_unified_launcher,
                                      allow_registry_overrides)
    
    expected_bundles_path = None
    if should_build_all_assets:
        process_assets(project_name, project_path, non_mono_build_path)

        expected_bundles_path = bundle_assets(project_path,
                                            selected_platform,
                                            seedlist_paths,
                                            non_mono_build_path,
                                            preferred_asset_list_path,
                                            max_bundle_size)
    else:
        expected_bundles_path = project_path / "AssetBundling/Bundles"    
    for should_do, expected_output_dir, project_file_patterns in zip(
                                              [should_build_game_launcher, should_build_server_launcher, should_build_unified_launcher],
                                              [output_path / f'{project_name}GamePackage',
                                               output_path / f'{project_name}ServerPackage',
                                               output_path / f'{project_name}UnifiedPackage'],
                                              [project_file_patterns_to_copy + game_project_file_patterns_to_copy,
                                               project_file_patterns_to_copy + server_project_file_patterns_to_copy,
                                               project_file_patterns_to_copy + game_project_file_patterns_to_copy + server_project_file_patterns_to_copy]):
        if should_do:
            setup_launcher_layout_directory(project_path,
                                         expected_output_dir,
                                         selected_platform,
                                         mono_build_path,
                                         build_config,
                                         [expected_bundles_path / f'game_{selected_platform}.pak', expected_bundles_path / f'engine_{selected_platform}.pak'],
                                         project_file_patterns,
                                         archive_output_format,
                                         logger)
    



#This code is only run by the 'export-project' O3DE CLI command
if "o3de_context" in globals():
    def parse_args(unprocessed_args: List[str]):
        parser = argparse.ArgumentParser(
                    prog='Exporter for standalone builds',
                    description="Exports a project as standalone to the desired output directory with release layout. "
                                "In order to use this script, the engine and project must be setup and registered beforehand. ")
        parser.add_argument('-out', '--output-path', type=pathlib.Path, required=True, help='Path that describes the final resulting Release Directory path location.')
        parser.add_argument('-cfg', '--config', type=str, default='profile', choices=['release', 'profile'],
                                help='The CMake build configuration to use when building project binaries.  Tool binaries built with this script will always be built with the profile configuration.')
        parser.add_argument('-a', '--archive-output',
                                type=str,
                                help="Option to create a compressed archive the output. "
                                "Specify the format of archive to create from the output directory. If 'none' specified, no archiving will occur.",
                                choices=["none", "zip", "gzip", "bz2", "xz"], default="none")
        parser.add_argument('-code', '--should-build-code', default=False,action='store_true', help='Toggles building all code for the project by launcher type (game, server, unified).')
        parser.add_argument('-assets', '--should-build-assets', default=False, action='store_true', help='Toggles building all assets for the project by launcher type (game, server, unified).')
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
    export_standalone_monolithic_project_centric(o3de_context.project_path,
                                                args.platform,
                                                args.output_path,
                                                args.build_non_mono_tools,
                                                args.config,
                                                [] if not args.seedlist_paths else args.seedlist_paths,
                                                [] if not args.game_project_file_patterns_to_copy else args.game_project_file_patterns_to_copy,
                                                [] if not args.server_project_file_patterns_to_copy else args.server_project_file_patterns_to_copy,
                                                [] if not args.project_file_patterns_to_copy else args.project_file_patterns_to_copy,
                                                args.preferred_asset_list_path,
                                                args.max_bundle_size,
                                                args.should_build_code,
                                                args.should_build_assets,
                                                not args.no_game_launcher,
                                                not args.no_server_launcher,
                                                not args.no_unified_launcher,
                                                args.allow_registry_overrides,
                                                args.non_mono_build_path,
                                                args.mono_build_path, 
                                                args.archive_output,
                                                o3de_logger)
