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
import re
import pathlib
import json
from o3de import manifest

logger = logging.getLogger()
logging.basicConfig()


def set_global_project(project_name: str or None,
                        project_path: str or pathlib.Path or None) -> int:
    """
    set what the current project is
    :param project_name: the name of the project you want to set, resolves project_path
    :param project_path: the path of the project you want to set
    :return: 0 for success or non 0 failure code
    """
    if project_path and project_name:
        logger.error(f'Project Name and Project Path provided, these are mutually exclusive.')
        return 1

    if not project_name and not project_path:
        logger.error('Must specify either a Project name or Project Path.')
        return 1

    if project_name and not project_path:
        project_path = manifest.get_registered(project_name=project_name)

    if not project_path:
        logger.error(f'Project Path {project_path} has not been registered.')
        return 1

    project_path = pathlib.Path(project_path).resolve()

    bootstrap_setreg_file = manifest.get_o3de_registry_folder() / 'bootstrap.setreg'
    if bootstrap_setreg_file.is_file():
        with bootstrap_setreg_file.open('r') as f:
            try:
                json_data = json.load(f)
            except json.JSONDecodeError as e:
                logger.error(f'Bootstrap.setreg failed to load: {str(e)}')
            else:
                try:
                    json_data["Amazon"]["AzCore"]["Bootstrap"]["project_path"] = project_path
                except KeyError as e:
                    logger.error(f'Bootstrap.setreg failed to load: {str(e)}')
                else:
                    try:
                        os.unlink(bootstrap_setreg_file)
                    except OSError as e:
                        logger.error(f'Failed to unlink bootstrap file {bootstrap_setreg_file}: {str(e)}')
                        return 1
    else:
        json_data = {}
        json_data.update({"Amazon":{"AzCore":{"Bootstrap":{"project_path":project_path.as_posix()}}}})

    with bootstrap_setreg_file.open('w') as s:
        s.write(json.dumps(json_data, indent=4))

    return 0


def get_global_project() -> pathlib.Path or None:
    """
    get what the current project set is
    :return: project_path or None on failure
    """
    bootstrap_setreg_file = manifest.get_o3de_registry_folder() / 'bootstrap.setreg'
    if not bootstrap_setreg_file.is_file():
        logger.error(f'Bootstrap.setreg file {bootstrap_setreg_file} does not exist.')
        return None

    with bootstrap_setreg_file.open('r') as f:
        try:
            json_data = json.load(f)
        except json.JSONDecodeError as e:
            logger.error(f'Bootstrap.setreg failed to load: {str(e)}')
        else:
            try:
                project_path = json_data["Amazon"]["AzCore"]["Bootstrap"]["project_path"]
            except KeyError as e:
                logger.error(f'Bootstrap.setreg cannot find Amazon:AzCore:Bootstrap:project_path: {str(e)}')
            else:
                return pathlib.Path(project_path).resolve()
    return None

def _run_get_global_project(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    project_path = get_global_project()
    if project_path:
        print(project_path.as_posix())
        return 0
    return 1


def _run_set_global_project(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    return set_global_project(args.project_name,
                               args.project_path)


def add_args(subparsers) -> None:
    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked locally or aggregated by a central python file.
    Ex. Directly run from this file alone with: python global_project.py set_global_project --project-name TestProject
    OR
    o3de.py can aggregate commands by importing global_project, call add_args and
    execute: python o3de.py set_global_project --project-path C:/TestProject
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    get_global_project_subparser = subparsers.add_parser('get-global-project')
    get_global_project_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                        help='By default the home folder is the user folder, override it to this folder.')
    
    get_global_project_subparser.set_defaults(func=_run_get_global_project)

    set_global_project_subparser = subparsers.add_parser('set-global-project')
    group = set_global_project_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-pn', '--project-name', required=False,
                       help='The name of the project. If supplied this will resolve the --project-path.')
    group.add_argument('-pp', '--project-path', required=False,
                       help='The path to the project')

    set_global_project_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                       help='By default the home folder is the user folder, override it to this folder.')

    set_global_project_subparser.set_defaults(func=_run_set_global_project)


if __name__ == "__main__":
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers
    the_subparsers = the_parser.add_subparsers(help='sub-command help', dest='command', required=True)

    # add args to the parser
    add_args(the_subparsers)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1

    # return
    sys.exit(ret)
