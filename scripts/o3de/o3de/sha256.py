#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import json
import logging
import hashlib
import pathlib
import sys

from o3de import utils

logger = logging.getLogger('o3de.sha256')
logging.basicConfig(format=utils.LOG_FORMAT)


def sha256(file_path: str or pathlib.Path,
           json_path: str or pathlib.Path = None) -> int:
    if not file_path:
        logger.error(f'File path cannot be empty.')
        return 1
    file_path = pathlib.Path(file_path).resolve()
    if not file_path.is_file():
        logger.error(f'File path {file_path} does not exist.')
        return 1

    if json_path:
        json_path = pathlib.Path(json_path).resolve()
        if not json_path.is_file():
            logger.error(f'Json path {json_path} does not exist.')
            return 1

    the_sha256 = hashlib.sha256(file_path.open('rb').read()).hexdigest()

    if json_path:
        with json_path.open('r') as s:
            try:
                json_data = json.load(s)
            except json.JSONDecodeError as e:
                logger.error(f'Failed to read Json path {json_path}: {str(e)}')
                return 1
        json_data.update({"sha256": the_sha256})
        utils.backup_file(json_path)
        with json_path.open('w') as s:
            try:
                s.write(json.dumps(json_data, indent=4) + '\n')
            except OSError as e:
                logger.error(f'Failed to write Json path {json_path}: {str(e)}')
                return 1
    else:
        print(the_sha256)
    return 0


def _run_sha256(args: argparse) -> int:
    return sha256(args.file_path,
                  args.json_path)


def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python sha256.py --file-path "C:/TestGem"
    :param parser: the caller passes an argparse parser like instance to this method
    """
    parser.add_argument('-f', '--file-path', type=str, required=True,
                        help='The path to the file you want to sha256.')
    parser.add_argument('-j', '--json-path', type=str, required=False,
                        help='Optional path to an o3de json file to add the "sha256" element to.')
    parser.set_defaults(func=_run_sha256)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py sha256  --file-path "C:/TestGem"
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    sha256_subparser = subparsers.add_parser('sha256')
    add_parser_args(sha256_subparser)


def main():
    """
    Runs sha256.py script as standalone script
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
