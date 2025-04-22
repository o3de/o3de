#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
Contains methods for removing a gem from a project
"""

import argparse
import logging
import os
import pathlib
import sys

from o3de import cmake, manifest, project_properties, utils

logger = logging.getLogger('o3de.disable_gem')
logging.basicConfig(format=utils.LOG_FORMAT)


def disable_gem_in_project(gem_name: str = None,
                           gem_path: pathlib.Path = None,
                           project_name: str = None,
                           project_path: pathlib.Path = None,
                           enabled_gem_file: pathlib.Path = None) -> int:
    """
    disable a gem in a projects enabled_gems.cmake file
    :param gem_name: name of the gem to add
    :param gem_path: path to the gem to add
    :param project_name: name of the project to add the gem to
    :param project_path: path to the project to add the gem to
    :param enabled_gem_file: File to remove enabled gem from
    :return: 0 for success, 2 if gem was not found, 1 on any other error  
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
                     f' {str(pathlib.Path("~/.o3de/o3de_manifest.json").expanduser())}, engine.json')
        return 1

    project_path = pathlib.Path(project_path).resolve()
    if not project_path.is_dir():
        logger.error(f'Project path {project_path} is not a folder.')
        return 1

    # We need either a gem name or path
    if not gem_name and not gem_path:
        logger.error(f'Must either specify a Gem path or Gem Name.')
        return 1

    # We need a gem name to deactivate in the project
    # The gem doesn't have to exist or be registered for a user
    # to be allowed to deactivate it
    if not gem_name and gem_path:
        gem_path = pathlib.Path(gem_path).resolve()

        # Make sure the gem path is a directory
        if not gem_path.is_dir():
            logger.error(f'Gem Path {gem_path} does not exist. The name of the gem to remove cannot be determined without a valid path.')
            return 1

        gem_json_data = manifest.get_gem_json_data(gem_path=gem_path, project_path=project_path)
        if not gem_json_data:
            logger.error(f'Could not read gem.json content under {gem_path}.')
            return 1

        gem_name = gem_json_data.get('gem_name','')
        if not gem_name:
            logger.error(f'Unable to determine the gem to remove because the gem name is empty in gem.json content under {gem_path}.')
            return 1

    gem_name_without_specifier, version_specifier = utils.get_object_name_and_optional_version_specifier(gem_name)
    ret_val = 0

    # Remove the gem from the deprecated enabled_gems.cmake file
    gem_found_in_cmake = False
    if not enabled_gem_file:
        enabled_gem_file = manifest.get_enabled_gem_cmake_file(project_path=project_path)
    if enabled_gem_file.is_file():
        # enabled_gems.cmake does not support gem names with specifiers
        gem_found_in_cmake = gem_name_without_specifier in manifest.get_enabled_gems(enabled_gem_file)
        if gem_found_in_cmake:
            ret_val = cmake.remove_gem_dependency(enabled_gem_file, gem_name_without_specifier)

    # Remove the name of the gem from the project.json "gem_names" field 
    project_json_data = manifest.get_project_json_data(project_path=project_path)
    if not project_json_data:
        logger.error(f'Could not read project.json content under {project_path}.')
        return 1
    
    gem_names = utils.get_gem_names_set(project_json_data.get('gem_names',[]), include_optional=True)

    # Remove the gem with the exact match, or any entry with the gem name if 
    # they don't include a version specifier 
    gem_found_in_gem_names = gem_name in gem_names or \
        (not version_specifier and utils.contains_object_name(gem_name_without_specifier, gem_names)) 

    if not gem_found_in_cmake and not gem_found_in_gem_names:
        # No gems found to remove with this name
        return 2

    ret_val = project_properties.edit_project_props(project_path,
                                                    delete_gem_names=gem_name) or ret_val

    return ret_val


def remove_explicit_gem_activation_for_all_paths(gem_root_folders: list,
                                                 project_name: str = None,
                                                 project_path: pathlib.Path = None,
                                                 enabled_gem_file: pathlib.Path = None) -> int:
    """
    walks each gem root folder directory structure and removes explicit
    activation of the gems to the project
    :param gem_root_folders: name of the gem to add
    :param project_name: name of to the project to add the gem to
    :param project_path: path to the project to add the gem to
    :param enabled_gem_file: if this dependency goes/is in a specific file
    :return: 0 for success or non 0 failure code
    """
    if not gem_root_folders:
        logger.error('gem_root_folders list cannot be empty')
        return 1

    # Skip removing of explicit activation of gems within template directories
    # They shouldn't be activated anyway
    def stop_on_template_folders(directories: list, filenames: list) -> bool:
        return 'template.json' in filenames

    gem_dirs_set = set()
    ret_val = 0
    for gem_root_folder in gem_root_folders:
        gem_root_folder = pathlib.Path(gem_root_folder).resolve()
        if not gem_root_folder.is_dir():
            logger.error(f'gem root folder of {gem_root_folder} is not a directory')
            ret_val = 1
        for root, dirs, files in os.walk(gem_root_folder):
            # Skip template directories
            if stop_on_template_folders(dirs, files):
                dirs[:] = []
            elif 'gem.json' in files:
                gem_dirs_set.add(pathlib.Path(root))

    for gem_dir in sorted(gem_dirs_set):
        # Run the command to remove explicit activation even if previous calls failed
        ret_val = disable_gem_in_project(gem_path=gem_dir,
                                         project_name=project_name,
                                         project_path=project_path,
                                         enabled_gem_file=enabled_gem_file) or ret_val

    return ret_val


def _run_disable_gem_in_project(args: argparse) -> int:
    if args.all_gem_paths:
        return remove_explicit_gem_activation_for_all_paths(
            args.all_gem_paths,
            args.project_name,
            args.project_path,
            args.enabled_gem_file
        )
    else:
        return disable_gem_in_project(args.gem_name,
                                      args.gem_path,
                                      args.project_name,
                                      args.project_path,
                                      args.enabled_gem_file)


def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python disable_gem.py --project-path D:/Test --gem-name Atom
    :param parser: the caller passes an argparse parser like instance to this method
    """

    # Sub-commands should declare their own verbosity flag, if desired
    utils.add_verbosity_arg(parser)

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
    group.add_argument('-agp', '--all-gem-paths', type=pathlib.Path, nargs='*', required=False,
                       help='Removes explicit activation of all gems in the path recursively.')
    parser.add_argument('-egf', '--enabled-gem-file', type=pathlib.Path, required=False,
                                      help='The cmake enabled gem file in which gem names are to be removed from.'
                                           'If not specified it will assume ')

    parser.set_defaults(func=_run_disable_gem_in_project)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py disable-gem-from-cmake --project-path D:/Test --gem-name Atom
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    disable_gem_project_subparser = subparsers.add_parser('disable-gem')
    add_parser_args(disable_gem_project_subparser)


def main():
    """
    Runs disable_gem_project.py script as standalone script
    """
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add args to the parser
    add_parser_args(the_parser)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1
    logger.info('Success!' if ret == 0 else 'Completed with issues: result {}'.format(ret))

    # return
    sys.exit(ret)


if __name__ == "__main__":
    main()
