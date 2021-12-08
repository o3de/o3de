#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import pathlib
import sys

from o3de import manifest


def _run_get_registered(args: argparse) -> str or pathlib.Path:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    return manifest.get_registered(args.engine_name,
                                   args.project_name,
                                   args.gem_name,
                                   args.template_name,
                                   args.default_folder,
                                   args.repo_name,
                                   args.restricted_name)


def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python get_registration.py --engine-name "o3de"
    :param parser: the caller passes an argparse parser like instance to this method
    """
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-en', '--engine-name', type=str, required=False,
                       help='Engine name.')
    group.add_argument('-pn', '--project-name', type=str, required=False,
                       help='Project name.')
    group.add_argument('-gn', '--gem-name', type=str, required=False,
                       help='Gem name.')
    group.add_argument('-tn', '--template-name', type=str, required=False,
                       help='Template name.')
    group.add_argument('-df', '--default-folder', type=str, required=False,
                       choices=['engines', 'projects', 'gems', 'templates', 'restricted'],
                       help='The default folders for o3de.')
    group.add_argument('-rn', '--repo-name', type=str, required=False,
                       help='Repo name.')
    group.add_argument('-rsn', '--restricted-name', type=str, required=False,
                       help='Restricted name.')

    parser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                        help='By default the home folder is the user folder, override it to this folder.')

    parser.set_defaults(func=_run_get_registered)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py get-registered --engine-name "o3de"
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    get_registered_subparser = subparsers.add_parser('get-registered')
    add_parser_args(get_registered_subparser)


def main():
    """
    Runs get_registration.py script as standalone script
    """
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers

    # add args to the parser
    add_parser_args(the_parser)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1

    # return
    sys.exit(ret)


if __name__ == "__main__":
    main()
