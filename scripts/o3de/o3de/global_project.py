#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import logging
import os
import sys
import re
import pathlib
import json

from o3de import manifest, validation, utils

logger = logging.getLogger('o3de.global_project')
logging.basicConfig(format=utils.LOG_FORMAT)

DEFAULT_BOOTSTRAP_SETREG = pathlib.Path('~/.o3de/Registry/bootstrap.setreg').expanduser()
PROJECT_PATH_KEY = ('Amazon', 'AzCore', 'Bootstrap', 'project_path')


def get_json_data(input_path: pathlib.Path):
    setreg_json_data = {}
    # If the output_path exist validate that it is a valid json file
    if input_path.is_file():
        with input_path.open('r') as f:
            try:
                setreg_json_data = json.load(f)
            except json.JSONDecodeError as e:
                logger.error(f'The file: {input_path} is not a valid json file: {str(e)}')

    return setreg_json_data

def set_global_project(output_path: pathlib.Path,
                          project_name: str = None,
                          project_path: pathlib.Path = None,
                          force: bool = False) -> int:
    """
    Adds a project path the a settings registry file in the users ~/.o3de/Registry directory
    :param output_path: path to .setreg file to store project_path value into
    :param project_name: name of the project to lookup path for
    :param project_path: path to the project to add to .setreg file
    :param force: if set, the project path will be set within the .setreg file regardless of if the path doesn't exist
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
        logger.error(
            f'The project name has been supplied. Unable to locate project path from the registered manifest.json files:'
            f' {str(pathlib.Path("~/.o3de/o3de_manifest.json").expanduser())}, engine.json\n'
            'A The --project-path parameter can be used directly to skip checking the manifest')
        return 1

    # Only perform project path validations when force=False
    if not force:
        if not project_path.is_dir():
            logger.error(f'Project path {project_path} is not a folder.')
            return 1

        # Validate that the supplied path points contains a valid project.json
        if not validation.valid_o3de_project_json(project_path / 'project.json'):
            logger.error(f'The supplied project path does not contain a valid project.json.\n'
                         f'The Path will not be set')
            return 1

    # If the output_path exist validate that it is a valid json file and read it's json data
    setreg_json_data = get_json_data(output_path)
    if output_path.is_file():
        with output_path.open('r') as f:
            try:
                setreg_json_data = json.load(f)
            except (json.JSONDecodeError) as e:
                logger.error(f'The output file: {output_path} is not a valid json file: {str(e)}')
                return 1

    # Add a json dictionary that will be merged with any existing json data from the .setreg file
    merge_json_data = {}
    json_object_iter = merge_json_data
    for json_key in PROJECT_PATH_KEY[:-1]:
        # Add the parent json object for the key to update
        json_object_iter = json_object_iter.setdefault(json_key, {})

    # Set the project path value here
    json_object_iter[PROJECT_PATH_KEY[-1]] = project_path.as_posix()
    setreg_json_data.update(merge_json_data)

    # Create the parent directories
    if output_path.parent:
        output_path.parent.mkdir(parents=True, exist_ok=True)
    try:
        with output_path.open('w') as s:
            s.write(json.dumps(setreg_json_data, indent=4) + '\n')
    except OSError as e:
        logger.error(f'Failed to write project path {project_path} to file {output_path}: {str(e)}')
        return 1

    return 0


def get_global_project(input_path: pathlib.Path) -> pathlib.Path or None:
    """
    Retrieves the /Amazon/AzCore/Bootstrap/project_path key from the supplied file path
    :return: project_path or None on failure
    """
    setreg_json_data = get_json_data(input_path)

    try:
        # Iterate over each element of the tuple and read the json key from each successive json object
        json_object_iter = setreg_json_data
        for json_key in PROJECT_PATH_KEY:
            json_object_iter = json_object_iter[json_key]
    except KeyError as e:
        logger.error(f'Cannot read key /{"/".join(PROJECT_PATH_KEY)} from file {input_path.as_posix()}: {str(e)}')
    else:
        project_path = json_object_iter
        return pathlib.Path(project_path).resolve()
    return None

def _run_get_global_project(args: argparse) -> int:
    project_path = get_global_project(args.input_path)
    if project_path:
        print(project_path.as_posix())
        return 0
    return 1


def _run_set_global_project(args: argparse) -> int:
    return set_global_project(args.output_path,
                                 args.project_name,
                                 args.project_path,
                                 args.force)


def add_parser_args(get_project_parser, set_project_parser):
    """
        add_parser_args is called to add arguments to each command such that it can be
        invoked locally or added by a central python file.
        Ex. Directly run from this file alone with: python global_project.py --project-path "D:/TestProject"
        :param parser: the caller passes an argparse parser like instance to this method
    """

    # get-current-project
    get_project_parser.add_argument('-i', '--input-path', type=pathlib.Path, required=False, default=DEFAULT_BOOTSTRAP_SETREG,
                        help=f'Optional path to file to read /{"/".join(PROJECT_PATH_KEY)} key from.'
                             f' If not supplied, then {DEFAULT_BOOTSTRAP_SETREG} is used instead')
    get_project_parser.set_defaults(func=_run_get_global_project)

    # set-current-project
    group = set_project_parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                       help='The path to the project.')
    group.add_argument('-pn', '--project-name', type=str, required=False,
                       help='The name of the project.')
    set_project_parser.add_argument('-o', '--output-path', type=pathlib.Path, required=False,
                                    default=DEFAULT_BOOTSTRAP_SETREG,
                                    help=f'Optional path to output file to write project_path key to. '
                                         f'If not supplied, then {DEFAULT_BOOTSTRAP_SETREG} is used instead')
    set_project_parser.add_argument('-f', '--force', action='store_true', default=False,
                                    help=f'Force the setting of the project path in the supplied setreg file')
    set_project_parser.set_defaults(func=_run_set_global_project)

def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py set-global-project --project-path "D:/TestProject"
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    get_project_subparser = subparsers.add_parser('get-global-project')
    set_project_subparser = subparsers.add_parser('set-global-project')
    add_parser_args(get_project_subparser, set_project_subparser)


def main():
    """
    Runs this script as standalone script
    """
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers
    project_subparsers = the_parser.add_subparsers(help="Commands for modifying the project path in the user's home"
                                                        " setreg files")

    # add args to the parser
    add_args(project_subparsers)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1

    # return
    sys.exit(ret)


if __name__ == "__main__":
    main()
