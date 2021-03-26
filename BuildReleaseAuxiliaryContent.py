"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Config file for pytest & PythonTestTools. Warns user if the packaged Python is not
being used.
"""
import argparse
import os
import re
import subprocess


def printMessage(message):
    print(message)


def getCurrentProject():
    bootstrap = open("bootstrap.cfg", "r")

    gameProjectRegex = re.compile(r"^project_path\s*=\s*(.*)")

    for line in bootstrap:
        gameFolderMatch = gameProjectRegex.match(line)
        if gameFolderMatch:
            return os.path.basename(gameFolderMatch.group(1))
    return None


def getCommandLineArgs():
    # Game project override is not taken because it's generally not safe to process assets for the non-active project. DLLs may be out of date.
    parser = argparse.ArgumentParser(
        description='Lumberyard game project release build automation tool, generates auxiliary content for the current game project specified in bootstrap.cfg'
                    'or --project override if provided.')
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
    parser.add_argument('--project', help='If present, project to use instead of the bootstrap project')
    return parser.parse_args()


def subprocessWithPrint(command):
    printMessage(command)
    subprocess.check_call(command)


def main():
    args = getCommandLineArgs()

    gameProject = args.project or getCurrentProject()
    if not gameProject:
        printMessage(
            "Game project could not be read from bootstrap.cfg, please verify it is set correctly and run this again.")
        return
    printMessage("Generating auxiliary content for project {} from build folder {} for platforms {}".format(gameProject,
                                                                                                            args.buildFolder,
                                                                                                            args.platforms))
    # Build paths to tools
    apBatchExe = os.path.join(args.buildFolder, "AssetProcessorBatch.exe")
    rcFolder = args.buildFolder
    rcExe = os.path.join(rcFolder, "rc.exe")

    rc_script_name = 'RCJob_Generic_MakeAuxiliaryContent.xml'
    rc_script_path = os.path.join(rcFolder, rc_script_name)
    this_script_folder = os.path.dirname(os.path.abspath(__file__))
    if not os.path.exists(rc_script_path):
        rc_script_path = os.path.join(this_script_folder, 'Code', 'Tools', 'rc', 'Config', 'rc',
                                      rc_script_name)
    if not os.path.exists(rc_script_path):
        printMessage(f"Could not find {rc_script_name} at {rcFolder} or {this_script_folder}")
        return

    # Make sure all assets are up to date.
    if not args.skipAssetProcessing:
        printMessage("Updating game assets")
        # Asset processor will read the current project from bootstrap.
        subprocessWithPrint("{} /platforms={}".format(apBatchExe, args.platforms))
    else:
        printMessage("Skipping asset processing")

    platformsSplit = args.platforms.split(',')
    for platform in platformsSplit:
        printMessage("Generating auxiliary content for platform {}".format(platform))
        # Call RC to generate the auxiliary content.
        recompressArg = 1 if args.recompress else 0
        useFastestArg = 1 if args.use_fastest else 0
        skiplevelPaksArg = 1 if args.skiplevelPaks else 0
        rc_cmd = f"{rcExe} /job={rc_script_path} /p={platform} /game={gameProject.lower()} " \
                 f"/recompress={recompressArg}" \
                 f" /use_fastest={useFastestArg} /skiplevelPaks={skiplevelPaksArg}"

        subprocessWithPrint(rc_cmd)


if __name__ == "__main__":
    main()
