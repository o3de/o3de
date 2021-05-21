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
import pathlib

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


def add_args(parser, subparsers) -> None:
    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python register.py register --gem-path "C:/TestGem"
    OR
    o3de.py can downloadable commands by importing engine_template,
    call add_args and execute: python o3de.py register --gem-path "C:/TestGem"
    :param parser: the caller instantiates a parser and passes it in here
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    get_registered_subparser = subparsers.add_parser('get-registered')
    group = get_registered_subparser.add_mutually_exclusive_group(required=True)
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

    get_registered_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                          help='By default the home folder is the user folder, override it to this folder.')

    get_registered_subparser.set_defaults(func=_run_get_registered)
