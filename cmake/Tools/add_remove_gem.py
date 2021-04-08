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
import logging
import os
import sys
import pathlib

from cmake.Tools import utils
from cmake.Tools import common

logger = logging.getLogger()
logging.basicConfig()


def add_gem_dependency(cmake_file: str,
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


def get_project_gem_list(project_path: str,
                         platform: str = 'Common') -> set:
    runtime_gems = get_gem_list(get_dependencies_cmake(project_path, 'runtime', platform))
    tool_gems = get_gem_list(get_dependencies_cmake(project_path, 'tool', platform))
    server_gems = get_gem_list(get_dependencies_cmake(project_path, 'server', platform))
    return runtime_gems.union(tool_gems.union(server_gems))


def get_gem_list(cmake_file: str) -> set:
    """
    Gets a list of declared gem dependencies of a cmake file
    :param cmake_file: path to the cmake file
    :return: set of gems found
    """
    if not os.path.isfile(cmake_file):
        logger.error(f'Failed to locate cmake file {cmake_file}')
        return set()

    gem_list = set()
    with open(cmake_file, 'r') as s:
        for line in s:
            gem_name = line.split('Gem::')
            if len(gem_name) > 1:
                # Only take the name as everything leading up to the first '.' if found
                # Gem naming conventions will have GemName.Editor, GemName.Server, and GemName
                # as different targets of the GemName Gem
                gem_list.add(gem_name[1].split('.')[0].replace('\n', ''))
    return gem_list


def remove_gem_dependency(cmake_file: str,
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


def add_gem_subdir(cmake_file: str,
                   gem_name: str) -> int:
    """
    adds a gem subdir to a cmake file
    :param cmake_file: path to the cmake file
    :param gem_name: name of the gem to add
    :return: 0 for success or non 0 failure code
    """
    if not os.path.isfile(cmake_file):
        logger.error(f'Failed to locate cmake file {cmake_file}')
        return 1

    if not utils.validate_identifier(gem_name):
        logger.error(f'{gem_name} is not a valid gem name')
        return 1

    # on a line by basis, see if there already is Gem::{gem_name}
    # find the first occurrence of a gem, copy its formatting and replace
    # the gem name with the new one and append it
    # if the gem is already present fail
    t_data = []
    added = False
    add_line = f'add_subdirectory({gem_name})'
    with open(cmake_file, 'r') as s:
        for line in s:
            if add_line in line:
                logger.info(f'{add_line} is already in {cmake_file} - skipping.')
                return 0
            if not added and r'add_subdirectory(' in line:
                new_gem = ' ' * line.find(r'add_subdirectory(') + f'add_subdirectory({gem_name})\n'
                t_data.append(new_gem)
                added = True
            t_data.append(line)

    if not added:
        logger.error(f'Failed to add add_subdirectory({gem_name}) from {cmake_file}.')
        return 1

    # write the cmake
    os.unlink(cmake_file)
    with open(cmake_file, 'w') as s:
        s.writelines(t_data)

    return 0


def remove_gem_subdir(cmake_file: str,
                      gem_name: str) -> int:
    """
    removes a gem subdir form a cmake file
    :param cmake_file: path to the cmake file
    :param gem_name: name of the gem to remove
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
            if f'add_subdirectory({gem_name})' in line:
                removed = True
            else:
                t_data.append(line)

    if not removed:
        logger.error(f'Failed to remove add_subdirectory({gem_name}) from {cmake_file}')
        return 1

    # write the cmake
    os.unlink(cmake_file)
    with open(cmake_file, 'w') as s:
        s.writelines(t_data)

    return 0


def get_dependencies_cmake(project_path: str,
                           dependency_type: str = 'runtime',
                           platform: str = 'Common') -> str:
    if not project_path:
        return ''
    if platform == 'Common':
        dependencies_file = f'{dependency_type}_dependencies.cmake'
        gem_path = os.path.join(project_path, 'Gem', 'Code', dependencies_file)
        if os.path.isfile(gem_path):
            return gem_path
        return os.path.join(project_path, 'Code', dependencies_file)
    else:
        dependencies_file = f'{platform.lower()}_{dependency_type}_dependencies.cmake'
        gem_path = os.path.join(project_path, 'Gem', 'Code', 'Platform', platform, dependencies_file)
        if os.path.isfile(gem_path):
            return gem_path
        return os.path.join(project_path, 'Code', 'Platform', platform, dependencies_file)

# this function is making some not so good assumptions about gem naming
def find_all_gems(gem_folder_list: list) -> list:
    """
    Find all Modules which appear to be gems living within a list of folders
    This method can be replaced or improved on with something more deterministic when LYN-1942 is done
    :param gem_folder_list:
    :return: list of gem objects containing gem Name and Path
    """
    module_gems = {}
    for folder in gem_folder_list:
        root_len = len(os.path.normpath(folder).split(os.sep))
        for root, dirs, files in os.walk(folder):
            for file in files:
                if file == 'CMakeLists.txt':
                    with open(os.path.join(root, file), 'r') as s:
                        for line in s:
                            trimmed = line.lstrip()
                            if trimmed.startswith('NAME '):
                                trimmed = trimmed.rstrip(' \n')
                                split_trimmed = trimmed.split(' ')
                                # Is this a type indicating a gem lives here
                                if len(split_trimmed) == 3 and split_trimmed[2] in ['MODULE', 'GEM_MODULE',
                                                                                    '${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}']:
                                    # Hold our "Name parts" such as Gem.MyGem.Editor as ['Gem', 'MyGem', 'Editor']
                                    name_split = split_trimmed[1].split('.')
                                    split_folders = os.path.normpath(root).split(os.sep)
                                    # This verifies we were passed a folder lower in the folder structure than the
                                    # gem we've found, so we'll name it by the first directory.  This is the common
                                    # case currently of being handed "Gems" where things that live under Gems are
                                    # known by the directory name
                                    if len(split_folders) > root_len:
                                        # Assume the gem is named by the root folder.  May modules may exist
                                        # together within the gem that are added collectively
                                        gem_name = split_folders[root_len]
                                    else:
                                        # We were passed the folder with the gem already inside it, we have to name
                                        # it by the gem name we found
                                        gem_name = name_split[0]
                                    this_gem = module_gems.get(gem_name, None)
                                    if not this_gem:
                                        if len(split_folders) > root_len:
                                            # Assume the gem is named by the root folder.  May modules may exist
                                            # together within the gem that are added collectively
                                            this_gem = {'Name': gem_name,
                                                        'Path': os.path.join(folder, split_folders[root_len])}
                                        else:
                                            this_gem = {'Name': gem_name, 'Path': folder}
                                    # Add any module types we know to be tools only here
                                    if len(name_split) > 1 and name_split[1] in ['Editor', 'Builder']:
                                        this_gem['Tools'] = True
                                    else:
                                        this_gem['Runtime'] = True
                                        this_gem['Tools'] = True

                                    module_gems[gem_name] = this_gem
                    break

    return sorted(list(module_gems.values()), key=lambda x: x.get('Name'))


def find_gem_modules(gem_folder: str) -> list:
    """
    Find all gem modules which appear to be gems living in this folder
    :param gem_folder: the folder to walk down
    :return: list of gem targets
    """
    module_identifiers = [
        'MODULE',
        'GEM_MODULE',
        '${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}'
    ]
    modules = []
    for root, dirs, files in os.walk(gem_folder):
        for file in files:
            if file == 'CMakeLists.txt':
                with open(os.path.join(root, file), 'r') as s:
                    for line in s:
                        trimmed = line.lstrip()
                        if trimmed.startswith('NAME '):
                            trimmed = trimmed.rstrip(' \n')
                            split_trimmed = trimmed.split(' ')
                            if len(split_trimmed) == 3 and split_trimmed[2] in module_identifiers:
                                modules.append(split_trimmed[1])
    return modules


def add_remove_gem(add: bool,
                   dev_root: str,
                   gem_path: str,
                   gem_target: str,
                   project_path: str,
                   dependencies_file: str or None,
                   project_restricted_path: str = 'restricted',
                   runtime_dependency: bool = False,
                   tool_dependency: bool = False,
                   server_dependency: bool = False,
                   platforms: str = 'Common',
                   gem_cmake: str = None) -> int:
    """
    add a gem to a project
    :param add: should we add a gem, if false we remove a gem
    :param dev_root: the dev root of the engine
    :param gem_path: path to the gem to add
    :param gem_target: the name of teh cmake gem module
    :param project_path: path to the project to add the gem to
    :param dependencies_file: if this dependency goes/is in a specific file
    :param project_restricted_path: path to the projects restricted folder
    :param runtime_dependency: bool to specify this is a runtime gem for the game
    :param tool_dependency: bool to specify this is a tool gem for the editor
    :param server_dependency: bool to specify this is a server gem for the server
    :param platforms: str to specify common or which specific platforms
    :param gem_cmake: str to specify that this gem should be removed from the Gems/CMakeLists.txt
    :return: 0 for success or non 0 failure code
    """

    # if no dev root error
    if not dev_root:
        logger.error('Dev root cannot be empty.')
        return 1
    if not os.path.isabs(dev_root):
        dev_root = os.path.abspath(dev_root)
    dev_root = pathlib.Path(dev_root)
    if not dev_root.is_dir():
        logger.error(f'Dev root {dev_root} is not a folder.')
        return 1

    # if no project path error
    if not project_path:
        logger.error('Project path cannot be empty.')
        return 1
    if not os.path.isabs(project_path):
        project_path = f'{dev_root}/{project_path}'
    project_path = pathlib.Path(project_path)
    if not project_path.is_dir():
        logger.error(f'Project path {project_path} is not a folder.')
        return 1

    project_restricted_path = project_restricted_path.replace('\\', '/')
    if not os.path.isabs(project_restricted_path):
        project_restricted_path = f'{dev_root}/{project_restricted_path}'
    project_restricted_path = pathlib.Path(project_restricted_path)
    if not project_restricted_path.is_dir():
        logger.error(f'Project Restricted path {project_restricted_path} is not a folder.')
        return 1

    # if no gem path error
    if not gem_path:
        logger.error('Gem path cannot be empty.')
        return 1
    if not os.path.isabs(gem_path):
        gem_path = f'{dev_root}/Gems/{gem_path}'
    gem_path = pathlib.Path(gem_path)
    # make sure this gem already exists if we're adding.  We can always remove a gem.
    if add and not gem_path.is_dir():
        logger.error(f'{gem_path} dir does not exist.')
        return 1

    # find all available modules in this gem_path
    modules = find_gem_modules(gem_path)
    if len(modules) == 0:
        logger.error(f'No gem modules found.')
        return 1

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

    # gem cmake list text file is assumed to be in the parent folder is not specified
    if add or isinstance(gem_cmake, str):
        if not gem_cmake:
            gem_cmake = gem_path.parent / 'CMakeLists.txt'
        if not os.path.isabs(gem_cmake):
            gem_cmake = f'{dev_root}/{gem_cmake}'
        gem_cmake = pathlib.Path(gem_cmake)
        if not gem_cmake.is_file():
            logger.error(f'CMakeLists.txt file {gem_cmake} does not exist.')
            return 1

    # if the user has not specified either we will assume they meant the most common which is runtime
    if not runtime_dependency and not tool_dependency and not server_dependency and not dependencies_file:
        logger.warning("Dependency type not specified: Assuming '--runtime-dependency'")
        runtime_dependency = True

    ret_val = 0

    # if the user has specified the dependencies file then ignore the runtime_dependency and tool_dependency flags
    if dependencies_file:
        dependencies_file = pathlib.Path(dependencies_file)
        # make sure this is a project has a dependencies_file
        if not dependencies_file.is_file():
            logger.error(f'Dependencies file {dependencies_file} is not present.')
            return 1
        if add:
            # add the dependency
            ret_val = add_gem_dependency(dependencies_file, gem_target)
        else:
            # remove the dependency
            ret_val = remove_gem_dependency(dependencies_file, gem_target)
    else:
        if ',' in platforms:
            platforms = platforms.split(',')
        else:
            platforms = [platforms]
        for platform in platforms:
            if runtime_dependency:
                # make sure this is a project has a runtime_dependencies.cmake file
                project_runtime_dependencies_file = pathlib.Path(get_dependencies_cmake(project_path, 'runtime', platform))
                if not project_runtime_dependencies_file.is_file():
                    logger.error(f'Runtime dependencies file {project_runtime_dependencies_file} is not present.')
                    return 1

                if add:
                    # add the dependency
                    ret_val = add_gem_dependency(project_runtime_dependencies_file, gem_target)
                else:
                    # remove the dependency
                    ret_val = remove_gem_dependency(project_runtime_dependencies_file, gem_target)

            if (ret_val == 0 or not add) and tool_dependency:
                # make sure this is a project has a tool_dependencies.cmake file
                project_tool_dependencies_file = pathlib.Path(get_dependencies_cmake(project_path, 'tool', platform))
                if not project_tool_dependencies_file.is_file():
                    logger.error(f'Tool dependencies file {project_tool_dependencies_file} is not present.')
                    return 1

                if add:
                    # add the dependency
                    ret_val = add_gem_dependency(project_tool_dependencies_file, gem_target)
                else:
                    # remove the dependency
                    ret_val = remove_gem_dependency(project_tool_dependencies_file, gem_target)

            if (ret_val == 0 or not add) and server_dependency:
                # make sure this is a project has a tool_dependencies.cmake file
                project_server_dependencies_file = pathlib.Path(get_dependencies_cmake(project_path, 'server', platform))
                if not project_server_dependencies_file.is_file():
                    logger.error(f'Server dependencies file {project_server_dependencies_file} is not present.')
                    return 1

                if add:
                    # add the dependency
                    ret_val = add_gem_dependency(project_server_dependencies_file, gem_target)
                else:
                    # remove the dependency
                    ret_val = remove_gem_dependency(project_server_dependencies_file, gem_target)

    if add:
        # add the gem subdir to the CMakeList.txt
        ret_val = add_gem_subdir(gem_cmake, gem_path.name)
    elif gem_cmake:
        # remove the gem subdir from the CMakeList.txt
        ret_val = remove_gem_subdir(gem_cmake, gem_path.name)

    return ret_val


def _run_add_gem(args: argparse) -> int:
    return add_remove_gem(True,
                          common.determine_engine_root(),
                          args.gem_path,
                          args.gem_target,
                          args.project_path,
                          args.dependencies_file,
                          args.project_restricted_path,
                          args.runtime_dependency,
                          args.tool_dependency,
                          args.server_dependency,
                          args.platforms,
                          args.add_to_cmake)


def _run_remove_gem(args: argparse) -> int:
    return add_remove_gem(False,
                          common.determine_engine_root(),
                          args.gem_path,
                          args.gem_target,
                          args.project_path,
                          args.dependencies_file,
                          args.project_restricted_path,
                          args.runtime_dependency,
                          args.tool_dependency,
                          args.server_dependency,
                          args.platforms,
                          args.remove_from_cmake)


def add_args(parser, subparsers) -> None:
    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked locally or aggregated by a central python file.
    Ex. Directly run from this file alone with: python add_remove_gem.py add_gem --gem TestGem --project TestProject
    OR
    lmbr.py can aggregate commands by importing add_remove_gem, call add_args and
    execute: python lmbr.py add_gem --gem TestGem --project TestProject
    :param parser: the caller instantiates a parser and passes it in here
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    add_gem_subparser = subparsers.add_parser('add-gem')
    add_gem_subparser.add_argument('-pp', '--project-path', type=str, required=True,
                                   help='The path to the project, can be absolute or dev root relative')
    add_gem_subparser.add_argument('-prp', '--project-restricted-path', type=str, required=False,
                                   default='restricted',
                                   help='The path to the projects restricted folder, can be absolute or dev root'
                                        ' relative')
    add_gem_subparser.add_argument('-gp', '--gem-path', type=str, required=True,
                                   help='The path to the gem, can be absolute or dev root/Gems relative')
    add_gem_subparser.add_argument('-gt', '--gem-target', type=str, required=False,
                                   help='The cmake target name to add. If not specified it will assume gem_name')
    add_gem_subparser.add_argument('-df', '--dependencies-file', type=str, required=False,
                                   help='The cmake dependencies file in which the gem dependencies are specified.'
                                        'If not specified it will assume ')
    add_gem_subparser.add_argument('-rd', '--runtime-dependency', action='store_true', required=False,
                                   default=False,
                                   help='Optional toggle if this gem should be added as a runtime dependency')
    add_gem_subparser.add_argument('-td', '--tool-dependency', action='store_true', required=False,
                                   default=False,
                                   help='Optional toggle if this gem should be added as a tool dependency')
    add_gem_subparser.add_argument('-sd', '--server-dependency', action='store_true', required=False,
                                   default=False,
                                   help='Optional toggle if this gem should be added as a server dependency')
    add_gem_subparser.add_argument('-pl', '--platforms', type=str, required=False,
                                   default='Common',
                                   help='Optional list of platforms this gem should be added to.'
                                   ' Ex. --platforms Mac,Windows,Linux')
    add_gem_subparser.add_argument('-a', '--add-to-cmake', type=str, required=False,
                                      default=None,
                                      help='Add the gem folder to the CMakeLists.txt so that it builds. If not'
                                           ' specified it will assume the CMakeLists.txt file in the parent folder of'
                                           ' the gem_path.')
    add_gem_subparser.set_defaults(func=_run_add_gem)

    remove_gem_subparser = subparsers.add_parser('remove-gem')
    remove_gem_subparser.add_argument('-pp', '--project-path', type=str, required=True,
                                      help='The path to the project, can be absolute or dev root relative')
    remove_gem_subparser.add_argument('-prp', '--project-restricted-path', type=str, required=False,
                                      default='restricted',
                                      help='The path to the projects restricted folder, can be absolute or dev root'
                                           ' relative')
    remove_gem_subparser.add_argument('-gp', '--gem-path', type=str, required=True,
                                      help='The path to the gem, can be absolute or dev root/Gems relative')
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
    remove_gem_subparser.add_argument('-r', '--remove-from-cmake', type=str, required=False,
                                      default=None,
                                      help='Remove the gem folder from the CMakeLists.txt that includes it so that it '
                                           'no longer builds build. If not specified it will assume you dont want to'
                                           ' remove it.')
    remove_gem_subparser.set_defaults(func=_run_remove_gem)


if __name__ == "__main__":
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers
    the_subparsers = the_parser.add_subparsers(help='sub-command help')

    # add args to the parser
    add_args(the_parser, the_subparsers)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args)

    # return
    sys.exit(ret)
