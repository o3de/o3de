#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import argparse
import datetime
import logging
import pathlib
import platform
import sys
import os
import subprocess

ROOT_DEV_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..'))
if ROOT_DEV_PATH not in sys.path:
    sys.path.append(ROOT_DEV_PATH)

from cmake.Tools import common

# The location of this python script is not portable relative to the engine root, we determine the engine root based
# on its relative location
DEV_ROOT = os.path.realpath(os.path.join(__file__, '../../..'))

BOOTSTRAP_CFG = os.path.join(DEV_ROOT, 'bootstrap.cfg')

EXECUTABLE_EXTN = '.exe' if platform.system() == 'Windows' else ''
RC_NAME = f'rc{EXECUTABLE_EXTN}'
APB_NAME = f'AssetProcessorBatch{EXECUTABLE_EXTN}'

# Depending on the user request for verbosity, the argument list to subprocess may or may not redirect stdout to NULL
VERBOSE_CALL_ARGS = dict(
    shell=True,
    cwd=DEV_ROOT
)
NON_VERBOSE_CALL_ARGS = dict(
    **VERBOSE_CALL_ARGS,
    stdout=subprocess.DEVNULL
)


def command_arg(arg):
    """
    Work-around for an issue when running subprocess on Linux: subprocess.check_call will take in the argument as an array
    but only invokes the first item in the array, ignoring the arguments. As quick fix, we will combine the array into the
    full command line and execute it that way on non-windows platforms
    """
    if platform.system() == 'Windows':
        return arg
    else:
        return ' '.join(arg)


def validate(binfolder, game_name, pak_script):

    #
    # Validate the binfolder is relative and contains 'rc' and 'AssetProcessorBatch'
    #
    if os.path.isabs(binfolder):
        raise common.LmbrCmdError("Invalid value for '-b/--binfolder'. It must be a path relative to the engine root folder",
                                  common.ERROR_CODE_ERROR_DIRECTORY)

    binfolder_abs_path = pathlib.Path(DEV_ROOT) / binfolder
    if not binfolder_abs_path.is_dir():
        raise common.LmbrCmdError("Invalid value for '-b/--binfolder'. Path does not exist or is not a directory",
                                  common.ERROR_CODE_ERROR_DIRECTORY)

    rc_check = binfolder_abs_path / RC_NAME
    if not rc_check.is_file():
        raise common.LmbrCmdError(f"Invalid value for '-b/--binfolder'. Path does not contain {RC_NAME}",
                                  common.ERROR_CODE_ERROR_DIRECTORY)

    apb_check = binfolder_abs_path / APB_NAME
    if not apb_check.is_file():
        raise common.LmbrCmdError(f"Invalid value for '-b/--binfolder'. Path does not contain {APB_NAME}",
                                  common.ERROR_CODE_ERROR_DIRECTORY)

    #
    # Validate the game name represents a game project within the game engine
    #
    gamefolder_abs_path = pathlib.Path(DEV_ROOT) / game_name
    if not gamefolder_abs_path.is_dir():
        raise common.LmbrCmdError(f"Invalid value for '-g/--game-name'. No game '{game_name} exists.",
                                  common.ERROR_CODE_ERROR_DIRECTORY)

    project_json_path = gamefolder_abs_path / 'project.json'
    if not project_json_path.is_file():
        raise common.LmbrCmdError(
            f"Invalid value for '-g/--game-name'. Folder '{game_name} is not a valid game project.",
            common.ERROR_CODE_FILE_NOT_FOUND)

    if not os.path.isfile(pak_script):
        raise common.LmbrCmdError(f'Pak script file {pak_script} does not exist.',
                                  common.ERROR_CODE_FILE_NOT_FOUND)


