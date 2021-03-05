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

from cmake.Tools import common

logger = logging.getLogger()
logging.basicConfig()


def set_current_project(dev_root: str,
                        project_path: str) -> int:
    """
    set what the current project is
    :param dev_root: the dev root of the engine
    :param project_path: the path of the project you want to set
    :return: 0 for success or non 0 failure code
    """
    project_path = project_path.strip()
    if not project_path.isalnum():
        logger.error('Project Name invalid. Set current project failed.')
        return 1
    try:
        with open(os.path.join(dev_root, 'bootstrap.cfg'), 'r') as s:
            data = s.read()
        data = re.sub(r'(.*sys_game_folder\s*?[=:]\s*?)([^\n]+)\n', r'\1 {}\n'.format(project_path),
                      data, flags=re.IGNORECASE)
        if os.path.isfile(os.path.join(dev_root, 'bootstrap.cfg')):
            os.unlink(os.path.join(dev_root, 'bootstrap.cfg'))
        with open(os.path.join(dev_root, 'bootstrap.cfg'), 'w') as s:
            s.write(data)
    except Exception as e:
        logger.error('Set current project failed.' + str(e))
        return 1
    return 0


def get_current_project(dev_root: str) -> str:
    """
    get what the current project set is
    :param dev_root: the dev root of the engine
    :return: sys_game_folder or None on failure
    """
    try:
        with open(os.path.join(dev_root, 'bootstrap.cfg'), 'r') as s:
            data = s.read()
        sys_game_folder = re.search(r'(.*sys_game_folder\s*?[=:]\s*?)(?P<sys_game_folder>[^\n]+)\n',
                                    data, flags=re.IGNORECASE).group('sys_game_folder').strip()
    except Exception as e:
        logger.error('Failed to get current project. Exception: ' + str(e))
        return ''
    return sys_game_folder


def _run_get_current_project(args: argparse) -> int:
    sys_game_folder = get_current_project(common.determine_dev_root())
    if sys_game_folder:
        print(sys_game_folder)
        return 0
    return 1


def _run_set_current_project(args: argparse) -> int:
    return set_current_project(common.determine_dev_root(), args.project_path)


def add_args(parser, subparsers) -> None:
    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked locally or aggregated by a central python file.
    Ex. Directly run from this file alone with: python current_project.py set_current_project --project TestProject
    OR
    lmbr.py can aggregate commands by importing current_project, call add_args and
    execute: python lmbr.py set_current_project --project TestProject
    :param parser: the caller instantiates a parser and passes it in here
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    get_current_project_subparser = subparsers.add_parser('get_current_project')
    get_current_project_subparser.set_defaults(func=_run_get_current_project)

    set_current_project_subparser = subparsers.add_parser('set_current_project')
    set_current_project_subparser.add_argument('-pp', '--project-path', required=True,
                                               help='The path to the project, can be absolute or dev root relative')
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
