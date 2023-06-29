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
import sys
import time

from o3de.export_project import process_command
from o3de import manifest

project_json_data = manifest.get_project_json_data(project_path=o3de_context.project_path)
project_name = project_json_data.get('project_name')
host_asset_platform = exp.determine_host_asset_platform()
parser = argparse.ArgumentParser(prog='Exporter for standalone builds',
                                 description = "Exports a project as standalone to the desired output directory with release layout. "
                                                "In order to use this script, the engine and project must be setup and registered beforehand. ")
parser.add_argument('-out', '--output-path', type=pathlib.Path, required=True, help='Path that describes the final resulting Release Directory path location.')
parser.add_argument('-cfg', '--config', type=str, default='profile', choices=['release', 'profile'],
                        help='The CMake build configuration to use when building project binaries. If tool binaries are built with this script, they will use profile mode.')
parser.add_argument('-aof', '--archive-output-format',
                        type=str,
                        help="Format of archive to create from the output directory",
                        choices=["zip", "gzip", "bz2", "xz"], default="zip")
parser.add_argument('-sl', '--seedlist', type=pathlib.Path, dest='seedlist_paths', action='append',
                        help='Path to a seed list file for asset bundling. Specify multiple times for each seed list.')
parser.add_argument('-gf', '--gem-folder-name-to-copy', type=str, dest='gem_folder_names_to_copy', action='append',
                        help="The name of a Gem folder to include in release. Only specify folder name. This should match what is in the project directory.")
parser.add_argument('-pfp', '--project-file_pattern-to-copy', type=str, dest='project_file_patterns_to_copy', action='append',
                        help="Any additional file patterns located in the project directory. File patterns will be relative to the project path.")
parser.add_argument('-bnmt', '--build-non-mono-tools', action='store_true')
parser.add_argument('-nmbp', '--non-mono-build-path', type=pathlib.Path, default=None)
parser.add_argument('-mbp', '--mono-build-path', type=pathlib.Path, default=None)
parser.add_argument('-nogame', '--no-game-launcher', action='store_true', help='this flag skips building the Game Launcher on a platform if not needed.')
parser.add_argument('-noserver', '--no-server-launcher', action='store_true', help='this flag skips building the Server Launcher on a platform if not needed.')
parser.add_argument('-nounified', '--no-unified-launcher', action='store_true', help='this flag skips building the Unified Launcher on a platform if not needed.')
parser.add_argument('-pl', '--platform', type=str, default=host_asset_platform, choices=['pc', 'linux', 'mac'])
parser.add_argument('-a', '--archive-output', action='store_true', help='This option places the final output of the build into a compressed archive')
parser.add_argument('-q', '--quiet', action='store_true', help='Suppresses logging information unless an error occurs.')

args = parser.parse_args(o3de_context.args)

if args.quiet:
    o3de_logger.setLevel(logging.ERROR)

non_mono_build_path = (o3de_context.engine_path) / 'build' / 'non_mono' if args.non_mono_build_path is None else args.non_mono_build_path
mono_build_path     = (o3de_context.engine_path) / 'build' / 'mono' if args.mono_build_path is None else args.mono_build_path

selected_platform = args.platform

if not selected_platform:
    o3de_logger.error("Unable to identify default host platform! Please supply the platform using '--platform'. Options are [pc, linux, mac].")
    sys.exit(1)

o3de_logger.info(f"Selected Project path: {o3de_context.project_path}")
o3de_logger.info(f"Selected Engine path : {o3de_context.engine_path}")
o3de_logger.info(f"Build path for non-monolithic executables: {args.non_mono_build_path}")
o3de_logger.info(f"Build path for monolithic executables: {args.mono_build_path}")

output_cache_path = args.output_path / 'Cache' / selected_platform
os.makedirs(output_cache_path, exist_ok=True)

# Build project assets and the engine (non-monolithic)
if args.build_non_mono_tools:
    process_command(['cmake', '-S', o3de_context.engine_path, '-B', str(non_mono_build_path), '-DLY_MONOLITHIC_GAME=0', f'-DLY_PROJECTS={o3de_context.project_path}'],
                    cwd=o3de_context.engine_path)
    process_command(['cmake', '--build', str(non_mono_build_path), '--target', 'AssetBundler', 'AssetBundlerBatch', 'AssetProcessor', 'AssetProcessorBatch', '--config','profile'],
                    cwd=o3de_context.engine_path)
    process_command(['cmake', '--build', str(non_mono_build_path), '--target', f'{project_name}.Assets', '--config', 'profile'],
                    cwd=o3de_context.engine_path)

