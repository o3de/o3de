#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import mars_utils
import sys
import pathlib
import traceback
import re
from tiaf import TestImpact
from tiaf_logger import get_logger

logger = get_logger(__file__)

def parse_args():
    def valid_file_path(value):
        if pathlib.Path(value).is_file():
            return value
        else:
            raise FileNotFoundError(value)

    def valid_timout_type(value):
        value = int(value)
        if value <= 0:
            raise ValueError("Timer values must be positive integers")
        return value

    def valid_test_failure_policy(value):
        if value == "continue" or value == "abort" or value == "ignore":
            return value
        else:
            raise ValueError("Test failure policy must be 'abort', 'continue' or 'ignore'")

    parser = argparse.ArgumentParser()

    # Configuration file path
    parser.add_argument(
        '--config',
        type=valid_file_path,
        help="Path to the test impact analysis framework configuration file", 
        required=True
    )

    # Source branch
    parser.add_argument(
        '--src-branch',
        help="Branch that is being built",
        required=True
    )

    # Destination branch
    parser.add_argument(
        '--dst-branch',
        help="For PR builds, the destination branch to be merged to, otherwise empty", 
        required=False
    )

    # Commit hash
    parser.add_argument(
        '--commit',
        help="Commit that is being built",
        required=True
    )

    # S3 bucket name
    parser.add_argument(
        '--s3-bucket', 
        help="Location of S3 bucket to use for persistent storage, otherwise local disk storage will be used", 
        required=False
    )

    # S3 bucket top level directory
    parser.add_argument(
        '--s3-top-level-dir', 
        help="The top level directory to use in the S3 bucket", 
        required=False
    )

    # MARS index prefix
    parser.add_argument(
        '--mars-index-prefix', 
        help="Index prefix to use for MARS, otherwise no data will be tramsmitted to MARS", 
        required=False
    )

    # Build number
    parser.add_argument(
        '--build-number', 
        help="The build number this run of TIAF corresponds to", 
        required=True
    )

    # Test suite
    parser.add_argument(
        '--suite', 
        help="Test suite to run", 
        required=True
    )

    # Test failure policy
    parser.add_argument(
        '--test-failure-policy', 
        type=valid_test_failure_policy, 
        help="Test failure policy for regular and test impact sequences (ignored when seeding)", 
        required=True
    )

    # Safe mode
    parser.add_argument(
        '--safe-mode', 
        action='store_true', 
        help="Run impact analysis tests in safe mode (ignored when seeding)", 
        required=False
    )

    # Test timeout
    parser.add_argument(
        '--test-timeout', 
        type=valid_timout_type, 
        help="Maximum run time (in seconds) of any test target before being terminated", 
        required=False
    )

    # Global timeout
    parser.add_argument(
        '--global-timeout', 
        type=valid_timout_type, 
        help="Maximum run time of the sequence before being terminated", 
        required=False
    )

    args = parser.parse_args()
    
    return args

if __name__ == "__main__":
    
    try:
        args = parse_args()

        s3_top_level_dir = None
        if args.s3_top_level_dir:
            s3_top_level_dir = args.s3_top_level_dir
        else:
            s3_top_level_dir = "tiaf"

        tiaf = TestImpact(args.config)
        tiaf_result = tiaf.run(args.commit, args.src_branch, args.dst_branch, args.s3_bucket, s3_top_level_dir, args.suite, args.test_failure_policy, args.safe_mode, args.test_timeout, args.global_timeout)
        
        if args.mars_index_prefix:
            logger.info("Transmitting report to MARS...")
            mars_utils.transmit_report_to_mars(args.mars_index_prefix, tiaf_result, sys.argv, args.build_number)

        logger.info("Complete!")
        # Non-gating will be removed from this script and handled at the job level in SPEC-7413
        #sys.exit(result.return_code)
        sys.exit(0)
    except Exception as e:
        # Non-gating will be removed from this script and handled at the job level in SPEC-7413
        logger.error(f"Exception caught by TIAF driver: '{e}'.")
        traceback.print_exc()
