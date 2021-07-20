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

class FilebeatExn(Exception):
    pass

class FilebeatClient(object):
    def __init__(self,  host="127.0.0.1", port=9000, timeout=20):
        self._filebeat_host = host
        self._filebeat_port = port
        self._socket_timeout = timeout
        self._socket = None

        self._open_socket()

    def send_event(self, payload, index, timestamp=None, pipeline="filebeat"):
        if timestamp is None:
            timestamp = datetime.datetime.utcnow().timestamp()

        event = {
            "index": index,
            "timestamp": timestamp,
            "pipeline": pipeline,
            "payload": json.dumps(payload)
        }

        # Serialise event, add new line and encode as UTF-8 before sending to Filebeat.
        data = json.dumps(event, sort_keys=True) + "\n"
        data = data.encode()

        print(f"-> {data}")
        self._send_data(data)

    def _open_socket(self):
        print(f"Connecting to Filebeat on {self._filebeat_host}:{self._filebeat_port}")

        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._socket.settimeout(self._socket_timeout)

        try:
            self._socket.connect((self._filebeat_host, self._filebeat_port))
        except (ConnectionError, socket.timeout):
            raise FilebeatExn("Failed to connect to Filebeat") from None

    def _send_data(self, data):
        total_sent = 0

        while total_sent < len(data):
            try:
                sent = self._socket.send(data[total_sent:])
            except BrokenPipeError:
                print("Filebeat socket closed by peer")
                self._socket.close()
                self._open_socket()
                total_sent = 0
            else:
                total_sent = total_sent + sent


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

def generate_mars_job(tiaf, return_code, suite, snapshot, seeding_branches, seeding_pipelines, runtime_args):
    mars_job = {}
    mars_job["src_commit"] = tiaf.src_commit
    mars_job["dst_commit"] = tiaf.dst_commit
    mars_job["commit_distance"] = tiaf.commit_distance
    mars_job["branch"] = tiaf.src_branch
    mars_job["suite"] = suite
    mars_job["pipeline"] =  tiaf.pipeline
    mars_job["snapshot"] = snapshot
    mars_job["seeding_branches"] = seeding_branches
    mars_job["seeding_pipelines"] = seeding_pipelines
    mars_job["is_seeding_branch"] = tiaf.is_seeding_branch
    mars_job["is_seeding_pipeline"] = tiaf.is_seeding_pipeline
    mars_job["use_test_impact_analysis"] = tiaf.use_test_impact_analysis
    mars_job["has_change_list"] = tiaf.has_change_list
    mars_job["driver_args"] = str(sys.argv)
    mars_job["runtime_args"] = runtime_args
    mars_job["return_code"] = return_code
    return mars_job

def generate_mars_sequence(report):
    mars_sequence = {}
    mars_sequence["foo"]
    mars_sequence["foo"]
    mars_sequence["foo"]
    mars_sequence["foo"]
    mars_sequence["foo"]

def export_mars_test_target(return_code, report):
    mars_test_target = {}


if __name__ == "__main__":
    

    try:
        args = parse_args()
        tiaf = TestImpact(args.config, args.dst_commit, args.src_branch, args.pipeline, args.seeding_branches, args.seeding_pipelines)

        mars_job = generate_mars_job(tiaf, -1, args.suite, args.snapshot, args.seeding_branches, args.seeding_pipelines, "--foo=bar")
        filebeat = FilebeatClient("localhost", 9000, 60)
        filebeat.send_event(mars_job, "jonawals.tiaf.job") # DICTIONARY OF PAYLOAD CONTENTS ONLY

        return_code, report, args = tiaf.run(args.suite, args.test_failure_policy, args.safe_mode, args.test_timeout, args.global_timeout)
        # Non-gating will be removed from this script and handled at the job level in SPEC-7413
        #sys.exit(return_code)
        sys.exit(0)
    except Exception as e:
        # Non-gating will be removed from this script and handled at the job level in SPEC-7413
        print(f"Exception caught by TIAF driver: {e}")
