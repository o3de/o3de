#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
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

    if args.sequence_type == SequenceType.TEST_IMPACT_ANALYSIS and args.dst_commit == None:
        raise ValueError("Test impact analysis sequence must have a change list")
    
    return args

if __name__ == "__main__":
    try:
        args = parse_args()
        tiaf = TestImpact(args.config, args.pipeline, args.dst_commit)
        return_code = tiaf.run(args.suite, args.test_failure_policy, args.safe_mode, args.test_timeout, args.global_timeout)
        # Non-gating will be removed from this script and handled at the job level in SPEC-7413
        #sys.exit(return_code)
        sys.exit(0)
    except:
        # Non-gating will be removed from this script and handled at the job level in SPEC-7413
        sys.exit(0)