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
import json
import logging
import os
import pathlib
import sys

from o3de import cmake, manifest, validation

logger = logging.getLogger()
logging.basicConfig()

def add_gem_dependency(cmake_file: str or pathlib.Path,
                       gem_target: str) -> int:
    """
    adds a gem dependency to a cmake file
    :param cmake_file: path to the cmake file
    :param gem_target: name of the cmake target
    :return: 0 for success or non 0 failure code
    """
    if not os.path.isfile(cmake_file):
        logger.error(f'Failed to locate cmake file {cmake_file}')
        return 1

    # on a line by basis, see if there already is Gem::{gem_name}
    # find the first occurrence of a gem, copy its formatting and replace
    # the gem name with the new one and append it
    # if the gem is already present fail
    t_data = []
    added = False
    with open(cmake_file, 'r') as s:
        for line in s:
            if f'Gem::{gem_target}' in line:
                logger.warning(f'{gem_target} is already a gem dependency.')
                return 0
            if not added and r'Gem::' in line:
                new_gem = ' ' * line.find(r'Gem::') + f'Gem::{gem_target}\n'
                t_data.append(new_gem)
                added = True
            t_data.append(line)

    # if we didn't add it the set gem dependencies could be empty so
    # add a new gem, if empty the correct format is 1 tab=4spaces
    if not added:
        index = 0
        for line in t_data:
            index = index + 1
            if r'set(GEM_DEPENDENCIES' in line:
                t_data.insert(index, f'    Gem::{gem_target}\n')
                added = True
                break

    # if we didn't add it then it's not here, add a whole new one
    if not added:
        t_data.append('\n')
        t_data.append('set(GEM_DEPENDENCIES\n')
        t_data.append(f'    Gem::{gem_target}\n')
        t_data.append(')\n')

    # write the cmake
    os.unlink(cmake_file)
    with open(cmake_file, 'w') as s:
        s.writelines(t_data)

    return 0


