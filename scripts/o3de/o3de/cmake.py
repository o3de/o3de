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
Contains methods for query CMake gem target information
"""

import logging
import os
import pathlib

from o3de import manifest

logger = logging.getLogger()
logging.basicConfig()

def get_project_runtime_gem_targets(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    return get_gem_targets_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='runtime', platform=platform))


def get_project_tool_gem_targets(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    return get_gem_targets_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='tool', platform=platform))


def get_project_server_gem_targets(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    return get_gem_targets_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='server', platform=platform))


def get_project_gem_targets(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    runtime_gems = get_gem_targets_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='runtime', platform=platform))
    tool_gems = get_gem_targets_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='tool', platform=platform))
    server_gems = get_gem_targets_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='server', platform=platform))
    return runtime_gems.union(tool_gems.union(server_gems))


def get_gem_targets_from_cmake_file(cmake_file: str or pathlib.Path) -> set:
    """
    Gets a list of declared gem targets dependencies of a cmake file
    :param cmake_file: path to the cmake file
    :return: set of gem targets found
    """
    cmake_file = pathlib.Path(cmake_file).resolve()

    if not cmake_file.is_file():
        logger.error(f'Failed to locate cmake file {cmake_file}')
        return set()

    gem_target_set = set()
    with cmake_file.open('r') as s:
        for line in s:
            gem_name = line.split('Gem::')
            if len(gem_name) > 1:
                # Only take the name as everything leading up to the first '.' if found
                # Gem naming conventions will have GemName.Editor, GemName.Server, and GemName
                # as different targets of the GemName Gem
                gem_target_set.add(gem_name[1].replace('\n', ''))
    return gem_target_set


def get_project_runtime_gem_names(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    return get_gem_names_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='runtime', platform=platform))


def get_project_tool_gem_names(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    return get_gem_names_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='tool', platform=platform))


def get_project_server_gem_names(project_path: str or pathlib.Path,
                            platform: str = 'Common') -> set:
    return get_gem_names_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='server', platform=platform))


def get_project_gem_names(project_path: str or pathlib.Path,
                     platform: str = 'Common') -> set:
    runtime_gem_names = get_gem_names_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='runtime', platform=platform))
    tool_gem_names = get_gem_names_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='tool', platform=platform))
    server_gem_names = get_gem_names_from_cmake_file(get_dependencies_cmake_file(project_path=project_path, dependency_type='server', platform=platform))
    return runtime_gem_names.union(tool_gem_names.union(server_gem_names))


def get_gem_names_from_cmake_file(cmake_file: str or pathlib.Path) -> set:
    """
    Gets a list of declared gem dependencies of a cmake file
    :param cmake_file: path to the cmake file
    :return: set of gems found
    """
    cmake_file = pathlib.Path(cmake_file).resolve()

    if not cmake_file.is_file():
        logger.error(f'Failed to locate cmake file {cmake_file}')
        return set()

    gem_set = set()
    with cmake_file.open('r') as s:
        for line in s:
            gem_name = line.split('Gem::')
            if len(gem_name) > 1:
                # Only take the name as everything leading up to the first '.' if found
                # Gem naming conventions will have GemName.Editor, GemName.Server, and GemName
                # as different targets of the GemName Gem
                gem_set.add(gem_name[1].split('.')[0].replace('\n', ''))
    return gem_set


def get_project_runtime_gem_paths(project_path: str or pathlib.Path,
                          platform: str = 'Common') -> set:
    gem_names = get_project_runtime_gem_names(project_path, platform)
    gem_paths = set()
    for gem_name in gem_names:
        gem_paths.add(manifest.get_registered(gem_name=gem_name))
    return gem_paths


def get_project_tool_gem_paths(project_path: str or pathlib.Path,
                          platform: str = 'Common') -> set:
    gem_names = get_project_tool_gem_names(project_path, platform)
    gem_paths = set()
    for gem_name in gem_names:
        gem_paths.add(manifest.get_registered(gem_name=gem_name))
    return gem_paths


def get_project_server_gem_paths(project_path: str or pathlib.Path,
                          platform: str = 'Common') -> set:
    gem_names = get_project_server_gem_names(project_path, platform)
    gem_paths = set()
    for gem_name in gem_names:
        gem_paths.add(manifest.get_registered(gem_name=gem_name))
    return gem_paths


def get_project_gem_paths(project_path: str or pathlib.Path,
                          platform: str = 'Common') -> set:
    gem_names = get_project_gem_names(project_path, platform)
    gem_paths = set()
    for gem_name in gem_names:
        gem_paths.add(manifest.get_registered(gem_name=gem_name))
    return gem_paths


def get_dependencies_cmake_file(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                dependency_type: str = 'runtime',
                                platform: str = 'Common') -> str or None:
    """
    get the standard cmake file name for a particular type of dependency
    :param gem_name: name of the gem, resolves gem_path
    :param gem_path: path of the gem
    :return: list of gem targets
    """
    if not project_name and not project_path:
        logger.error(f'Must supply either a Project Name or Project Path.')
        return None

    if project_name and not project_path:
        project_path = manifest.get_registered(project_name=project_name)

    project_path = pathlib.Path(project_path).resolve()

    if platform == 'Common':
        dependencies_file = f'{dependency_type}_dependencies.cmake'
        dependencies_file_path = project_path / 'Gem/Code' / dependencies_file
        if dependencies_file_path.is_file():
            return dependencies_file_path
        return project_path / 'Code' / dependencies_file
    else:
        dependencies_file = f'{platform.lower()}_{dependency_type}_dependencies.cmake'
        dependencies_file_path = project_path / 'Gem/Code/Platform' / platform / dependencies_file
        if dependencies_file_path.is_file():
            return dependencies_file_path
        return project_path / 'Code/Platform' / platform / dependencies_file


def get_all_gem_targets() -> list:
    modules = []
    for gem_path in manifest.get_all_gems():
        this_gems_targets = get_gem_targets(gem_path=gem_path)
        modules.extend(this_gems_targets)
    return modules


def get_gem_targets(gem_name: str = None,
                    gem_path: str or pathlib.Path = None) -> list:
    """
    Finds gem targets in a gem
    :param gem_name: name of the gem, resolves gem_path
    :param gem_path: path of the gem
    :return: list of gem targets
    """
    if not gem_name and not gem_path:
        return []

    if gem_name and not gem_path:
        gem_path = manifest.get_registered(gem_name=gem_name)

    if not gem_path:
        return []

    gem_path = pathlib.Path(gem_path).resolve()
    gem_json = gem_path / 'gem.json'
    if not validation.valid_o3de_gem_json(gem_json):
        return []

    module_identifiers = [
        'MODULE',
        'GEM_MODULE',
        '${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}'
    ]
    modules = []
    for root, dirs, files in os.walk(gem_path):
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
