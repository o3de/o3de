"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Config file for pytest & PythonTestTools. Warns user if the packaged Python is not
being used.
"""
import argparse
import os
import pathlib
import subprocess

# Move upwards twice from the <engine-root>/scripts/bundler directory to get the parent directory
SCRIPT_PATH = pathlib.Path(__file__).resolve()
DEFAULT_ENGINE_ROOT = SCRIPT_PATH.parents[2]

def printMessage(message):
    print(message)


def getCommandLineArgs():
    parser = argparse.ArgumentParser(
        description='O3DE project release build automation tool, generates auxiliary content for the project specified on the command line.')
    parser.add_argument('--platforms', required=True,
                        help='Required: The comma separated platform list to generate auxiliary content for.')
    parser.add_argument('--buildFolder', required=True,
                        help='Required: The build folder for your engine, containing AssetProcessorBatch.exe')
    parser.add_argument('--recompress', action='store_true',
                        help='If present, the ResourceCompiler (RC.exe) will decompress and compress back each PAK file found as they are transferred from the cache folder to the game_pc_pak folder.')
    parser.add_argument('--use_fastest', action='store_true',
                        help='As each file is being added to its PAK file, they will be compressed across all available codecs (ZLIB, ZSTD and LZ4) and the one with the fastest decompression time will be chosen. The default is to always use ZLIB.')
    parser.add_argument('--skipAssetProcessing', action='store_true',
                        help='If present, assetprocessorbatch will not be launched before creating the auxiliary content for the current game project specified in the bootstrap.cfg file. The default behavior of processing assets first helps you make sure your auxiliary content will always be generated from the most up to date data for your game.')
    parser.add_argument('--skiplevelPaks', action='store_true',
                        help='If present, the ResourceCompiler (RC.exe) will skip putting any level related pak files for e.g level.pak and terraintexture.pak in the auxiliary contents. Consider using this option if your levels are being packaged through some other system, such as the AssetBundler.')
    parser.add_argument('--project-path', required=True, type=pathlib.Path,
                        help='Required: specifies the path to the o3de project to use')
    parser.add_argument('--engine-path', type=pathlib.Path, default=DEFAULT_ENGINE_ROOT,
                        help='specifies the path to the engine root to use')
    return parser.parse_args()


def subprocessWithPrint(command):
    printMessage(command)
    subprocess.check_call(command)


def main():
    args = getCommandLineArgs()

    if not args.project_path:
        printMessage(
            "Project-path has not been supplied, it is required to in order to locate a project auxiliary content.")
        return
    printMessage(f"Generating auxiliary content for project at path {str(args.project_path)} from build folder {args.buildFolder} for platforms {args.platforms}")
    # Build paths to tools
    apBatchExe = os.path.join(args.buildFolder, "AssetProcessorBatch.exe")
    rcFolder = args.buildFolder
    rcExe = os.path.join(rcFolder, "rc.exe")

    rc_script_name = 'RCJob_Generic_MakeAuxiliaryContent.xml'
    rc_script_path = os.path.join(rcFolder, rc_script_name)
    if not os.path.exists(rc_script_path):
        rc_script_path = args.engine_path / 'Code' / 'Tools' / 'rc' / 'Config' / 'rc' / rc_script_name
    if not os.path.exists(rc_script_path):
        printMessage(f"Could not find {rc_script_name} at {rcFolder} or {str(args.engine_path)}")
        return

    # Make sure all assets are up to date.
    if not args.skipAssetProcessing:
        printMessage("Updating game assets")
        # Asset processor will read the current project from bootstrap.
        subprocessWithPrint(f"{apBatchExe} --project-path={str(args.project_path)} --platforms={args.platforms}")
    else:
        printMessage("Skipping asset processing")

    platformsSplit = args.platforms.split(',')
    for platform in platformsSplit:
        printMessage("Generating auxiliary content for platform {}".format(platform))
        # Call RC to generate the auxiliary content.
        recompressArg = 1 if args.recompress else 0
        useFastestArg = 1 if args.use_fastest else 0
        skiplevelPaksArg = 1 if args.skiplevelPaks else 0
        rc_cmd = f"{rcExe} --job={rc_script_path} --platform={platform}" \
                 f" --regset=/Amazon/AzCore/Bootstrap/project_path={str(args.project_path)} " \
                 f"--recompress={recompressArg}" \
                 f" --use_fastest={useFastestArg} --skiplevelPaks={skiplevelPaksArg}"

        subprocessWithPrint(rc_cmd)


if __name__ == "__main__":
    main()
