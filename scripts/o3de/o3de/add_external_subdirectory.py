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
Contains command to add an external_subdirectory to a project's cmake scripts
"""

import argparse
import logging
import pathlib
import sys

from o3de import manifest

logger = logging.getLogger()
logging.basicConfig()

def add_external_subdirectory(external_subdir: str or pathlib.Path,
                              engine_path: str or pathlib.Path = None) -> int:
    """
    add external subdirectory to a cmake
    :param external_subdir: external subdirectory to add to cmake
    :param engine_path: optional engine path, defaults to this engine
    :return: 0 for success or non 0 failure code
    """
    external_subdir = pathlib.Path(external_subdir).resolve()
    if not external_subdir.is_dir():
        logger.error(f'Add External Subdirectory Failed: {external_subdir} does not exist.')
        return 1

    external_subdir_cmake = external_subdir / 'CMakeLists.txt'
    if not external_subdir_cmake.is_file():
        logger.error(f'Add External Subdirectory Failed: {external_subdir} does not contain a CMakeLists.txt.')
        return 1

    json_data = manifest.load_o3de_manifest()
    engine_object = manifest.find_engine_data(json_data, engine_path)
    if not engine_object:
        logger.error(f'Add External Subdirectory Failed: {engine_path} not registered.')
        return 1

    engine_object.setdefault('external_subdirectories', [])
    while external_subdir.as_posix() in engine_object['external_subdirectories']:
        engine_object['external_subdirectories'].remove(external_subdir.as_posix())

    def parse_cmake_file(cmake: str or pathlib.Path,
                         files: set):
        cmake_path = pathlib.Path(cmake).resolve()
        cmake_file = cmake_path
        if cmake_path.is_dir():
            files.add(cmake_path)
            cmake_file = cmake_path / 'CMakeLists.txt'
        elif cmake_path.is_file():
            cmake_path = cmake_path.parent
        else:
            return

        with cmake_file.open('r') as s:
            lines = s.readlines()
            for line in lines:
                line = line.strip()
                start = line.find('include(')
                if start == 0:
                    end = line.find(')', start)
                    if end > start + len('include('):
                        try:
                            include_cmake_file = pathlib.Path(engine_path / line[start + len('include('): end]).resolve()
                        except FileNotFoundError as e:
                            pass
                        else:
                            parse_cmake_file(include_cmake_file, files)
                else:
                    start = line.find('add_subdirectory(')
                    if start == 0:
                        end = line.find(')', start)
                        if end > start + len('add_subdirectory('):
                            try:
                                include_cmake_file = pathlib.Path(
                                    cmake_path / line[start + len('add_subdirectory('): end]).resolve()
                            except FileNotFoundError as e:
                                pass
                            else:
                                parse_cmake_file(include_cmake_file, files)

    cmake_files = set()
    parse_cmake_file(engine_path, cmake_files)
    for external in engine_object["external_subdirectories"]:
        parse_cmake_file(external, cmake_files)

    if external_subdir in cmake_files:
        manifest.save_o3de_manifest(json_data)
        logger.warning(f'External subdirectory {external_subdir.as_posix()} already included by add_subdirectory().')
        return 1

    engine_object['external_subdirectories'].insert(0, external_subdir.as_posix())
    engine_object['external_subdirectories'] = sorted(engine_object['external_subdirectories'])

    manifest.save_o3de_manifest(json_data)

    return 0


def _run_add_external_subdirectory(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    return add_external_subdirectory(args.external_subdirectory)


def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python add-external-subdirectory.py "/home/foo/external-subdir"
    :param parser: the caller passes an argparse parser like instance to this method
    """
    parser.add_argument('external_subdirectory', metavar='external_subdirectory', type=str,
                                                     help='add an external subdirectory to cmake')

    parser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                                     help='By default the home folder is the user folder, override it to this folder.')

    parser.set_defaults(func=_run_add_external_subdirectory)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py add_external_subdirectory "/home/foo/external-subdir"
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    add_external_subdirectory_subparser = subparsers.add_parser('add-external-subdirectory')
    add_parser_args(add_external_subdirectory_subparser)


def main():
    """
    Runs add_external_subdirectory.py script as standalone script
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