def process(binfolder, game_name, asset_platform, autorun_assetprocessor, recompress, fastest_compression, target,
            pak_script, warn_on_assetprocessor_error, verbose):

    logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.DEBUG if verbose else logging.INFO)

    target_path_root_abs = pathlib.Path(DEV_ROOT) / target
    if target_path_root_abs.is_file():
        raise common.LmbrCmdError(f"Target '{target}' already exists as a file.",
                                  common.ERROR_CODE_GENERAL_ERROR)
    os.makedirs(target_path_root_abs.absolute(), exist_ok=True)

    target_pak_folder_name = f'{game_name.lower()}_{asset_platform}_paks'
    target_pak = target_path_root_abs / target_pak_folder_name

    # Prepare the asset processor batch arguments and execute if requested
    if autorun_assetprocessor:
        ap_executable = os.path.join(binfolder, APB_NAME)
        ap_cmd_args = [ap_executable,
                       f'/gamefolder={game_name}',
                       f'/platforms={asset_platform}']
        logging.debug("Calling {}".format(' '.join(ap_cmd_args)))
        try:
            logging.info(f"Running {APB_NAME} on {game_name}")
            start_time = datetime.datetime.now()

            call_args = VERBOSE_CALL_ARGS if verbose else NON_VERBOSE_CALL_ARGS

            subprocess.check_call(command_arg(ap_cmd_args),
                                  **call_args)

            total_time = datetime.datetime.now() - start_time
            logging.info(f"Asset Processing Complete. Elapse: {total_time}")
        except subprocess.CalledProcessError:
            if warn_on_assetprocessor_error:
                logging.warning('AssetProcessorBatch reported errors')
            else:
                raise common.LmbrCmdError("AssetProcessorBatch has one or more failed assets.",
                                          common.ERROR_CODE_GENERAL_ERROR)

    rc_executable = os.path.join(binfolder, RC_NAME)
    rc_cmd_args = [rc_executable,
                   f'/job={pak_script}',
                   f'/p={asset_platform}',
                   f'/game={game_name}',
                   f'/trg={target_pak}']
    if recompress:
        rc_cmd_args.append('/recompress=1')
    if fastest_compression:
        rc_cmd_args.append('/use_fastest=1')
    logging.debug("Calling {}".format(' '.join(rc_cmd_args)))

    try:
        logging.info(f"Running {APB_NAME} on {game_name}")
        start_time = datetime.datetime.now()

        call_args = VERBOSE_CALL_ARGS if verbose else NON_VERBOSE_CALL_ARGS

        subprocess.check_call(command_arg(rc_cmd_args),
                              **call_args)

        total_time = datetime.datetime.now() - start_time
        logging.info(f"Asset Processing Complete. Elapse: {total_time}")
        logging.info(f"Pak files for {game_name} written to {target_pak}")

    except subprocess.CalledProcessError as err:
        raise common.LmbrCmdError(f"{RC_NAME} returned an error: {str(err)}.",
                                  err.returncode)


def main(args):

    parser = argparse.ArgumentParser()

    parser.add_argument('-b', '--binfolder',
                        help='The relative location of the binary folder that contains the resource compiler and asset processor')

    bootstrap = common.get_bootstrap_values(DEV_ROOT, ['sys_game_folder'])
    parser.add_argument('-g', '--game-name',
                        help='The name of the Game whose asset pak will be generated for',
                        default=bootstrap.get('sys_game_folder'))

    parser.add_argument('-p', '--asset-platform',
                        help='The asset platform type to process')

    parser.add_argument('-a', '--autorun-assetprocessor',
                        help='Option to automatically invoke asset processor batch on the game before generating the pak',
                        action='store_true')

    parser.add_argument('-w', '--warn-on-assetprocessor-error',
                        help='When -a/--autorun-assetprocessor is specified, warn on asset processor failure rather than aborting the process',
                        action='store_true')

    parser.add_argument('-r', '--recompress',
                        action='store_true',
                        help='If present, the ResourceCompiler (RC.exe) will decompress and compress back each PAK file '
                             'found as they are transferred from the cache folder to the game_pc_pak folder.')
    parser.add_argument('-fc', '--fastest-compression',
                        action='store_true',
                        help='As each file is being added to its PAK file, they will be compressed across all available '
                             'codecs (ZLIB, ZSTD and LZ4) and the one with the fastest decompression time will be '
                             'chosen. The default is to always use ZLIB')
    parser.add_argument('--target',
                        default='Pak',
                        help='Specify a target folder for the pak files. (Default : Pak)')
    parser.add_argument('--pak-script',
                        default=f'{DEV_ROOT}/{os.path.normpath("Code/Tools/RC/Config/rc/RCJob_Generic_MakePaks.xml")}',
                        help="The absolute path of the pak script configuration file to use to create the paks.")

    parser.add_argument('-v', '--verbose',
                        help='Enable debug messages',
                        action='store_true')

    parsed = parser.parse_args(args)

    validate(binfolder=parsed.binfolder,
             game_name=parsed.game_name,
             pak_script=parsed.pak_script)

    process(binfolder=parsed.binfolder,
            game_name=parsed.game_name,
            asset_platform=parsed.asset_platform,
            autorun_assetprocessor=parsed.autorun_assetprocessor,
            recompress=parsed.recompress,
            fastest_compression=parsed.fastest_compression,
            target=parsed.target,
            pak_script=parsed.pak_script,
            warn_on_assetprocessor_error=parsed.warn_on_assetprocessor_error,
            verbose=parsed.verbose)


if __name__ == '__main__':
    try:
        if not os.path.isfile(BOOTSTRAP_CFG):
            raise common.LmbrCmdError("Invalid dev root, missing bootstrap.cfg.",
                                      common.ERROR_CODE_FILE_NOT_FOUND)

        main(sys.argv[1:])
        exit(0)

    except common.LmbrCmdError as err:
        print(str(err), file=sys.stderr)
        exit(err.code)
