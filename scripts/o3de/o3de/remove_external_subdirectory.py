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
"""
Contains command to add a gem to a project's cmake scripts
"""

import argparse
import logging
import pathlib

from o3de import manifest

logger = logging.getLogger()
logging.basicConfig()

def remove_external_subdirectory(external_subdir: str or pathlib.Path,
                                 engine_path: str or pathlib.Path = None) -> int:
    """
    remove external subdirectory from cmake
    :param external_subdir: external subdirectory to add to cmake
    :param engine_path: optional engine path, defaults to this engine
    :return: 0 for success or non 0 failure code
    """
    json_data = manifest.load_o3de_manifest()
    engine_object = manifest.find_engine_data(json_data, engine_path)
    if not engine_object:
        logger.error(f'Remove External Subdirectory Failed: {engine_path} not registered.')
        return 1

    external_subdir = pathlib.Path(external_subdir).resolve()
    while external_subdir.as_posix() in engine_object['external_subdirectories']:
        engine_object['external_subdirectories'].remove(external_subdir.as_posix())

    manifest.save_o3de_manifest(json_data)

    return 0


def _run_remove_external_subdirectory(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    return remove_external_subdirectory(args.external_subdirectory)


def add_args(parser, subparsers) -> None:
    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python register.py register --gem-path "C:/TestGem"
    OR
    o3de.py can downloadable commands by importing engine_template,
    call add_args and execute: python o3de.py register --gem-path "C:/TestGem"
    :param parser: the caller instantiates a parser and passes it in here
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    remove_external_subdirectory_subparser = subparsers.add_parser('remove-external-subdirectory')
    remove_external_subdirectory_subparser.add_argument('external_subdirectory', metavar='external_subdirectory',
                                                        type=str,
                                                        help='remove external subdirectory from cmake')

    remove_external_subdirectory_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                                        help='By default the home folder is the user folder, override it to this folder.')

    remove_external_subdirectory_subparser.set_defaults(func=_run_remove_external_subdirectory)
