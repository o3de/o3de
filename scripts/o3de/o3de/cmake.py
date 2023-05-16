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

import argparse
import logging
import pathlib
import sys

from o3de import manifest, utils, compatibility

logger = logging.getLogger('o3de.cmake')
logging.basicConfig(format=utils.LOG_FORMAT)

enable_gem_start_marker = 'set(ENABLED_GEMS'
enable_gem_end_marker = ')'

# The need for `enabled_gems.cmake` is deprecated
# Functionality still exists to retrieve and remove gems from `enabled_gems.cmake`
# but gems should only be added to `project.json` by the o3de CLI

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


def resolve_gem_dependency_paths(
        engine_path:pathlib.Path,
        project_path:pathlib.Path,
        external_subdirectories:str or list or None,
        resolved_gem_dependencies_output_path:pathlib.Path or None):
    """
    Resolves gem dependencies for the given engine and project and
    writes the output to the path provided.  This is used during CMake
    configuration because writing a CMake depencency resolver would be
    difficult and Python already has a solver with unit tests.
    :param engine_path: optional path to the engine, if not provided, the project's engine will be determined 
    :param project_path: optional path to the project, if not provided the engine path must be provided
    :param resolved_gem_dependencies_output_path: optional path to a file that will be written 
        containing a CMake list of gem names and paths.  If not provided, the list is written to STDOUT.
    :return: 0 for success or non 0 failure code
    """

    if not engine_path and not project_path:
        logger.error(f'project path or engine path are required to resolve dependencies')
        return 1

    if not engine_path:
        engine_path = manifest.get_project_engine_path(project_path=project_path)
        if not engine_path:
            engine_path = manifest.get_this_engine_path()
            if not engine_path:
                logger.error('Failed to find a valid engine path for the project at '
                             f'"{project_path}" which is required to resolve gem dependencies.')
                return 1

            logger.warning('Failed to determine the correct engine for the project at '
                           f'"{project_path}", falling back to this engine at {engine_path}.')
    
    engine_json_data = manifest.get_engine_json_data(engine_path=engine_path)
    if not engine_json_data:
        logger.error('Failed to retrieve engine json data for the engine at '
                     f'"{engine_path}" which is required to resolve gem dependencies.')
        return 1

    if project_path:
        project_json_data = manifest.get_project_json_data(project_path=project_path)
        if not project_json_data:
            logger.error('Failed to retrieve project json data for the project at '
                        f'"{project_path}" which is required to resolve gem dependencies.')
            return 1
        active_gem_names = project_json_data.get('gem_names',[])
        enabled_gems_file = manifest.get_enabled_gem_cmake_file(project_path=project_path)
        if enabled_gems_file.is_file():
            active_gem_names.extend(manifest.get_enabled_gems(enabled_gems_file))
    else:
        active_gem_names = engine_json_data.get('gem_names',[])

    # some gem name entries will be dictionaries - convert to a set of strings 
    gem_names_with_optional_gems = utils.get_gem_names_set(active_gem_names, include_optional=True)
    if not gem_names_with_optional_gems:
        logger.info(f'No gem names were found to use as input to resolve gem dependencies.')
        if resolved_gem_dependencies_output_path:
            with resolved_gem_dependencies_output_path.open('w') as output:
                output.write('')
        return 0

    all_gems_json_data = manifest.get_gems_json_data_by_name(engine_path=engine_path, 
                                                             project_path=project_path, 
                                                             include_manifest_gems=True, 
                                                             include_engine_gems=True,
                                                             external_subdirectories=external_subdirectories.split(';') if isinstance(external_subdirectories, str) else external_subdirectories)

    # First try to resolve with optional gems
    results, errors = compatibility.resolve_gem_dependencies(gem_names_with_optional_gems, 
                                                             all_gems_json_data, 
                                                             engine_json_data, 
                                                             include_optional=True)
    if errors:
        logger.warning('Failed to resolve dependencies with optional gems, trying without optional gems.')

        # Try without optional gems
        gem_names_without_optional = utils.get_gem_names_set(active_gem_names, include_optional=False)
        results, errors = compatibility.resolve_gem_dependencies(gem_names_without_optional, 
                                                                 all_gems_json_data, 
                                                                 engine_json_data,
                                                                 include_optional=False)

    if errors:
        logger.error(f'Failed to resolve dependencies:\n  ' + '\n  '.join(errors))
        return 1

    # make a list of <gem_name>;<gem_path> for cmake
    gem_paths = sorted(f"{gem.gem_json_data['gem_name'].strip()};{gem.gem_json_data['path'].resolve().as_posix()}" for _, gem in results.items())
    # use dict to remove duplicates and preserve order so it's easier to read/debug
    gem_paths = list(dict.fromkeys(gem_paths))
    # join everything with a ';' character which is a list entry delimiter in CMake
    # so the keys and values are all list entries
    gem_paths_list = ';'.join(gem_paths)

    if resolved_gem_dependencies_output_path:
        with resolved_gem_dependencies_output_path.open('w') as output:
            output.write(gem_paths_list)
    else:
        print(gem_paths_list)

    return 0

def _resolve_gem_dependency_paths(args: argparse) -> int:
    return resolve_gem_dependency_paths(
                            engine_path=args.engine_path,
                            project_path=args.project_path,
                            external_subdirectories=args.external_subdirectories,
                            resolved_gem_dependencies_output_path=args.gem_paths_output_file
                             )

def add_parser_args(parser):
    group = parser.add_argument_group("resolve gem dependencies")
    group.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                       help='The path to the project.')
    group.add_argument('-ep', '--engine-path', type=pathlib.Path, required=False,
                       help='The path to the engine.')
    group.add_argument('-ed', '--external-subdirectories', type=str, required=False, nargs='*',
                       help='Additional list of subdirectories.')
    group.add_argument('-gpof', '--gem-paths-output-file', type=pathlib.Path, required=False,
                       help='The path to the resolved gem paths output file. If not provided, the list will be output to STDOUT.')
    parser.set_defaults(func=_resolve_gem_dependency_paths)

def main():
    the_parser = argparse.ArgumentParser()
    add_parser_args(the_parser)
    the_args = the_parser.parse_args()
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1
    sys.exit(ret)

if __name__ == "__main__":
    main()
