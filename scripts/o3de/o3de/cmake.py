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
import enum
import json
import logging
import os
import pathlib
import string
import sys

# add the scripts/o3de directory to the front of the sys.path temporarily to import 
# some o3de python modules
sys.path.insert(0, os.path.dirname(pathlib.Path(__file__).parent))
from o3de import manifest, utils, compatibility
# Remove the temporarily added path
sys.path = sys.path[1:]

logger = logging.getLogger('o3de.cmake')
logging.basicConfig(format=utils.LOG_FORMAT)

TEMPLATE_CMAKE_PRESETS_INCLUDE_JSON = """
{
    "version": 4,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 23,
        "patch": 0
    },
    "include": [
        "${CMakePresetsInclude}"
    ]
}
"""

PROJECT_ENGINE_PRESET_RELATIVE_PATH = pathlib.PurePath('user/cmake/engine/CMakePresets.json')

class UpdatePresetResult(enum.Enum):
    EnginePathAdded = 0
    EnginePathAlreadyIncluded = 1
    Error = 2

def update_cmake_presets_for_project(preset_path: pathlib.PurePath, engine_name: str = '',
                                     engine_version: str = '',
                                     engine_path: pathlib.PurePath or None = None) -> UpdatePresetResult:
    """
    Updates a cmake-presets formated JSON file with an include that points
    to the root CMakePresets.json inside the registered engine
    :param preset_path: path to the file to update with cmake-preset formatted json
    :param engine_name: name of the engine
    :param engine_version: version specifier for the engine.
           if empty string it is not used
    :return: UpdatePresetResult enum with value EnginePathAdded or EnginePathAlreadyIncluded
             if successful
    """
    if not engine_path:
        engine_with_specifier = f'{engine_name}=={engine_version}' if engine_version else engine_name
        engine_path = manifest.get_registered(engine_name=engine_with_specifier)
        if not engine_path:
            logger.error(f'Engine with identifier {engine_with_specifier} is not registered.\n'
                         f'The cmake-presets file at {preset_path} will not be modified')
            return UpdatePresetResult.Error

    engine_cmake_presets_path = engine_path / "CMakePresets.json"
    preset_json = {}
    # Convert the path to a concrete Path option
    preset_path = pathlib.Path(preset_path)
    try:
        with preset_path.open('r') as preset_fp:
            try:
                preset_json = json.load(preset_fp)
            except json.JSONDecodeError as e:
                logger.warning(f'Cannot parse JSON data from cmake-presets file at path "{preset_path}".\n'
                            'The JSON content in the file will be reset to only include the path to the registered engine:\n'
                            f'{str(e)}')
    except OSError as e:
        # It is OK if the preset_path file does not exist
        pass

    # Update an existing preset file if it exist
    if preset_json:
        preset_include_list = preset_json.get('include', [])
        if engine_cmake_presets_path in map(lambda preset_json_include: pathlib.PurePath(preset_json_include), preset_include_list):
            # If the engine_path is already included in the preset file, return without writing to the file
            return UpdatePresetResult.EnginePathAlreadyIncluded

        # Replace all "include" paths in the existing preset file
        # The reason this occurs is to prevent a scenario where previously registered engines
        # are being referenced by this preset file
        preset_json['include'] = [ engine_cmake_presets_path.as_posix() ]
    else:
        try:
            preset_json = json.loads(string.Template(TEMPLATE_CMAKE_PRESETS_INCLUDE_JSON).safe_substitute(
                CMakePresetsInclude=engine_cmake_presets_path.as_posix()))
        except json.JSONDecodeError as e:
            logger.error(f'Failed to substitute engine path {engine_path} into project CMake Presets template')
            return UpdatePresetResult.Error

    result = UpdatePresetResult.EnginePathAdded
    # Write the updated cmake-presets json to the preset_path file
    try:
        preset_path.parent.mkdir(parents=True, exist_ok=True)
        with preset_path.open('w') as preset_fp:
            try:
                preset_fp.write(json.dumps(preset_json, indent=4) + '\n')
                return result
            except OSError as e:
                logger.error(f'Failed to write "{preset_path}" to filesystem: {str(e)}')
                return UpdatePresetResult.Error
    except OSError as e:
        logger.error(f'Failed to open {preset_path} for write: {str(e)}')
        return UpdatePresetResult.Error


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

