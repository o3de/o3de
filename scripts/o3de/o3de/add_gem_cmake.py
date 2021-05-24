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
import sys

from o3de import add_external_subdirectory, manifest, validation

logger = logging.getLogger()
logging.basicConfig()

def add_gem_to_cmake(gem_name: str = None,
                     gem_path: str or pathlib.Path = None,
                     engine_name: str = None,
                     engine_path: str or pathlib.Path = None) -> int:
    """
    add a gem to a cmake as an external subdirectory for an engine
    :param gem_name: name of the gem to add to cmake
    :param gem_path: the path of the gem to add to cmake
    :param engine_name: name of the engine to add to cmake
    :param engine_path: the path of the engine to add external subdirectory to, default to this engine
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

    gem_path = pathlib.Path(gem_path).resolve()
    gem_json = gem_path / 'gem.json'
    if not gem_json.is_file():
        logger.error(f'Gem json {gem_json} is not present.')
        return 1
    if not validation.valid_o3de_gem_json(gem_json):
        logger.error(f'Gem json {gem_json} is not valid.')
        return 1

    if not engine_name and not engine_path:
        engine_path = manifest.get_this_engine_path()

    if engine_name and not engine_path:
        engine_path = manifest.get_registered(engine_name=engine_name)

    if not engine_path:
        logger.error(f'Engine Path {engine_path} has not been registered.')
        return 1

    engine_json = engine_path / 'engine.json'
    if not engine_json.is_file():
        logger.error(f'Engine json {engine_json} is not present.')
        return 1
    if not validation.valid_o3de_engine_json(engine_json):
        logger.error(f'Engine json {engine_json} is not valid.')
        return 1

    return add_external_subdirectory.add_external_subdirectory(external_subdir=gem_path, engine_path=engine_path)

def _run_add_gem_to_cmake(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    return add_gem_to_cmake(gem_name=args.gem_name, gem_path=args.gem_path)


def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python add_gem_cmake.py --gem-path "/path/to/gem"
    :param parser: the caller passes an argparse parser like instance to this method
    """
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-gp', '--gem-path', type=str, required=False,
                       help='The path to the gem.')
    group.add_argument('-gn', '--gem-name', type=str, required=False,
                       help='The name of the gem.')

    parser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                            help='By default the home folder is the user folder, override it to this folder.')

    parser.set_defaults(func=_run_add_gem_to_cmake)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py add-gem-to-cmake --gem-path "/path/to/gem"
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    add_gem_cmake_subparser = subparsers.add_parser('add-gem-to-cmake')
    add_parser_args(add_gem_cmake_subparser)


def main():
    """
    Runs add_gem_cmake.py script as standalone script
    """
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers

    # add args to the parser
    add_parser_args(the_parser)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1

    # return
    sys.exit(ret)


if __name__ == "__main__":
    main()
