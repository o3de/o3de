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
import argparse
import os

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

        #print(f"-> {data}")
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

def format_timestamp(timestamp):
    return datetime.datetime.utcfromtimestamp(timestamp).strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z"

def generate_mars_timestamp(t0_offset_milliseconds, t0_timestamp):
    t0_offset_seconds = t0_offset_milliseconds * 0.001
    t0_offset_timestamp = t0_timestamp + t0_offset_seconds
    return format_timestamp(t0_offset_timestamp)

def generate_mars_job(tiaf_result, driver_args):
    mars_job = {key:tiaf_result[key] for key in 
    [
        "src_commit",
        "dst_commit",
        "commit_distance",
        "src_branch",
        "dst_branch",
        "suite",
        "coverage_update_branches",
        "is_coverage_update_branch",
        "use_test_impact_analysis",
        "has_change_list",
        "has_historic_data",
        "s3_bucket",
        "has_change_list",
        "runtime_args",
        "return_code"
    ]}

    mars_job["driver_args"] = driver_args
    return mars_job

def generate_test_run_list(test_runs):
    test_run_list = []
    for test_run in test_runs:
        test_run_list.append(test_run["name"])
    return test_run_list

def generate_mars_test_runs(test_run_selection, test_run_report, t0_timestamp):
    mars_test_runs = {key:test_run_report[key] for key in 
    [        
        "result",
        "num_passing_test_runs",
        "num_failing_test_runs",
        "num_execution_failure_test_runs",
        "num_timed_out_test_runs",
        "num_unexecuted_test_runs",
        "total_num_passing_tests",
        "total_num_failing_tests",
        "total_num_disabled_tests"
    ]}
    
    mars_test_runs["start_time"] = generate_mars_timestamp(test_run_report["start_time"], t0_timestamp)
    mars_test_runs["end_time"] = generate_mars_timestamp(test_run_report["end_time"], t0_timestamp)
    mars_test_runs["duration"] = test_run_report["duration"] * 0.001

    mars_test_runs["included_test_runs"] = test_run_selection["included_test_runs"]
    mars_test_runs["excluded_test_runs"] = test_run_selection["excluded_test_runs"]
    mars_test_runs["num_included_test_runs"] = test_run_selection["num_included_test_runs"]
    mars_test_runs["num_excluded_test_runs"] = test_run_selection["num_excluded_test_runs"]
    mars_test_runs["total_num_test_runs"] = test_run_selection["total_num_test_runs"]

    mars_test_runs["passing_test_runs"] = generate_test_run_list(test_run_report["passing_test_runs"])
    mars_test_runs["failing_test_runs"] = generate_test_run_list(test_run_report["failing_test_runs"])
    mars_test_runs["execution_failure_test_runs"] = generate_test_run_list(test_run_report["execution_failure_test_runs"])
    mars_test_runs["timed_out_test_runs"] = generate_test_run_list(test_run_report["timed_out_test_runs"])
    mars_test_runs["unexecuted_test_runs"] = generate_test_run_list(test_run_report["unexecuted_test_runs"])

    return mars_test_runs

def generate_test_runs_from_list(test_run_list):
    test_run_list = {
    "total_num_test_runs": len(test_run_list),
    "num_included_test_runs": len(test_run_list),
    "num_excluded_test_runs": 0,
    "included_test_runs": test_run_list,
    "excluded_test_runs": []
    }
    return test_run_list

def generate_mars_sequence(sequence_report, mars_job, change_list, t0_timestamp):
    #
    mars_sequence = {key:sequence_report[key] for key in 
    [
        "type", 
        "result", 
        "total_num_test_runs", 
        "total_num_passing_test_runs",
        "total_num_failing_test_runs",
        "total_num_execution_failure_test_runs",
        "total_num_timed_out_test_runs",
        "total_num_unexecuted_test_runs",
        "total_num_passing_tests",
        "total_num_failing_tests",
        "total_num_disabled_tests"
    ]}

    #
    mars_sequence["start_time"] = generate_mars_timestamp(sequence_report["start_time"], t0_timestamp)
    mars_sequence["end_time"] = generate_mars_timestamp(sequence_report["end_time"], t0_timestamp)
    mars_sequence["duration"] = sequence_report["duration"] * 0.001

    #
    config = {key:sequence_report[key] for key in 
    [
        "test_target_timeout",
        "global_timeout",
        "max_concurrency"
    ]}

    #
    test_run_selection = {}
    test_run_selection["selected"] = generate_mars_test_runs(sequence_report["selected_test_runs"], sequence_report["selected_test_run_report"], t0_timestamp)
    if sequence_report["type"] == "impact_analysis" or sequence_report["type"] == "safe_impact_analysis":
        total_test_runs = sequence_report["total_num_test_runs"]
        if total_test_runs > 0:
            test_run_selection["selected"]["efficiency"] = (1.0 - (test_run_selection["selected"]["total_num_test_runs"] / total_test_runs)) * 100
        else:
            test_run_selection["selected"]["efficiency"] = 100
        test_run_selection["drafted"] = generate_mars_test_runs(generate_test_runs_from_list(sequence_report["drafted_test_runs"]), sequence_report["drafted_test_run_report"], t0_timestamp)
        if sequence_report["type"] == "safe_impact_analysis":
            test_run_selection["discarded"] = generate_mars_test_runs(sequence_report["discarded_test_runs"], sequence_report["discarded_test_run_report"], t0_timestamp)
    else:
        test_run_selection["selected"]["efficiency"] = 0

    #
    mars_sequence["job"] = mars_job
    mars_sequence["config"] = config
    mars_sequence["policy"] = sequence_report["policy"]
    mars_sequence["test_run_selection"] = test_run_selection
    mars_sequence["change_list"] = change_list

    return mars_sequence

