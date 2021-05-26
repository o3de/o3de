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

def get_project_gems(project_path: pathlib.Path,
                            platform: str = 'Common') -> set:
    return get_gems_from_cmake_file(get_enabled_gem_cmake_file(project_path=project_path, platform=platform))


def get_gem_from_cmake_file(cmake_file: pathlib.Path) -> set:
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
            gem_name = line.strip()
            gem_target_set.add(gem_name)
    return gem_target_set


def get_project_gem_paths(project_path:  pathlib.Path,
                          platform: str = 'Common') -> set:
    gem_names = get_project_gems(project_path, platform)
    gem_paths = set()
    for gem_name in gem_names:
        gem_paths.add(manifest.get_registered(gem_name=gem_name))
    return gem_paths


def get_enabled_gem_cmake_file(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                platform: str = 'Common') -> pathlib.Path or None:
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
    enable_gem_filename = "enabled_gem.cmake"

    if platform == 'Common':
        project_code_dir = project_path / 'Gem/Code'
        if project_code_dir.is_dir():
            dependencies_file_path = project_code_dir / enable_gem_filename
            return dependencies_file_path.resolve()
        return (project_path / 'Code' / enable_gem_filename).resolve()
    else:
        project_code_dir = project_path / 'Gem/Code/Platform' / platform
        if project_code_dir.is_dir():
            dependencies_file_path = project_code_dir / enable_gem_filename
            return dependencies_file_path.resolve()
        return (project_path / 'Code/Platform' / platform / enable_gem_filename).resolve()
