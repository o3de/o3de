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
    def file_path(path):
        if os.path.isfile(path):
            return path
        else:
            raise FileNotFoundError(path)

    def sequence_type(type):
        if type == "regular":
            return SequenceType.REGULAR
        elif type == "tia":
            return SequenceType.TEST_IMPACT_ANALYSIS
        else:
            raise ValueError(type)
            
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', dest="config", type=file_path, help="Path to the test impact analysis framework configuration file", required=True)
    parser.add_argument('-d', '--destCommit', dest="dst_commit", help="Commit to run test impact analysis on (if empty, the most recent commit will be used)")
    parser.add_argument('-t', '--sequenceType', dest="sequence_type", type=sequence_type, help="Test sequence type to run ('regular' or 'tia')", required=True)
    parser.add_argument('-t', '--suites', dest="suits", type=sequence_type, help="Test sequence type to run ('regular' or 'tia')", required=True)
    parser.add_argument('-s', '--safeMode', dest='safe_mode', action='store_true', help="If set, will run any test impact analysis runs in safe mode (unselected tests will still be run)")
    parser.set_defaults(dst_commit="HEAD^")
    parser.set_defaults(safe_mode=False)
    args = parser.parse_args()
    
    return args

if __name__ == "__main__":
    args = parse_args()
    tiaf = TestImpact(args.config, args.dst_commit)
    tiaf.run(args.sequence_type, args.safe_mode)
    sys.exit(0)