def extract_mars_test_run(test_run, instrumentation, mars_job, t0_timestamp):
    mars_test_run = {key:test_run[key] for key in 
    [
        "name",
        "result",
        "num_passing_tests",
        "num_failing_tests",
        "num_disabled_tests"
    ]}

    #
    mars_test_run["start_time"] = generate_mars_timestamp(test_run["start_time"], t0_timestamp)
    mars_test_run["end_time"] = generate_mars_timestamp(test_run["end_time"], t0_timestamp)
    mars_test_run["duration"] = test_run["duration"] * 0.001

    mars_test_run["job"] = mars_job
    mars_test_run["command_args"] = test_run["command_string"].split()
    mars_test_run["instrumentation"] = instrumentation
    return mars_test_run

def extract_mars_test_runs_from_report(test_run_report, instrumentation, mars_job, t0_timestamp):
    mars_test_runs = []

    for test_run in test_run_report["passing_test_runs"]:
        mars_test_runs.append(extract_mars_test_run(test_run, instrumentation, mars_job, t0_timestamp))
    for test_run in test_run_report["failing_test_runs"]:
        mars_test_runs.append(extract_mars_test_run(test_run, instrumentation, mars_job, t0_timestamp))
    for test_run in test_run_report["execution_failure_test_runs"]:
        mars_test_runs.append(extract_mars_test_run(test_run, instrumentation, mars_job, t0_timestamp))
    for test_run in test_run_report["timed_out_test_runs"]:
        mars_test_runs.append(extract_mars_test_run(test_run, instrumentation, mars_job, t0_timestamp))
    for test_run in test_run_report["unexecuted_test_runs"]:
        mars_test_runs.append(extract_mars_test_run(test_run, instrumentation, mars_job, t0_timestamp))

    return mars_test_runs

def generate_mars_test_targets(sequence_report, mars_job, t0_timestamp):
    mars_test_targets = []
    if sequence_report["type"] == "seed" or sequence_report["type"] == "safe_impact_analysis" or (sequence_report["type"] == "impact_analysis" and sequence_report["policy"]["dynamic_dependency_map"] == "update"):
        instrumentation = True
    else:
        instrumentation = False
    
    mars_test_targets += extract_mars_test_runs_from_report(sequence_report["selected_test_run_report"], instrumentation, mars_job, t0_timestamp)
    if sequence_report["type"] == "impact_analysis" or sequence_report["type"] == "safe_impact_analysis":
        mars_test_targets += extract_mars_test_runs_from_report(sequence_report["drafted_test_run_report"], instrumentation, mars_job, t0_timestamp)
        if sequence_report["type"] == "safe_impact_analysis":
            mars_test_targets += extract_mars_test_runs_from_report(sequence_report["discarded_test_run_report"], instrumentation, mars_job, t0_timestamp)

    return mars_test_targets

def transmit_report_to_mars(tiaf_result, driver_args):
    filebeat = FilebeatClient("localhost", 9000, 60)
    t0_timestamp = datetime.datetime.now().timestamp()
    #
    mars_job = generate_mars_job(tiaf_result, driver_args)
    filebeat.send_event(mars_job, "jonawals.tiaf.job")

    if tiaf_result["report"] is not None:
        #
        mars_sequence = generate_mars_sequence(tiaf_result["report"], mars_job, tiaf_result["change_list"], t0_timestamp)
        filebeat.send_event(mars_sequence, "jonawals.tiaf.sequence")
        #
        mars_test_targets = generate_mars_test_targets(tiaf_result["report"], mars_job, t0_timestamp)
        for mars_test_target in mars_test_targets:
            filebeat.send_event(mars_test_target, "jonawals.tiaf.test_target")