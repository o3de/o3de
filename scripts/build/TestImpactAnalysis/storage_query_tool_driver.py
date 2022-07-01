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

    def valid_path(value):
        if pathlib.Path(value).is_dir():
            return value
        else:
            raise FileNotFoundError(value)

    parser = argparse.ArgumentParser()

    parser.add_argument(
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

    parser.add_argument(
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

    parser.add_argument(
        "--delete",
        action="store_true",
        help="Flag to delete file if found",
        required=False
    )

    parser.add_argument(
        "--read",
        action="store_true",
        help="Flag to access file if found",
        required=False
    )

    parser.add_argument(
        "--update",
        action="store_true",
        help="Flag to replace file if found",
        required=False
    )

    parser.add_argument(
        "--create",
        action="store_true",
        help="Flag to create file if not found",
        required=False
    )

    parser.add_argument(
        "--file-in",
        type=valid_file_path,
        help="Path to file to be used when creating or updating a file",
        required=False
    )

    parser.add_argument(
        "--file-out",
        type=valid_path,
        help="Path to store file in when downloading",
        required=False
    )


    args = parser.parse_args()

    return args



if __name__ == "__main__":
    try:
        args = vars(parse_args())

        if args.get('local'):
            sqt = LocalStorageQueryTool(**args)
        
        if args.get('s3'):
            sqt = S3StorageQueryTool(**args)

    except Exception as e:
        logger.error(type(e).__name__)
        logger.error(e.args)
        logger.error(f"Exception caught by TIAF driver: '{e}'.")