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
import pathlib
import re

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
    :param gem_name: name of the gem with optional version specifier
    :return: 0 for success or non 0 failure code
    """
    if not cmake_file.is_file():
        logger.error(f'Failed to locate cmake file {str(cmake_file)}')
        return 1

    gem_name_with_version_specifier = gem_name
    gem_name, gem_version_specifier = utils.get_object_name_and_optional_version_specifier(gem_name)

    pre_gem_names = ''
    post_gem_names = ''
    all_gem_names = '' 
    indent = 4

    with cmake_file.open('r') as s:
        file_contents = s.read()
        if file_contents:
            # regex to isolate pre, gem names and post 
            regex_all_gem_names = re.compile('(?P<pre>[\s\S]*set\s*\(\s*ENABLED_GEMS\s*)?(?P<gem_names>[^\)]+)(?P<post>\)[\s\S]*)?')
            all_gem_names_matches = regex_all_gem_names.search(file_contents)
            if not all_gem_names_matches:
                logger.error(f'{cmake_file} is not formatted correctly and cannot be modified.')
                return 1

            pre_gem_names = all_gem_names_matches.group('pre')
            post_gem_names = all_gem_names_matches.group('post')
            if pre_gem_names and not post_gem_names:
                logger.error(f'{cmake_file} is missing a closing parenthesis or is not formatted correctly.')
                return 1

            # regex to replace every version of the gem name 
            regex_replace_gem_name = re.compile(f'(?:^|\s+){gem_name}[=><~]*[\S]*')
            all_gem_names = regex_replace_gem_name.sub('', all_gem_names_matches.group('gem_names'))

            all_gem_names += f'\n{" "  * indent}{gem_name_with_version_specifier}'


    if not pre_gem_names:
        pre_gem_names = f'\n' \
                        f'{enable_gem_start_marker}\n' \
                        f'{" "  * indent}{gem_name_with_version_specifier}\n'

    if not post_gem_names:
        post_gem_names = f'{enable_gem_end_marker}\n'

    # write the cmake
    with cmake_file.open('w') as s:
        s.write(pre_gem_names + all_gem_names + post_gem_names)

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
                # Strip of trailing whitespace. This also strips result lines which are empty of the indent
                result_line = result_line.rstrip()
                if result_line:
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
        possible_project_enable_gem_filename_paths = [
            pathlib.Path(project_path / 'Gem' / enable_gem_filename),
            pathlib.Path(project_path / 'Gem/Code' / enable_gem_filename),
            pathlib.Path(project_path / 'Code' / enable_gem_filename)
        ]
        for possible_project_enable_gem_filename_path in possible_project_enable_gem_filename_paths:
            if possible_project_enable_gem_filename_path.is_file():
                return possible_project_enable_gem_filename_path.resolve()
        return possible_project_enable_gem_filename_paths[0].resolve()
    else:
        possible_project_platform_enable_gem_filename_paths = [
            pathlib.Path(project_path / 'Gem/Platform' / platform / enable_gem_filename),
            pathlib.Path(project_path / 'Gem/Code/Platform' / platform / enable_gem_filename),
            pathlib.Path(project_path / 'Code/Platform' / platform / enable_gem_filename)
        ]
        for possible_project_platform_enable_gem_filename_path in possible_project_platform_enable_gem_filename_paths:
            if possible_project_platform_enable_gem_filename_path.is_file():
                return possible_project_platform_enable_gem_filename_path.resolve()
        return possible_project_platform_enable_gem_filename_paths[0].resolve()
