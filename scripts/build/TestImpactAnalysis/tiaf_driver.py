#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
from tiaf import TestImpact
import mars_utils
import sys
import os

def parse_args():
    def file_path(value):
        if os.path.isfile(value):
            return value
        else:
            raise FileNotFoundError(value)

    def timout_type(value):
        value = int(value)
        if value <= 0:
            raise ValueError("Timer values must be positive integers")
        return value

    def test_failure_policy(value):
        if value == "continue" or value == "abort" or value == "ignore":
            return value
        else:
            raise ValueError("Test failure policy must be 'abort', 'continue' or 'ignore'")

    parser = argparse.ArgumentParser()
    parser.add_argument('--config', dest="config", type=file_path, help="Path to the test impact analysis framework configuration file", required=True)
    parser.add_argument('--snapshot', dest="snapshot", help="The snapshot being used by the build", required=True)
    parser.add_argument('--branch', dest="src_branch",  help="The branch that is being build", required=True)
    parser.add_argument('--seeding-branches', dest="seeding_branches", type=lambda arg: arg.split(','), help="Comma separated branches that seeding will occur on", required=True)
    parser.add_argument('--pipeline', dest="pipeline", help="Pipeline the test impact analysis framework is running on", required=True)
    parser.add_argument('--seeding-pipelines', dest="seeding_pipelines", type=lambda arg: arg.split(','), help="Comma separated pipeline that seeding will occur on", required=True)
    parser.add_argument('--commit', dest="dst_commit", help="Commit to run test impact analysis on (ignored when seeding)", required=True)
    parser.add_argument('--suite', dest="suite", help="Test suite to run", required=True)
    parser.add_argument('--test-failure-policy', dest="test_failure_policy", type=test_failure_policy, help="Test failure policy for regular and test impact sequences (ignored when seeding)", required=True)
    parser.add_argument('--safe-mode', dest="safe_mode", action='store_true', help="Run impact analysis tests in safe mode (ignored when seeding)")
    parser.add_argument('--test-timeout', dest="test_timeout", type=timout_type, help="Maximum run time (in seconds) of any test target before being terminated", required=False)
    parser.add_argument('--global-timeout', dest="global_timeout", type=timout_type, help="Maximum run time of the sequence before being terminated", required=False)
    parser.set_defaults(test_timeout=None)
    parser.set_defaults(global_timeout=None)
    args = parser.parse_args()
    
    return args

if __name__ == "__main__":
    
    try:
        args = parse_args()
        tiaf = TestImpact(args.config, args.dst_commit, args.src_branch, args.pipeline, args.seeding_branches, args.seeding_pipelines)
        tiaf_result = tiaf.run(args.suite, args.test_failure_policy, args.safe_mode, args.test_timeout, args.global_timeout)

        filebeat = mars_utils.FilebeatClient("localhost", 9000, 60)
        mars_job = mars_utils.generate_mars_job(tiaf_result, args.snapshot, str(sys.argv))
        filebeat.send_event(mars_job, "jonawals.tiaf.job") # DICTIONARY OF PAYLOAD CONTENTS ONLY

        # Non-gating will be removed from this script and handled at the job level in SPEC-7413
        #sys.exit(result.return_code)
        sys.exit(0)
    except Exception as e:
        # Non-gating will be removed from this script and handled at the job level in SPEC-7413
        print(f"Exception caught by TIAF driver: {e}")
