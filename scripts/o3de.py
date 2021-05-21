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
import sys

# As o3de.py shares the same name as the o3de package attempting to use a regular
# from o3de import <module> line tries to import from the current o3de.py script and not the package
# So the current script directory is removed from the sys.path temporary
SCRIPT_DIR_REMOVED = False
SCRIPT_DIR = pathlib.Path(__file__).parent.resolve()
while str(SCRIPT_DIR) in sys.path:
    SCRIPT_DIR_REMOVED = True
    sys.path.remove(str(SCRIPT_DIR))

from o3de import engine_template
from o3de import global_project
from o3de import registration

if SCRIPT_DIR_REMOVED:
    sys.path.insert(0, str(SCRIPT_DIR))


def add_args(parser, subparsers) -> None:
    global_project.add_args(parser, subparsers)
    engine_template.add_args(parser, subparsers)
    registration.add_args(parser, subparsers)


if __name__ == "__main__":
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers
    the_subparsers = the_parser.add_subparsers(help='sub-command help')

    # add args to the parser
    add_args(the_parser, the_subparsers)

    # parse args
    the_args = the_parser.parse_args()

    # if empty print help
    if len(sys.argv) == 1:
        the_parser.print_help(sys.stderr)
        sys.exit(1)

    # run
    ret = the_args.func(the_args)

    # return
    sys.exit(ret)