def _update_project_presets_to_include_engine_presets(args: argparse) -> int:
    project_path = args.project_path
    if not project_path:
        project_path = manifest.get_registered(project_name=args.project_name)
        if not project_path:
            logger.error(f'Project with name {args.project_name} is not registered')
            return 1

    # Form the path the CMakePresets.json that will include the engine presets
    preset_path = project_path / PROJECT_ENGINE_PRESET_RELATIVE_PATH

    # Map boolean non-error result to a return code of 0
    return 0 if update_cmake_presets_for_project(
        preset_path=preset_path,
        engine_name=args.engine_name,
        engine_version=args.engine_version,
        engine_path=args.engine_path) != UpdatePresetResult.Error else 1

def add_args(subparsers) -> None:
    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked locally or aggregated by a central python file.
    Ex. Directly run from this file alone with: <engine-root>/python/python cmake.py resolve-gem-dependencies --pp <path-to-project>
    OR
    o3de.py can aggregate commands by importing cmake,
    call add_args and execute:  <engine-root>/python/python o3de.py resolve-gem-dependencies --pp <path-to-project>
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    # Add command for resolving gem depenencies
    gem_dependencies_parser = subparsers.add_parser('resolve-gem-dependencies')
    group = gem_dependencies_parser.add_argument_group("resolve gem dependencies")
    group.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                       help='The path to the project.')
    group.add_argument('-ep', '--engine-path', type=pathlib.Path, required=False,
                       help='The path to the engine.')
    group.add_argument('-ed', '--external-subdirectories', type=str, required=False, nargs='*',
                       help='Additional list of subdirectories.')
    group.add_argument('-gpof', '--gem-paths-output-file', type=pathlib.Path, required=False,
                       help='The path to the resolved gem paths output file. If not provided, the list will be output to STDOUT.')
    gem_dependencies_parser.set_defaults(func=_resolve_gem_dependency_paths)

    # Add command for updating the project presets
    update_cmake_presets_for_project_parser = subparsers.add_parser('update-cmake-presets-for-project')
    project_group = update_cmake_presets_for_project_parser.add_mutually_exclusive_group(required=True)
    project_group.add_argument('--project-path', '-pp', type=pathlib.Path,
                               help='The path to a project.')
    project_group.add_argument('--project-name', '-pn', type=str,
                               help='The name of a project.')

    engine_group = update_cmake_presets_for_project_parser.add_argument_group('engine identifiers')
    # The --engine-path and --engine-name arguments are mutually exclusive and one of the are required
    engine_path_group = engine_group.add_mutually_exclusive_group(required=True)
    engine_path_group.add_argument('--engine-path', '-ep', type=pathlib.Path,
                                   help='The path to the engine.')
    engine_path_group.add_argument('--engine-name', '-en', type=str,
                                   help='The name of the engine use to lookup the engine path.')
    # Add the --engine-version argument directly to the `engine_group` variable
    engine_group.add_argument('--engine-version', '-ev', type=str,
                              help='Version of the engine to query when the --engine-name argument is used')


    update_cmake_presets_for_project_parser.set_defaults(func=_update_project_presets_to_include_engine_presets)

def main():
    the_parser = argparse.ArgumentParser()

    script_name = pathlib.PurePath(__file__).name
    cmake_subparsers = the_parser.add_subparsers(
        help=f'To get help on a sub-command:\n{script_name} <sub-command> -h',
        title='Sub-Commands', dest='command', required=True)
    add_args(cmake_subparsers)

    the_args = the_parser.parse_args()
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1
    sys.exit(ret)

if __name__ == "__main__":
    main()