def add_gem_to_project(gem_name: str = None,
                       gem_path: str or pathlib.Path = None,
                       gem_target: str = None,
                       project_name: str = None,
                       project_path: str or pathlib.Path = None,
                       dependencies_file: str or pathlib.Path = None,
                       runtime_dependency: bool = False,
                       tool_dependency: bool = False,
                       server_dependency: bool = False,
                       platforms: str = 'Common',
                       add_to_cmake: bool = True) -> int:
    """
    add a gem to a project
    :param gem_name: name of the gem to add
    :param gem_path: path to the gem to add
    :param gem_target: the name of the cmake gem module
    :param project_name: name of to the project to add the gem to
    :param project_path: path to the project to add the gem to
    :param dependencies_file: if this dependency goes/is in a specific file
    :param runtime_dependency: bool to specify this is a runtime gem for the game
    :param tool_dependency: bool to specify this is a tool gem for the editor
    :param server_dependency: bool to specify this is a server gem for the server
    :param platforms: str to specify common or which specific platforms
    :param add_to_cmake: bool to specify that this gem should be added to cmake
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

    # get the engine name this project is associated with
    # and resolve that engines path
    project_json = project_path / 'project.json'
    if not validation.valid_o3de_project_json(project_json):
        logger.error(f'Project json {project_json} is not valid.')
        return 1
    with project_json.open('r') as s:
        try:
            project_json_data = json.load(s)
        except json.JSONDecodeError as e:
            logger.error(f'Error loading Project json {project_json}: {str(e)}')
            return 1
        else:
            try:
                engine_name = project_json_data['engine']
            except KeyError as e:
                logger.error(f'Project json {project_json} "engine" not found: {str(e)}')
                return 1
            else:
                engine_path = manifest.get_registered(engine_name=engine_name)
                if not engine_path:
                    logger.error(f'Engine {engine_name} is not registered.')
                    return 1

    # we need either a gem name or path
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

    # if add to cmake, make sure the gem.json exists and valid before we proceed
    if add_to_cmake:
        gem_json = gem_path / 'gem.json'
        if not gem_json.is_file():
            logger.error(f'Gem json {gem_json} is not present.')
            return 1
        if not validation.valid_o3de_gem_json(gem_json):
            logger.error(f'Gem json {gem_json} is not valid.')
            return 1

    # find all available modules in this gem_path
    modules = cmake.get_gem_targets(gem_path=gem_path)
    if len(modules) == 0:
        logger.error(f'No gem modules found under {gem_path}.')
        return 1

    # if the gem has no modules and the user has specified a target fail
    if gem_target and not modules:
        logger.error(f'Gem has no targets, but gem target {gem_target} was specified.')
        return 1

    # if the gem target is not in the modules
    if gem_target not in modules:
        logger.error(f'Gem target not in gem modules: {modules}')
        return 1

    if gem_target:
        # if the user has not specified either we will assume they meant the most common which is runtime
        if not runtime_dependency and not tool_dependency and not server_dependency and not dependencies_file:
            logger.warning("Dependency type not specified: Assuming '--runtime-dependency'")
            runtime_dependency = True

        ret_val = 0

        # if the user has specified the dependencies file then ignore the runtime_dependency and tool_dependency flags
        if dependencies_file:
            dependencies_file = pathlib.Path(dependencies_file).resolve()
            # make sure this is a project has a dependencies_file
            if not dependencies_file.is_file():
                logger.error(f'Dependencies file {dependencies_file} is not present.')
                return 1
            # add the dependency
            ret_val = add_gem_dependency(dependencies_file, gem_target)

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
                        return 1
                    # add the dependency
                    ret_val = add_gem_dependency(project_runtime_dependencies_file, gem_target)

                if (ret_val == 0) and tool_dependency:
                    # make sure this is a project has a tool_dependencies.cmake file
                    project_tool_dependencies_file = pathlib.Path(
                        cmake.get_dependencies_cmake_file(project_path=project_path, dependency_type='tool',
                                                    platform=platform)).resolve()
                    if not project_tool_dependencies_file.is_file():
                        logger.error(f'Tool dependencies file {project_tool_dependencies_file} is not present.')
                        return 1
                    # add the dependency
                    ret_val = add_gem_dependency(project_tool_dependencies_file, gem_target)

                if (ret_val == 0) and server_dependency:
                    # make sure this is a project has a tool_dependencies.cmake file
                    project_server_dependencies_file = pathlib.Path(
                        cmake.get_dependencies_cmake_file(project_path=project_path, dependency_type='server',
                                                    platform=platform)).resolve()
                    if not project_server_dependencies_file.is_file():
                        logger.error(f'Server dependencies file {project_server_dependencies_file} is not present.')
                        return 1
                    # add the dependency
                    ret_val = add_gem_dependency(project_server_dependencies_file, gem_target)

    return ret_val


def _run_add_gem_to_project(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    return add_gem_to_project(args.gem_name,
                              args.gem_path,
                              args.gem_target,
                              args.project_name,
                              args.project_path,
                              args.dependencies_file,
                              args.runtime_dependency,
                              args.tool_dependency,
                              args.server_dependency,
                              args.platforms,
                              args.add_to_cmake)


def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python add_gem_project.py --project-path "D:/TestProject" --gem-path "D:/TestGem"
    :param parser: the caller passes an argparse parser like instance to this method
    """
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-pp', '--project-path', type=str, required=False,
                       help='The path to the project.')
    group.add_argument('-pn', '--project-name', type=str, required=False,
                       help='The name of the project.')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-gp', '--gem-path', type=str, required=False,
                       help='The path to the gem.')
    group.add_argument('-gn', '--gem-name', type=str, required=False,
                       help='The name of the gem.')
    parser.add_argument('-gt', '--gem-target', type=str, required=False,
                                   help='The cmake target name to add. If not specified it will assume gem_name')
    parser.add_argument('-df', '--dependencies-file', type=str, required=False,
                                   help='The cmake dependencies file in which the gem dependencies are specified.'
                                        'If not specified it will assume ')
    parser.add_argument('-rd', '--runtime-dependency', action='store_true', required=False,
                                   default=False,
                                   help='Optional toggle if this gem should be added as a runtime dependency')
    parser.add_argument('-td', '--tool-dependency', action='store_true', required=False,
                                   default=False,
                                   help='Optional toggle if this gem should be added as a tool dependency')
    parser.add_argument('-sd', '--server-dependency', action='store_true', required=False,
                                   default=False,
                                   help='Optional toggle if this gem should be added as a server dependency')
    parser.add_argument('-pl', '--platforms', type=str, required=False,
                                   default='Common',
                                   help='Optional list of platforms this gem should be added to.'
                                        ' Ex. --platforms Mac,Windows,Linux')
    parser.add_argument('-a', '--add-to-cmake', type=bool, required=False,
                                   default=True,
                                   help='Automatically call add-gem-to-cmake.')

    parser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                   help='By default the home folder is the user folder, override it to this folder.')

    parser.set_defaults(func=_run_add_gem_to_project)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py add-gem-to-project --project-path "D:/TestProject" --gem-path "D:/TestGem"
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    add_gem_project_subparser = subparsers.add_parser('add-gem-to-project')
    add_parser_args(add_gem_project_subparser)


def main():
    """
    Runs add_gem_project.py script as standalone script
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
