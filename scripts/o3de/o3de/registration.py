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
"""
This file contains all the code that has to do with registering engines, projects, gems and templates
"""

import argparse
import sys


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
    # register
    from o3de import register
    register.add_args(parser, subparsers)

    # show
    from o3de import print_registration
    print_registration.add_args(parser, subparsers)

    # get-registered
    from o3de import get_registration
    get_registration.add_args(parser, subparsers)

    # download
    from o3de import download
    download.add_args(parser, subparsers)

    # add external subdirectories
    from o3de import add_external_subdirectory
    add_external_subdirectory.add_args(parser, subparsers)

    # remove external subdirectories
    from o3de import remove_external_subdirectory
    remove_external_subdirectory.add_args(parser, subparsers)

    # add gems to cmake
    from o3de import add_gem_cmake
    add_gem_cmake.add_args(parser, subparsers)

    # remove gems from cmake
    from o3de import remove_gem_cmake
    remove_gem_cmake.add_args(parser, subparsers)

    # add a gem to a project
    from o3de import add_gem_project
    add_gem_project.add_args(parser, subparsers)

    # remove a gem from a project
    from o3de import remove_gem_project
    remove_gem_project.add_args(parser, subparsers)

    # sha256
    from o3de import sha256
    sha256.add_args(parser, subparsers)


if __name__ == "__main__":
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers
    the_subparsers = the_parser.add_subparsers(help='sub-command help', dest='command', required=True)

    # add args to the parser
    add_args(the_parser, the_subparsers)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1

    # return
    sys.exit(ret)
