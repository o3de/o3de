#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
Contains methods for query CMake gem target information
"""

import logging
import os
import pathlib

from o3de import manifest, utils

logger = logging.getLogger('o3de.cmake')
logging.basicConfig(format=utils.LOG_FORMAT)

enable_gem_start_marker = 'set(ENABLED_GEMS'
enable_gem_end_marker = ')'


def add_gem_dependency(cmake_file: pathlib.Path,
                       gem_name: str) -> int:
    """
    adds a gem dependency to a cmake file
    :param cmake_file: path to the cmake file
    :param gem_name: name of the gem
    :return: 0 for success or non 0 failure code
    """
    if not cmake_file.is_file():
        logger.error(f'Failed to locate cmake file {str(cmake_file)}')
        return 1

    # on a line by basis, see if there already is {gem_name}
    # find the first occurrence of a gem, copy its formatting and replace
    # the gem name with the new one and append it
    t_data = []
    added = False
    start_marker_line_index = None
    end_marker_line_index = None
    with cmake_file.open('r') as s:
        in_gem_list = False
        line_index = 0
        for line in s:
            parsed_line = line.strip()
            if parsed_line.startswith(enable_gem_start_marker):
                # Skip pass the 'set(ENABLED_GEMS' marker just in case their are gems declared on the same line
                parsed_line = parsed_line[len(enable_gem_start_marker):]
                # Set the flag to indicate that we are in the ENABLED_GEMS variable
                in_gem_list = True
                start_marker_line_index = line_index

            if in_gem_list:
                # Since we are inside the ENABLED_GEMS variable determine if the line has the end_marker of ')'
                if parsed_line.endswith(enable_gem_end_marker):
                    # Strip away the line end marker
                    parsed_line = parsed_line[:-len(enable_gem_end_marker)]
                    # Set the flag to indicate that we are no longer in the ENABLED_GEMS variable after this line
                    in_gem_list = False
                    end_marker_line_index = line_index

                # Split the rest of the line on whitespace just in case there are multiple gems in a line
                gem_name_list = map(lambda gem_name: gem_name.strip('"'), parsed_line.split())
                if gem_name in gem_name_list:
                    logger.warning(f'{gem_name} is already enabled in file {str(cmake_file)}.')
                    return 0

            t_data.append(line)
            line_index += 1

    indent = 4
    if start_marker_line_index:
        # Make sure if there is a enable gem start marker, there is an end marker as well
        if not end_marker_line_index:
            logger.error(f'The Enable Gem start marker of "{enable_gem_start_marker}" has been found, but not the'
                         f' Enable Gem end marker of "{enable_gem_end_marker}"')
            return 1

        # Insert the gem before the ')' end marker
        end_marker_partition = list(t_data[end_marker_line_index].rpartition(enable_gem_end_marker))
        end_marker_partition[1] = f'{" " * indent}{gem_name}\n' + end_marker_partition[1]
        t_data[end_marker_line_index] = ''.join(end_marker_partition)
        added = True

    # if we didn't add, then create a new set(ENABLED_GEMS) variable
    # add a new gem, if empty the correct format is 1 tab=4spaces
    if not added:
        t_data.append('\n')
        t_data.append(f'{enable_gem_start_marker}\n')
        t_data.append(f'{" "  * indent}{gem_name}\n')
        t_data.append(f'{enable_gem_end_marker}\n')

    # write the cmake
    with cmake_file.open('w') as s:
        s.writelines(t_data)

    return 0

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
    removed = False

    with cmake_file.open('r') as s:
        in_gem_list = False
        for line in s:
            # Strip whitespace from both ends of the line, but keep track of the leading whitespace
            # for indenting the result line
            parsed_line = line.lstrip()
            indent = line[:-len(parsed_line)]
            parsed_line = parsed_line.rstrip()
            result_line = indent
            if parsed_line.startswith(enable_gem_start_marker):
                # Skip pass the 'set(ENABLED_GEMS' marker just in case their are gems declared on the same line
                parsed_line = parsed_line[len(enable_gem_start_marker):]
                result_line += enable_gem_start_marker
                # Set the flag to indicate that we are in the ENABLED_GEMS variable
                in_gem_list = True

            if in_gem_list:
                # Since we are inside the ENABLED_GEMS variable determine if the line has the end_marker of ')'
                if parsed_line.endswith(enable_gem_end_marker):
                    # Strip away the line end marker
                    parsed_line = parsed_line[:-len(enable_gem_end_marker)]
                    # Set the flag to indicate that we are no longer in the ENABLED_GEMS variable after this line
                    in_gem_list = False
                # Split the rest of the line on whitespace just in case there are multiple gems in a line
                # Strip double quotes surround any gem name
                gem_name_list = list(map(lambda gem_name: gem_name.strip('"'), parsed_line.split()))
                while gem_name in gem_name_list:
                    gem_name_list.remove(gem_name)
                    removed = True

                # Append the renaming gems to the line
                result_line += ' '.join(gem_name_list)
                # If the in_gem_list was flipped to false, that means the currently parsed line contained the
                # line end marker, so append that to the result_line
                result_line += enable_gem_end_marker if not in_gem_list else ''
                t_data.append(result_line + '\n')
            else:
                t_data.append(line)

    if not removed:
        logger.error(f'Failed to remove {gem_name} from cmake file {cmake_file}')
        return 1

    # write the cmake
    with cmake_file.open('w') as s:
        s.writelines(t_data)

    return 0


def get_project_gems(project_path: pathlib.Path,
                     platform: str = 'Common') -> set:
    return get_gems_from_cmake_file(get_enabled_gem_cmake_file(project_path=project_path, platform=platform))


def get_enabled_gems(cmake_file: pathlib.Path) -> set:
    """
    Gets a list of enabled gems from the cmake file
    :param cmake_file: path to the cmake file
    :return: set of gem targets found
    """
    cmake_file = pathlib.Path(cmake_file).resolve()

    if not cmake_file.is_file():
        logger.error(f'Failed to locate cmake file {cmake_file}')
        return set()

    gem_target_set = set()
    with cmake_file.open('r') as s:
        in_gem_list = False
        for line in s:
            line = line.strip()
            if line.startswith(enable_gem_start_marker):
                # Set the flag to indicate that we are in the ENABLED_GEMS variable
                in_gem_list = True
                # Skip pass the 'set(ENABLED_GEMS' marker just in case their are gems declared on the same line
                line = line[len(enable_gem_start_marker):]
            if in_gem_list:
                # Since we are inside the ENABLED_GEMS variable determine if the line has the end_marker of ')'
                if line.endswith(enable_gem_end_marker):
                    # Strip away the line end marker
                    line = line[:-len(enable_gem_end_marker)]
                    # Set the flag to indicate that we are no longer in the ENABLED_GEMS variable after this line
                    in_gem_list = False
                # Split the rest of the line on whitespace just in case there are multiple gems in a line
                gem_name_list = list(map(lambda gem_name: gem_name.strip('"'), line.split()))
                gem_target_set.update(gem_name_list)

    return gem_target_set


def get_project_gem_paths(project_path:  pathlib.Path,
                          platform: str = 'Common') -> set:
    gem_names = get_project_gems(project_path, platform)
    gem_paths = set()
    for gem_name in gem_names:
        gem_paths.add(manifest.get_registered(gem_name=gem_name, project_path=project_path))
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
    enable_gem_filename = "enabled_gems.cmake"

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
