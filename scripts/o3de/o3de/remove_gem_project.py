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
This file contains methods for removing a gem from a project
"""

import argparse
import logging
import os
import pathlib

from o3de import cmake, remove_gem_cmake

logger = logging.getLogger()
logging.basicConfig()


def remove_gem_dependency(cmake_file: str or pathlib.Path,
                          gem_target: str) -> int:
    """
    removes a gem dependency from a cmake file
    :param cmake_file: path to the cmake file
    :param gem_target: cmake target name
    :return: 0 for success or non 0 failure code
    """
    if not os.path.isfile(cmake_file):
        logger.error(f'Failed to locate cmake file {cmake_file}')
        return 1

    # on a line by basis, remove any line with Gem::{gem_name}
    t_data = []
    # Remove the gem from the cmake_dependencies file by skipping the gem name entry
    removed = False
    with open(cmake_file, 'r') as s:
        for line in s:
            if f'Gem::{gem_target}' in line:
                removed = True
            else:
                t_data.append(line)

    if not removed:
        logger.error(f'Failed to remove Gem::{gem_target} from cmake file {cmake_file}')
        return 1

    # write the cmake
    os.unlink(cmake_file)
    with open(cmake_file, 'w') as s:
        s.writelines(t_data)

    return 0


def remove_gem_from_project(gem_name: str = None,
                            gem_path: str or pathlib.Path = None,
                            gem_target: str = None,
                            project_name: str = None,
                            project_path: str or pathlib.Path = None,
                            dependencies_file: str or pathlib.Path = None,
                            runtime_dependency: bool = False,
                            tool_dependency: bool = False,
                            server_dependency: bool = False,
                            platforms: str = 'Common',
                            remove_from_cmake: bool = False) -> int:
    """
    remove a gem from a project
    :param gem_name: name of the gem to add
    :param gem_path: path to the gem to add
    :param gem_target: the name of teh cmake gem module
    :param project_name: name of the project to add the gem to
    :param project_path: path to the project to add the gem to
    :param dependencies_file: if this dependency goes/is in a specific file
    :param runtime_dependency: bool to specify this is a runtime gem for the game
    :param tool_dependency: bool to specify this is a tool gem for the editor
    :param server_dependency: bool to specify this is a server gem for the server
    :param platforms: str to specify common or which specific platforms
    :param remove_from_cmake: bool to specify that this gem should be removed from cmake
    :return: 0 for success or non 0 failure code
    """

    # we need either a project name or path
    if not project_name and not project_path:
        logger.error(f'Must either specify a Project path or Project Name.')
        return 1

    # if project name resolve it into a path
    if project_name and not project_path:
        project_path = manifest.get_registered(project_name=project_name)
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
    gem_path = pathlib.Path(gem_path).resolve()
    # make sure this gem already exists if we're adding.  We can always remove a gem.
    if not gem_path.is_dir():
        logger.error(f'Gem Path {gem_path} does not exist.')
        return 1

    # find all available modules in this gem_path
    modules = cmake.get_gem_targets(gem_path=gem_path)
    if len(modules) == 0:
        logger.error(f'No gem modules found.')
        return 1

    # if the user has not set a specific gem target remove all of them

    # if gem target not specified, see if there is only 1 module
    if not gem_target:
        if len(modules) == 1:
            gem_target = modules[0]
        else:
            logger.error(f'Gem target not specified: {modules}')
            return 1
    elif gem_target not in modules:
        logger.error(f'Gem target not in gem modules: {modules}')
        return 1

    # if the user has not specified either we will assume they meant the most common which is runtime
    if not runtime_dependency and not tool_dependency and not server_dependency and not dependencies_file:
        logger.warning("Dependency type not specified: Assuming '--runtime-dependency'")
        runtime_dependency = True

    # when removing we will try to do as much as possible even with failures so ret_val will be the last error code
    ret_val = 0

    # if the user has specified the dependencies file then ignore the runtime_dependency and tool_dependency flags
    if dependencies_file:
        dependencies_file = pathlib.Path(dependencies_file).resolve()
        # make sure this is a project has a dependencies_file
        if not dependencies_file.is_file():
            logger.error(f'Dependencies file {dependencies_file} is not present.')
            return 1
        # remove the dependency
        error_code = remove_gem_dependency(dependencies_file, gem_target)
        if error_code:
            ret_val = error_code
    else:
        if ',' in platforms:
            platforms = platforms.split(',')
        else:
            platforms = [platforms]
        for platform in platforms:
            if runtime_dependency:
                # make sure this is a project has a runtime_dependencies.cmake file
                project_runtime_dependencies_file = pathlib.Path(
                    cmake.get_dependencies_cmake_file(project_path=project_path, dependency_type='runtime',
                                                platform=platform)).resolve()
                if not project_runtime_dependencies_file.is_file():
                    logger.error(f'Runtime dependencies file {project_runtime_dependencies_file} is not present.')
                else:
                    # remove the dependency
                    error_code = remove_gem_dependency(project_runtime_dependencies_file, gem_target)
                    if error_code:
                        ret_val = error_code

            if tool_dependency:
                # make sure this is a project has a tool_dependencies.cmake file
                project_tool_dependencies_file = pathlib.Path(
                    cmake.get_dependencies_cmake_file(project_path=project_path, dependency_type='tool',
                                                platform=platform)).resolve()
                if not project_tool_dependencies_file.is_file():
                    logger.error(f'Tool dependencies file {project_tool_dependencies_file} is not present.')
                else:
                    # remove the dependency
                    error_code = remove_gem_dependency(project_tool_dependencies_file, gem_target)
                    if error_code:
                        ret_val = error_code

            if server_dependency:
                # make sure this is a project has a tool_dependencies.cmake file
                project_server_dependencies_file = pathlib.Path(
                    cmake.get_dependencies_cmake_file(project_path=project_path, dependency_type='server',
                                                platform=platform)).resolve()
                if not project_server_dependencies_file.is_file():
                    logger.error(f'Server dependencies file {project_server_dependencies_file} is not present.')
                else:
                    # remove the dependency
                    error_code = remove_gem_dependency(project_server_dependencies_file, gem_target)
                    if error_code:
                        ret_val = error_code

    if remove_from_cmake:
        error_code = remove_gem_cmake.remove_gem_from_cmake(gem_path=gem_path)
        if error_code:
            ret_val = error_code

    return ret_val


def _run_remove_gem_from_project(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    return remove_gem_from_project(args.gem_name,
                                   args.gem_path,
                                   args.gem_target,
                                   args.project_path,
                                   args.project_name,
                                   args.dependencies_file,
                                   args.runtime_dependency,
                                   args.tool_dependency,
                                   args.server_dependency,
                                   args.platforms,
                                   args.remove_from_cmake)


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
    remove_gem_subparser = subparsers.add_parser('remove-gem-from-project')
    group = remove_gem_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-pp', '--project-path', type=str, required=False,
                       help='The path to the project.')
    group.add_argument('-pn', '--project-name', type=str, required=False,
                       help='The name of the project.')
    group = remove_gem_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-gp', '--gem-path', type=str, required=False,
                       help='The path to the gem.')
    group.add_argument('-gn', '--gem-name', type=str, required=False,
                       help='The name of the gem.')
    remove_gem_subparser.add_argument('-gt', '--gem-target', type=str, required=False,
                                      help='The cmake target name to add. If not specified it will assume gem_name')
    remove_gem_subparser.add_argument('-df', '--dependencies-file', type=str, required=False,
                                      help='The cmake dependencies file in which the gem dependencies are specified.'
                                           'If not specified it will assume ')
    remove_gem_subparser.add_argument('-rd', '--runtime-dependency', action='store_true', required=False,
                                      default=False,
                                      help='Optional toggle if this gem should be removed as a runtime dependency')
    remove_gem_subparser.add_argument('-td', '--tool-dependency', action='store_true', required=False,
                                      default=False,
                                      help='Optional toggle if this gem should be removed as a server dependency')
    remove_gem_subparser.add_argument('-sd', '--server-dependency', action='store_true', required=False,
                                      default=False,
                                      help='Optional toggle if this gem should be removed as a server dependency')
    remove_gem_subparser.add_argument('-pl', '--platforms', type=str, required=False,
                                      default='Common',
                                      help='Optional list of platforms this gem should be removed from'
                                           ' Ex. --platforms Mac,Windows,Linux')
    remove_gem_subparser.add_argument('-r', '--remove-from-cmake', type=bool, required=False,
                                      default=False,
                                      help='Automatically call remove-from-cmake.')

    remove_gem_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                      help='By default the home folder is the user folder, override it to this folder.')

    remove_gem_subparser.set_defaults(func=_run_remove_gem_from_project)
