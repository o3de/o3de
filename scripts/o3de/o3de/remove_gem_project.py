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
Contains methods for removing a gem from a project
"""

import argparse
import logging
import os
import pathlib
import sys

from o3de import cmake, manifest

logger = logging.getLogger()
logging.basicConfig()


def remove_gem_dependency(cmake_file: pathlib.Path,
                          gem_name: str) -> int:
    """
    removes a gem dependency from a cmake file
    :param cmake_file: path to the cmake file
    :param gem_name: name of the gem
    :return: 0 for success or non 0 failure code
    """
    if not cmake_file.is_file():
        logger.error(f'Failed to locate cmake file {cmake_file}')
        return 1

    # on a line by basis, remove any line with {gem_name}
    t_data = []
    # Remove the gem from the enabled_gem file by skipping the gem name entry
    removed = False
    with open(cmake_file, 'r') as s:
        for line in s:
            if gem_name == line.strip():
                removed = True
            else:
                t_data.append(line)

    if not removed:
        logger.error(f'Failed to remove {gem_name} from cmake file {cmake_file}')
        return 1

    # write the cmake
    with open(cmake_file, 'w') as s:
        s.writelines(t_data)

    return 0


def remove_gem_from_project(gem_name: str = None,
                            gem_path: pathlib.Path = None,
                            project_name: str = None,
                            project_path: pathlib.Path = None,
                            enabled_gem_file: pathlib.Path = None,
                            platforms: str = 'Common') -> int:
    """
    remove a gem from a project
    :param gem_name: name of the gem to add
    :param gem_path: path to the gem to add
    :param project_name: name of the project to add the gem to
    :param project_path: path to the project to add the gem to
    :param enabled_gem_file: File to remove enabled gem from
    :param platforms: str to specify common or which specific platforms
    :return: 0 for success or non 0 failure code
    """

    # we need either a project name or path
    if not project_name and not project_path:
        logger.error(f'Must either specify a Project path or Project Name.')
        return 1

    # if project name resolve it into a path
    if project_name and not project_path:
        project_path = manifest.get_registered(project_name=project_name)
    if not project_path:
        logger.error(f'Unable to locate project path from the registered manifest.json files:'
                     f' {str(pathlib.Path.home() / ".o3de/manifest.json")}, engine.json')
        return 1

    project_path = pathlib.Path(project_path).resolve()
    if not project_path.is_dir():
        logger.error(f'Project path {project_path} is not a folder.')
        return 1

    # We need either a gem name or path
    if not gem_name and not gem_path:
        logger.error(f'Must either specify a Gem path or Gem Name.')
        return 1

    # if gem name resolve it into a path
    if gem_name and not gem_path:
        gem_path = manifest.get_registered(gem_name=gem_name)
    if not gem_path:
        logger.error(f'Unable to locate gem path from the registered manifest.json files:'
                     f' {str(pathlib.Path.home() / ".o3de/manifest.json")},'
                     f' {project_path / "project.json"}, engine.json')
        return 1
    gem_path = pathlib.Path(gem_path).resolve()
    # make sure this gem already exists if we're adding.  We can always remove a gem.
    if not gem_path.is_dir():
        logger.error(f'Gem Path {gem_path} does not exist.')
        return 1


    # Read gem.json from the gem path
    gem_json_data = manifest.get_gem_json_data(gem_path=gem_path)
    if not gem_json_data:
        logger.error(f'Could not read gem.json content under {gem_path}.')
        return 1

    # when removing we will try to do as much as possible even with failures so ret_val will be the last error code
    ret_val = 0

    # if the user has specified the dependencies file then ignore the runtime_dependency and tool_dependency flags
    if enabled_gem_file:
        # make sure this is a project has an enabled_gem file
        if not enabled_gem_file.is_file():
            logger.error(f'Enabled gem file {enabled_gem_file} is not present.')
            return 1
        # remove the dependency
        error_code = remove_gem_dependency(dependencies_file, gem_json_data['gem_name'])
        if error_code:
            ret_val = error_code
    else:
        if ',' in platforms:
            platforms = platforms.split(',')
        else:
            platforms = [platforms]
        for platform in platforms:
                # make sure this is a project has a enabled_gem.cmake file
            project_enabled_gem_file = cmake.get_enabled_gem_cmake_file(project_path=project_path, platform=platform)
            if not project_enabled_gem_file.is_file():
                logger.error(f'Enabled gem file {project_enabled_gem_file} is not present.')
            else:
                # remove the dependency
                error_code = remove_gem_dependency(project_enabled_gem_file, gem_json_data['gem_name'])
                if error_code:
                    ret_val = error_code


    return ret_val


def _run_remove_gem_from_project(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    return remove_gem_from_project(args.gem_name,
                                   args.gem_path,
                                   args.project_name,
                                   args.project_path,
                                   args.enabled_gem_file,
                                   args.platforms)


def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python remove_gem_project.py --project-path D:/Test --gem-name Atom
    :param parser: the caller passes an argparse parser like instance to this method
    """
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                       help='The path to the project.')
    group.add_argument('-pn', '--project-name', type=str, required=False,
                       help='The name of the project.')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-gp', '--gem-path', type=pathlib.Path, required=False,
                       help='The path to the gem.')
    group.add_argument('-gn', '--gem-name', type=str, required=False,
                       help='The name of the gem.')
    parser.add_argument('-egf', '--enabled-gem-file', type=pathlib.Path, required=False,
                                      help='The cmake enabled gem file in which gem dependencies are to be removed from.'
                                           'If not specified it will assume ')
    parser.add_argument('-pl', '--platforms', type=str, required=False,
                                      default='Common',
                                      help='Optional list of platforms this gem should be removed from'
                                           ' Ex. --platforms Mac,Windows,Linux')

    parser.add_argument('-ohf', '--override-home-folder', type=pathlib.Path, required=False,
                                      help='By default the home folder is the user folder, override it to this folder.')

    parser.set_defaults(func=_run_remove_gem_from_project)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py remove-gem-from-project --project-path D:/Test --gem-name Atom
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    remove_gem_project_subparser = subparsers.add_parser('remove-gem-from-project')
    add_parser_args(remove_gem_project_subparser)


def main():
    """
    Runs remove_gem_project.py script as standalone script
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
