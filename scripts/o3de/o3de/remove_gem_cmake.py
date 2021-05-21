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
Contains methods for removing a gem from a project's cmake scripts
"""

import argparse
import logging
import pathlib

from o3de import manifest, remove_external_subdirectory

logger = logging.getLogger()
logging.basicConfig()

def remove_gem_from_cmake(gem_name: str = None,
                          gem_path: str or pathlib.Path = None,
                          engine_name: str = None,
                          engine_path: str or pathlib.Path = None) -> int:
    """
    remove a gem to cmake as an external subdirectory
    :param gem_name: name of the gem to remove from cmake
    :param gem_path: the path of the gem to add to cmake
    :param engine_name: optional name of the engine to remove from cmake
    :param engine_path: the path of the engine to remove external subdirectory from, defaults to this engine
    :return: 0 for success or non 0 failure code
    """
    if not gem_name and not gem_path:
        logger.error('Must specify either a Gem name or Gem Path.')
        return 1

    if gem_name and not gem_path:
        gem_path = manifest.get_registered(gem_name=gem_name)

    if not gem_path:
        logger.error(f'Gem Path {gem_path} has not been registered.')
        return 1

    if not engine_name and not engine_path:
        engine_path = manifest.get_this_engine_path()

    if engine_name and not engine_path:
        engine_path = manifest.get_registered(engine_name=engine_name)

    if not engine_path:
        logger.error(f'Engine Path {engine_path} is not registered.')
        return 1

    return remove_external_subdirectory.remove_external_subdirectory(external_subdir=gem_path, engine_path=engine_path)


def _run_remove_gem_from_cmake(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    return remove_gem_from_cmake(args.gem_name, args.gem_path)


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
    # convenience functions to disambiguate the gem name -> gem_path and call remove-external-subdirectory on gem_path
    remove_gem_from_cmake_subparser = subparsers.add_parser('remove-gem-from-cmake')
    group = remove_gem_from_cmake_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-gp', '--gem-path', type=str, required=False,
                       help='The path to the gem.')
    group.add_argument('-gn', '--gem-name', type=str, required=False,
                       help='The name of the gem.')

    remove_gem_from_cmake_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                                 help='By default the home folder is the user folder, override it to this folder.')

    remove_gem_from_cmake_subparser.set_defaults(func=_run_remove_gem_from_cmake)
