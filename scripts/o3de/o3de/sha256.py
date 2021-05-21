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
import json
import logging
import hashlib

from o3de import utils

logger = logging.getLogger()
logging.basicConfig()


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

    sha256 = hashlib.sha256(file_path.open('rb').read()).hexdigest()

    if json_path:
        with json_path.open('r') as s:
            try:
                json_data = json.load(s)
            except Exception as e:
                logger.error(f'Failed to read Json path {json_path}: {str(e)}')
                return 1
        json_data.update({"sha256": sha256})
        utils.backup_file(json_path)
        with json_path.open('w') as s:
            try:
                s.write(json.dumps(json_data, indent=4))
            except Exception as e:
                logger.error(f'Failed to write Json path {json_path}: {str(e)}')
                return 1
    else:
        print(sha256)
    return 0


def _run_sha256(args: argparse) -> int:
    return sha256(args.file_path,
                  args.json_path)


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
    sha256_subparser = subparsers.add_parser('sha256')
    sha256_subparser.add_argument('-f', '--file-path', type=str, required=True,
                                  help='The path to the file you want to sha256.')
    sha256_subparser.add_argument('-j', '--json-path', type=str, required=False,
                                  help='optional path to an o3de json file to add the "sha256" element to.')
    sha256_subparser.set_defaults(func=_run_sha256)
