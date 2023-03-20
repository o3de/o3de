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
from storage_query_tool import S3StorageQueryTool
from storage_query_tool import LocalStorageQueryTool
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
        '--search-in',
        type=str,
        help="Directory SQT should search in when searching locally, or directory when using S3.",
        required=False
    )

    parser.add_argument(
        '--s3-bucket',
        type=str,
        help="Flag to SQT to use S3, will search in the named bucket.",
        required=False
    )

    parser.add_argument(
        '--full-address',
        type=str,
        help="Full address to desired file, either locally or in bucket.",
        required=False
    )

    parser.add_argument(
        "--action",
        type=str,
        help="What action TIAF Tools should take.",
        choices=["read", "update", "delete", "create"],
        required=False
    )

    parser.add_argument(
        "--file-in",
        type=valid_file_path,
        help="Path to file to be used when creating or updating a file.",
        required=False
    )

    parser.add_argument(
        "--file-out",
        type=valid_path,
        help="Path to directory store file in when downloading.",
        required=False
    )

    parser.add_argument(
        "--file-type",
        choices=["json", "zip"],
        help="What file type SQT should expect to be interacting with. Current options are zip and json.",
        required=True
    )

    return parser.parse_args()

def run(args: dict):
    try:
        if args.get('s3_bucket'):
            sqt = S3StorageQueryTool(**args)
        else:
            if args.get('search_in'):
                if not pathlib.Path(args['search_in']).is_dir():
                    raise FileNotFoundError(args['search_in'])
                sqt = LocalStorageQueryTool(**args)
            elif args.get('full_address'):
                sqt = LocalStorageQueryTool(**args)
            else:
                logger.error(
                    "You must define a repository to search in when searching locally")

    except Exception as e:
        logger.error(type(e).__name__)
        logger.error(e.args)
        logger.error(f"Exception caught by TIAF Tools: '{e}'.")


if __name__ == "__main__":
    parsed_args = vars(parse_args())
    run(args=parsed_args)
