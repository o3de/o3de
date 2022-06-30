#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import pathlib
from tiaf_logger import get_logger
from s3_storage_query_tool import S3StorageQueryTool
from local_storage_query_tool import LocalStorageQueryTool
logger = get_logger(__file__)


def parse_args():
    def valid_file_path(value):
        if pathlib.Path(value).is_file():
            return value
        else:
            raise FileNotFoundError(value)

    parser = argparse.ArgumentParser()

    storage_type_group = parser.add_mutually_exclusive_group(required=True) 

    storage_type_group.add_argument(
        '--local',
        action="store_true",
        help="Flag to SQT to search locally",
        required=False
    )

    parser.add_argument(
        '--search-in',
        type=valid_file_path,
        help="Directory SQT should search in when searching locally",
        required=False
    )

    storage_type_group.add_argument(
        '--s3',
        action="store_true",
        help="Flag to SQT to search using a bucket",
        required=False
    )

    parser.add_argument(
        '--bucket-name',
        type=str,
        help="Bucket name to search for",
        required=False
    )

    parser.add_argument(
        '--root-directory',
        help="Root directory to search for", 
        required=False
    )

    parser.add_argument(
        '--branch',
        type=str,
        help="Branch name to search for",
        required=False
    )

    parser.add_argument(
        '--suite',
        type=str,
        help="Testing suite to search for",
        required=False
    )

    parser.add_argument(
        '--build',
        type=str,
        help="Build configuration to search for",
        required=False
    )

    args = parser.parse_args()

    return args



if __name__ == "__main__":
    try:
        args = parse_args()
        logger.info(args)

        if args.local:
            sqt = LocalStorageQueryTool(kwargs=args)
        
        if args.s3:
            sqt = S3StorageQueryTool(kwargs=args)

    except Exception as e:
        logger.error(f"Exception caught by TIAF driver: '{e}'.")