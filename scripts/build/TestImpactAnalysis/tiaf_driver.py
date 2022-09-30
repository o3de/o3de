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
from test_impact import NativeTestImpact, PythonTestImpact
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
            raise ValueError(
                "Test failure policy must be 'abort', 'continue' or 'ignore'")

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
        action='store_const',
        const="on",
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

    # Target exclusion JSON file
    parser.add_argument(
        '--exclude-file',
        type=valid_file_path,
        help="Path to file containing tests to exclude from this run",
        required=False
    )

    # Runtime type (native or python)
    parser.add_argument(
        "--runtime-type",
        choices=SUPPORTED_RUNTIMES.keys(),
        help="The runtime TIAF should run tests for",
        required=True
    )

    # Runtime sequence override
    parser.add_argument(
        "--sequence-override",
        help="Override sequence type",
        required=False
    )

    # Python test runner policy
    parser.add_argument(
        "--testrunner-policy",
        choices=["live","null"],
        help="Test runner policy for TIAF",
        required=False
    )

    # Test target output routing
    parser.add_argument(
        "--target-output",
        choices=["stdout"],
        help="Test target std/error output routing",
        required=False
    )

    args = parser.parse_args()

    return args


SUPPORTED_RUNTIMES = {
    "python": PythonTestImpact,
    "native": NativeTestImpact
}


def main(args: dict):
    try:
        tiaf_class = SUPPORTED_RUNTIMES[args.pop("runtime_type")]
        tiaf = tiaf_class(args)
        tiaf_result = tiaf.run()
        if args.get('mars_index_prefix'):
            logger.info("Transmitting report to MARS...")
            mars_utils.transmit_report_to_mars(
                args['mars_index_prefix'], tiaf_result, sys.argv, args['build_number'])

        logger.info("Complete!")
        # Non-gating will be removed from this script and handled at the job level in SPEC-7413
        # sys.exit(result.return_code)
        sys.exit(0)
    except Exception as e:
        # Non-gating will be removed from this script and handled at the job level in SPEC-7413
        logger.error(f"Exception caught by TIAF driver: '{e}'.")
        traceback.print_exc()
    finally:
        # This will not gate the AR run - replace with result.return_code if you wish to enable gating.
        sys.exit(0)


if __name__ == "__main__":

    args = vars(parse_args())
    main(args)
