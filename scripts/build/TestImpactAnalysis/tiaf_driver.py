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
from tiaf import SequenceType

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

    def sequence_type(value):
        if value == "regular":
            return SequenceType.REGULAR
        elif value == "tia":
            return SequenceType.TEST_IMPACT_ANALYSIS
        elif value == "seed":
            return SequenceType.SEED
        else:
            raise ValueError(value)

    def timout_type(value):
        value = int(value)
        if value <= 0:
            raise ValueError("Timer values must be positive integers")
        return value
            
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', dest="config", type=file_path, help="Path to the test impact analysis framework configuration file", required=True)
    parser.add_argument('--destCommit', dest="dst_commit", help="Commit to run test impact analysis on (not required for seed)")
    parser.add_argument('--sequenceType', dest="sequence_type", type=sequence_type, help="Test sequence type to run ('regular', 'seed' or 'tia')", required=True)
    parser.add_argument('--suites', dest="suites", nargs='*', help="Suites to include for regular tes sequences (use '*' for all suites)")
    parser.add_argument('--testTimeout', dest="test_timeout", type=timout_type, help="Maximum flight time (in seconds) of any test target before being terminated", required=False)
    parser.add_argument('--globalTimeout', dest="global_timeout", type=timout_type, help="Maximum tun time of the sequence before being terminated", required=False)
    parser.set_defaults(suites="*")
    parser.set_defaults(safe_mode=False)
    parser.set_defaults(test_timeout=None)
    parser.set_defaults(global_timeout=None)
    args = parser.parse_args()

    if args.sequence_type == SequenceType.TEST_IMPACT_ANALYSIS and args.dst_commit == None:
        raise ValueError("Test impact analysis sequence must have a change list")
    
    return args

if __name__ == "__main__":
    args = parse_args()
    tiaf = TestImpact(args.config, args.dst_commit)
    return_code = tiaf.run(args.sequence_type, args.test_timeout, args.global_timeout)
    sys.exit(return_code)