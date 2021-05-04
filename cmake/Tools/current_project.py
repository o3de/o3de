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
import cmake.Tools.registration as registration

logger = logging.getLogger()
logging.basicConfig()


def set_current_project(engine_name: str or None,
                        engine_path: str or pathlib.Path or None,
                        project_name: str or None,
                        project_path: str or pathlib.Path or None) -> int:
    """
    set what the current project is
    :param engine_name: the name of the engine
    :param engine_path: the path of the engine, default is this engine
    :param project_name: the name of the project you want to set
    :param project_path: the path of the project you want to set
    :return: 0 for success or non 0 failure code
    """
    if project_path and project_name:
        logger.error(f'Project Name and Project Path provided, these are mutually exclusive.')
        return 1

    if not engine_name and not engine_path:
        engine_path = registration.get_this_engine_path()

    if engine_name and not engine_path:
        engine_path = registration.get_registered(engine_name=engine_name)

    if not engine_path:
        logger.error(f'Engine path cannot be empty.')
        return 1

    engine_path = pathlib.Path(engine_path).resolve()

    if not engine_path.is_dir():
        logger.error(f'Engine path {engine_path} does not exist.')
        return 1

    bootstrap_cfg_file = engine_path / 'bootstrap.cfg'
    if not bootstrap_cfg_file.is_file():
        logger.error(f'Bootstrap file {bootstrap_cfg_file} does not exist.')
        return 1

    if project_name and not project_path:
        project_path = registration.get_registered(project_name=project_name)

    if not project_path:
        logger.error(f'Project path {project_path} cannot be empty.')
        return 1

    project_path = pathlib.Path(project_path).resolve()

    if not project_path.is_dir():
        logger.error(f'Project path {project_path} does not exist.')
        return 1

    with bootstrap_cfg_file.open('r') as s:
        data = s.read()
        try:
            data = re.sub(r'(.*project_path\s*?[=:]\s*?)([^\n]+)\n', r'\1{}\n'.format(project_path.as_posix()),
                          data, flags=re.IGNORECASE)
        except Exception as e:
            logger.error(f'Failed to replace "project_path" in bootstrap file {bootstrap_cfg_file}: {str(e)}')
            return 1
    try:
        os.unlink(bootstrap_cfg_file)
    except Exception as e:
        logger.error(f'Failed to unlink bootstrap file {bootstrap_cfg_file}: {str(e)}')
        return 1
    else:
        with bootstrap_cfg_file.open('w') as s:
            s.write(data)

    return 0


def get_current_project(engine_name: str or None,
                        engine_path: str or pathlib.Path or None) -> pathlib.Path or None:
    """
    get what the current project set is
    :param engine_name: the name of the engine
    :param engine_path: the path of the engine, default is this engine
    :return: project_path or None on failure
    """
    if engine_name and engine_path:
        logger.error(f'Engine Name and Engine Path provided, these are mutually exclusive.')
        return 1

    if not engine_name and not engine_path:
        engine_path = registration.get_this_engine_path()

    if engine_name:
        engine_path = registration.get_registered(engine_name=engine_name)

    if not engine_path:
        logger.error('Engine path cannot be empty.')
        return None

    engine_path = pathlib.Path(engine_path).resolve()

    if not engine_path.is_dir():
        logger.error(f'Engine path {engine_path} does not exist.')
        return None

    bootstrap_cfg_file = engine_path / 'bootstrap.cfg'
    if not bootstrap_cfg_file.is_file():
        logger.error(f'Bootstrap file {bootstrap_cfg_file} does not exist.')
        return None

    with bootstrap_cfg_file.open('r') as s:
        data = s.read()
        try:
            project_path = re.search(r'(.*project_path\s*?[=:]\s*?)(?P<project_path>[^\n]+)\n',
                                 data, flags=re.IGNORECASE).group('project_path').strip()
        except Exception as e:
            logger.error(f'Failed to read "project_path" from bootstrap file {bootstrap_cfg_file}.')
            return None

    project_path = pathlib.Path(project_path).resolve()

    return project_path


def _run_get_current_project(args: argparse) -> int:
    project_path = get_current_project(args.engine_name,
                                       args.engine_path)
    if project_path:
        print(project_path)
        return 0
    return 1


def _run_set_current_project(args: argparse) -> int:
    if args.override_home_folder:
        registration.override_home_folder = args.override_home_folder

    return set_current_project(args.engine_name,
                               args.engine_path,
                               args.project_name,
                               args.project_path)


def add_args(parser, subparsers) -> None:
    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked locally or aggregated by a central python file.
    Ex. Directly run from this file alone with: python current_project.py set_current_project --project-name TestProject
    OR
    o3de.py can aggregate commands by importing current_project, call add_args and
    execute: python o3de.py set_current_project --project-path C:/TestProject
    :param parser: the caller instantiates a parser and passes it in here
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    get_current_project_subparser = subparsers.add_parser('get-current-project')
    group = get_current_project_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-en', '--engine-name', required=False,
                       help='The name of the engine. If supplied this will resolve the --engine-path.')
    group.add_argument('-ep', '--engine-path', required=False,
                       help='The path to the engine.')

    get_current_project_subparser.set_defaults(func=_run_get_current_project)


    set_current_project_subparser = subparsers.add_parser('set-current-project')
    group = set_current_project_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-en', '--engine-name', required=False,
                       help='The name of the engine. If supplied this will resolve the --engine-path.')
    group.add_argument('-ep', '--engine-path', required=False,
                       help='The path to the engine')
    group = set_current_project_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-pn', '--project-name', required=False,
                       help='The name of the project. If supplied this will resolve the --project-path.')
    group.add_argument('-pp', '--project-path', required=False,
                       help='The path to the project')

    set_current_project_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                       help='By default the home folder is the user folder, override it to this folder.')

    set_current_project_subparser.set_defaults(func=_run_set_current_project)


if __name__ == "__main__":
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers
    the_subparsers = the_parser.add_subparsers(help='sub-command help')

    # add args to the parser
    add_args(the_parser, the_subparsers)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args)

    # return
    sys.exit(ret)