# Build monolithic game
process_command(['cmake', '-S', '.', '-B', str(mono_build_path), '-DLY_MONOLITHIC_GAME=1', '-DALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES=0', f'-DLY_PROJECTS={o3de_context.project_path}'],
                cwd=o3de_context.engine_path)
    
if not args.no_game_launcher:
    process_command(['cmake', '--build', str(mono_build_path), '--target', f'{project_name}.GameLauncher', '--config', args.config],
                    cwd=o3de_context.engine_path) 

if not args.no_server_launcher:
    process_command(['cmake', '--build', str(mono_build_path), '--target', f'{project_name}.ServerLauncher', '--config', args.config],
                    cwd=o3de_context.engine_path)

if not args.no_unified_launcher:
    process_command(['cmake', '--build', str(mono_build_path), '--target', f'{project_name}.UnifiedLauncher', '--config', args.config],
                    cwd=o3de_context.engine_path)

# Before bundling content, make sure that the necessary executables exist
asset_bundler_batch_path = non_mono_build_path / 'bin' / 'profile' / ('AssetBundlerBatch' + ('.exe' if host_asset_platform=='pc' else ''))
if not asset_bundler_batch_path.is_file():
    o3de_logger.error(f"AssetBundlerBatch not found at path '{asset_bundler_batch_path}'. In order to bundle the data for project, this executable must be present!")
    o3de_logger.error("To correct this issue, do 1 of the following: "
                    "1) Use the --build-non-mono-tools flag in the CLI parameters"
                    "2) If you are trying to build in a project-centric fashion, please switch to engine-centric for this export script"
                    f"3) Build AssetBundlerBatch by hand and make sure it is available at {asset_bundler_batch_path}"
                    "4) Set the --non-mono-build-path to point at a directory which contains this executable")
    sys.exit(1)

# Bundle content
engine_asset_list_path = o3de_context.project_path / 'AssetBundling' /  'AssetLists' / f'engine_{selected_platform}.assetlist'

process_command([asset_bundler_batch_path, 'assetLists','--addDefaultSeedListFiles', '--assetListFile', engine_asset_list_path, '--project-path', o3de_context.project_path, '--allowOverwrites' ],
                cwd=o3de_context.engine_path)


game_asset_list_path = o3de_context.project_path /'AssetBundling'/'AssetLists'/ f'game_{selected_platform}.assetlist'

game_asset_list_command = [asset_bundler_batch_path, 'assetLists', '--assetListFile', game_asset_list_path]

for seedlist_path in args.seedlist_paths:
    game_asset_list_command += ['--seedListFile', seedlist_path]

game_asset_list_command += ['--project-path', o3de_context.project_path, '--allowOverwrites']

process_command(game_asset_list_command, cwd=o3de_context.engine_path)

engine_bundle_path = output_cache_path / f'engine_{selected_platform}.pak'
process_command([asset_bundler_batch_path, 'bundles', '--assetListFile', engine_asset_list_path, '--outputBundlePath', engine_bundle_path, '--project-path', o3de_context.project_path, '--allowOverwrites'],
                cwd=o3de_context.engine_path)

# This is to prevent any accidental file locking mechanism from failing subsequent bundling operations
# See https://github.com/o3de/o3de/issues/16167
time.sleep(1)

game_bundle_path = output_cache_path / f'game_{selected_platform}.pak'
process_command([asset_bundler_batch_path, 'bundles', '--assetListFile', game_asset_list_path, '--outputBundlePath', game_bundle_path, '--project-path', o3de_context.project_path, '--allowOverwrites'],
                cwd=o3de_context.engine_path)

# Create Launcher Layout Directory
import shutil

for file in glob.glob(str(pathlib.PurePath(mono_build_path / 'bin' / args.config / '*.*'))):
    shutil.copy(file, args.output_path)

for gem_folder_name in args.gem_folder_names_to_copy:
    output_gem_path = args.output_path / 'Gems'/ gem_folder_name
    os.makedirs(output_gem_path, exist_ok=True)
    for file in glob.glob(str(pathlib.PurePath(mono_build_path / 'bin' / args.config / 'Gems' / gem_folder_name / '*.*'))):
        shutil.copy(file, output_gem_path)

for project_file_pattern in args.project_file_patterns_to_copy:
    for file in glob.glob(str(pathlib.PurePath(o3de_context.project_path / project_file_pattern))):
        shutil.copy(file, args.output_path)

# Optionally zip the layout directory if the user requests
if args.archive_output:
    archive_name = args.output_path
    o3de_logger.info("Archiving output directory (this may take a while)...")
    shutil.make_archive(args.output_path, args.archive_output_format, root_dir = args.output_path)

o3de_logger.info(f"Exporting project is complete! Release Directory can be found at {args.output_path}")
