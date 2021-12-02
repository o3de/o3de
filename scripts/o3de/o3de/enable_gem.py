#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
Contains command to add a gem to a project's enabled_gem.cmake file
"""

import argparse
import json
import logging
import os
import pathlib
import sys

from o3de import cmake, manifest, register, validation, utils

logger = logging.getLogger('o3de.enable_gem')
logging.basicConfig(format=utils.LOG_FORMAT)


def enable_gem_in_project(gem_name: str = None,
                          gem_path: pathlib.Path = None,
                          project_name: str = None,
                          project_path: pathlib.Path = None,
                          enabled_gem_file: pathlib.Path = None) -> int:
    """
    enable a gem in a projects enabled_gems.cmake file
    :param gem_name: name of the gem to add
    :param gem_path: path to the gem to add
    :param project_name: name of to the project to add the gem to
    :param project_path: path to the project to add the gem to
    :param enabled_gem_file_file: if this dependency goes/is in a specific file
    :return: 0 for success or non 0 failure code
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
                     f' {str(pathlib.Path.home() / ".o3de/manifest.json")}, engine.json')
        return 1

    project_path = pathlib.Path(project_path).resolve()
    if not project_path.is_dir():
        logger.error(f'Project path {project_path} is not a folder.')
        return 1

    # we need either a gem name or path
    if not gem_name and not gem_path:
        logger.error(f'Must either specify a Gem path or Gem Name.')
        return 1

    # if gem name resolve it into a path
    if gem_name and not gem_path:
        gem_path = manifest.get_registered(gem_name=gem_name, project_path=project_path)
    if not gem_path:
        logger.error(f'Unable to locate gem path from the registered manifest.json files:'
                     f' {str(pathlib.Path( "~/.o3de/o3de_manifest.json").expanduser())},'
                     f' {project_path / "project.json"}, engine.json')
        return 1

    gem_path = pathlib.Path(gem_path).resolve()
    # make sure this gem already exists if we're adding.  We can always remove a gem.
    if not gem_path.is_dir():
        logger.error(f'Gem Path {gem_path} does not exist.')
        return 1

    # Read gem.json from the gem path
    gem_json_data = manifest.get_gem_json_data(gem_path=gem_path, project_path=project_path)
    if not gem_json_data:
        logger.error(f'Could not read gem.json content under {gem_path}.')
        return 1


    ret_val = 0
    if enabled_gem_file:
        # make sure this is a project has an enabled gems file
        if not enabled_gem_file.is_file():
            logger.error(f'Enabled gem file {enabled_gem_file} is not present.')
            return 1
        project_enabled_gem_file = enabled_gem_file

    else:
        # Find the path to enabled gem file.
        # It will be created if it doesn't exist
        project_enabled_gem_file = cmake.get_enabled_gem_cmake_file(project_path=project_path)
        if not project_enabled_gem_file.is_file():
            project_enabled_gem_file.touch()

    # Before adding the gem_dependency check if the project is registered in either the project or engine
    # manifest
    buildable_gems = manifest.get_engine_gems()
    buildable_gems.extend(manifest.get_project_gems(project_path))
    # Convert each path to pathlib.Path object and filter out duplictes using dict.fromkeys
    buildable_gems = list(dict.fromkeys(map(lambda gem_path_string: pathlib.Path(gem_path_string), buildable_gems)))

    ret_val = 0
    # If the gem is not part of buildable set, it needs to be registered
    if not gem_path in buildable_gems:
        ret_val = register.register(gem_path=gem_path, external_subdir_project_path=project_path)

    # add the gem if it is registered in either the project.json or engine.json
    ret_val = ret_val or cmake.add_gem_dependency(project_enabled_gem_file, gem_json_data['gem_name'])

    return ret_val


def _run_enable_gem_in_project(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    return enable_gem_in_project(args.gem_name,
                                 args.gem_path,
                                 args.project_name,
                                 args.project_path,
                                 args.enabled_gem_file)


def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python enable_gem.py --project-path "D:/TestProject" --gem-path "D:/TestGem"
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
    parser.add_argument('-egf', '--enabled-gem-file', type=pathlib.Path, required=False,
                                   help='The cmake enabled_gem file in which the gem names are specified.'
                                        'If not specified it will assume enabled_gems.cmake')

    parser.add_argument('-ohf', '--override-home-folder', type=pathlib.Path, required=False,
                                   help='By default the home folder is the user folder, override it to this folder.')

    parser.set_defaults(func=_run_enable_gem_in_project)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py add-gem-to-project --project-path "D:/TestProject" --gem-path "D:/TestGem"
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    enable_gem_project_subparser = subparsers.add_parser('enable-gem')
    add_parser_args(enable_gem_project_subparser)


def main():
    """
    Runs enable_gem.py script as standalone script
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
