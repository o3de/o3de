#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import sys
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

def generate_mars_job(tiaf_result, snapshot, driver_args):
    mars_job = tiaf_result
    mars_job["snapshot"] = snapshot
    mars_job["driver_args"] = driver_args
    mars_job.pop("report", None)
    return mars_job

def generate_mars_sequence(tiaf_result, snapshot, driver_args, test_impact_data, change_list):
    #
    sequence = {}
    sequence["type"] = tiaf_result["report"]["type"]
    sequence["start_time"] = tiaf_result["report"]["start_time"]
    sequence["end_time"] = tiaf_result["report"]["end_time"]
    sequence["duration"] = tiaf_result["report"]["duration"]
    sequence["result"] = tiaf_result["report"]["result"]
    sequence["test_impact_data"] = tiaf_result["report"]["test_impact_data"]
    sequence["change_list"] = change_list

    #
    config = {}
    config["test_target_timeout"] = tiaf_result["report"]["test_target_timeout"]
    config["global_timeout"] = tiaf_result["report"]["global_timeout"]
    config["max_concurrency"] = tiaf_result["report"]["max_concurrency"]
    
    #
    policy = {}
    policy["execution_failure"] = tiaf_result["report"]["policy"]["execution_failure"]
    policy["coverage_failure"] = tiaf_result["report"]["policy"]["coverage_failure"]
    policy["test_failure"] = tiaf_result["report"]["policy"]["test_failure"]
    policy["integrity_failure"] = tiaf_result["report"]["policy"]["integrity_failure"]
    policy["test_sharding"] = tiaf_result["report"]["policy"]["test_sharding"]
    policy["target_output_capture"] = tiaf_result["report"]["policy"]["target_output_capture"]

    if sequence["type"] == "impact_analysis" or sequence["type"] == "safe_impact_analysis":
        policy["test_prioritization"] = tiaf_result["report"]["policy"]["test_prioritization"]
        if sequence["type"] == "impact_analysis":
            policy["dynamic_dependency_map"] = tiaf_result["report"]["policy"]["dynamic_dependency_map"]

    

    mars_sequence = {}
    mars_sequence["job"] = generate_mars_job(tiaf_result, snapshot, driver_args)


    
def export_mars_test_target(return_code, report):
    mars_test_target = {}