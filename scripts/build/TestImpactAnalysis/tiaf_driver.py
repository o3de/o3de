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
from tiaf import TestImpact

import sys
import os
import datetime
import json
import socket

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
    parser.add_argument('--pipeline', dest="pipeline", help="Pipeline the test impact analysis framework is running on", required=True)
    parser.add_argument('--destCommit', dest="dst_commit", help="Commit to run test impact analysis on (ignored when seeding)", required=True)
    parser.add_argument('--suite', dest="suite", help="Test suite to run", required=True)
    parser.add_argument('--testFailurePolicy', dest="test_failure_policy", type=test_failure_policy, help="Test failure policy for regular and test impact sequences (ignored when seeding)", required=True)
    parser.add_argument('--safeMode', dest="safe_mode", action='store_true', help="Run impact analysis tests in safe mode (ignored when seeding)")
    parser.add_argument('--testTimeout', dest="test_timeout", type=timout_type, help="Maximum run time (in seconds) of any test target before being terminated", required=False)
    parser.add_argument('--globalTimeout', dest="global_timeout", type=timout_type, help="Maximum run time of the sequence before being terminated", required=False)
    parser.set_defaults(test_failure_policy="abort")
    parser.set_defaults(test_timeout=None)
    parser.set_defaults(global_timeout=None)
    args = parser.parse_args()
    
    return args

if __name__ == "__main__":
    args = parse_args()
    tiaf = TestImpact(args.config, args.pipeline, args.dst_commit)
    return_code = tiaf.run(args.suite, args.test_failure_policy, args.safe_mode, args.test_timeout, args.global_timeout)
    sys.exit(return_code)
    