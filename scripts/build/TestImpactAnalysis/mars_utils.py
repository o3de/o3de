#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import datetime
import json
import socket
from tiaf_logger import get_logger

logger = get_logger(__file__)

MARS_JOB_KEY = "job"
BUILD_NUMBER_KEY = "build_number"
SRC_COMMIT_KEY = "src_commit"
DST_COMMIT_KEY = "dst_commit"
COMMIT_DISTANCE_KEY = "commit_distance"
SRC_BRANCH_KEY = "src_branch"
DST_BRANCH_KEY = "dst_branch"
SUITE_KEY = "suite"
SOURCE_OF_TRUTH_BRANCH_KEY = "source_of_truth_branch"
IS_SOURCE_OF_TRUTH_BRANCH_KEY = "is_source_of_truth_branch"
USE_TEST_IMPACT_ANALYSIS_KEY = "use_test_impact_analysis"
HAS_CHANGE_LIST_KEY = "has_change_list"
HAS_HISTORIC_DATA_KEY = "has_historic_data"
S3_BUCKET_KEY = "s3_bucket"
DRIVER_ARGS_KEY = "driver_args"
RUNTIME_ARGS_KEY = "runtime_args"
RUNTIME_RETURN_CODE_KEY = "return_code"
NAME_KEY = "name"
RESULT_KEY = "result"
NUM_PASSING_TESTS_KEY = "num_passing_tests"
NUM_FAILING_TESTS_KEY = "num_failing_tests"
NUM_DISABLED_TESTS_KEY = "num_disabled_tests"
COMMAND_ARGS_STRING = "command_args"
NUM_PASSING_TEST_RUNS_KEY = "num_passing_test_runs"
NUM_FAILING_TEST_RUNS_KEY = "num_failing_test_runs"
NUM_EXECUTION_FAILURE_TEST_RUNS_KEY = "num_execution_failure_test_runs"
NUM_TIMED_OUT_TEST_RUNS_KEY = "num_timed_out_test_runs"
NUM_UNEXECUTED_TEST_RUNS_KEY = "num_unexecuted_test_runs"
TOTAL_NUM_PASSING_TESTS_KEY = "total_num_passing_tests"
TOTAL_NUM_FAILING_TESTS_KEY = "total_num_failing_tests"
TOTAL_NUM_DISABLED_TESTS_KEY = "total_num_disabled_tests"
START_TIME_KEY = "start_time"
END_TIME_KEY = "end_time"
DURATION_KEY = "duration"
INCLUDED_TEST_RUNS_KEY = "included_test_runs"
EXCLUDED_TEST_RUNS_KEY = "excluded_test_runs"
NUM_INCLUDED_TEST_RUNS_KEY = "num_included_test_runs"
NUM_EXCLUDED_TEST_RUNS_KEY = "num_excluded_test_runs"
TOTAL_NUM_TEST_RUNS_KEY = "total_num_test_runs"
PASSING_TEST_RUNS_KEY = "passing_test_runs"
FAILING_TEST_RUNS_KEY = "failing_test_runs"
EXECUTION_FAILURE_TEST_RUNS_KEY = "execution_failure_test_runs"
TIMED_OUT_TEST_RUNS_KEY = "timed_out_test_runs"
UNEXECUTED_TEST_RUNS_KEY = "unexecuted_test_runs"
TOTAL_NUM_PASSING_TEST_RUNS_KEY = "total_num_passing_test_runs"
TOTAL_NUM_FAILING_TEST_RUNS_KEY = "total_num_failing_test_runs"
TOTAL_NUM_EXECUTION_FAILURE_TEST_RUNS_KEY = "total_num_execution_failure_test_runs"
TOTAL_NUM_TIMED_OUT_TEST_RUNS_KEY = "total_num_timed_out_test_runs"
TOTAL_NUM_UNEXECUTED_TEST_RUNS_KEY = "total_num_unexecuted_test_runs"
SEQUENCE_TYPE_KEY = "type"
IMPACT_ANALYSIS_SEQUENCE_TYPE_KEY = "impact_analysis"
SAFE_IMPACT_ANALYSIS_SEQUENCE_TYPE_KEY = "safe_impact_analysis"
SEED_SEQUENCE_TYPE_KEY = "seed"
TEST_TARGET_TIMEOUT_KEY = "test_target_timeout"
GLOBAL_TIMEOUT_KEY = "global_timeout"
MAX_CONCURRENCY_KEY = "max_concurrency"
SELECTED_KEY = "selected"
DRAFTED_KEY = "drafted"
DISCARDED_KEY = "discarded"
SELECTED_TEST_RUN_REPORT_KEY = "selected_test_run_report"
DISCARDED_TEST_RUN_REPORT_KEY = "discarded_test_run_report"
DRAFTED_TEST_RUN_REPORT_KEY = "drafted_test_run_report"
SELECTED_TEST_RUNS_KEY = "selected_test_runs"
DRAFTED_TEST_RUNS_KEY = "drafted_test_runs"
DISCARDED_TEST_RUNS_KEY = "discarded_test_runs"
INSTRUMENTATION_KEY = "instrumentation"
EFFICIENCY_KEY = "efficiency"
CONFIG_KEY = "config"
POLICY_KEY = "policy"
CHANGE_LIST_KEY = "change_list"
TEST_RUN_SELECTION_KEY = "test_run_selection"
DYNAMIC_DEPENDENCY_MAP_POLICY_KEY = "dynamic_dependency_map"
DYNAMIC_DEPENDENCY_MAP_POLICY_UPDATE_KEY = "update"
REPORT_KEY = "report"

class FilebeatExn(Exception):
    pass

class FilebeatClient(object):
    def __init__(self, host="127.0.0.1", port=9000, timeout=20):
        self._filebeat_host = host
        self._filebeat_port = port
        self._socket_timeout = timeout
        self._socket = None

        self._open_socket()

    def send_event(self, payload, index, timestamp=None, pipeline="filebeat"):
        if not timestamp:
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
        logger.info(f"Connecting to Filebeat on {self._filebeat_host}:{self._filebeat_port}")

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
                logging.error("Filebeat socket closed by peer")
                self._socket.close()
                self._open_socket()
                total_sent = 0
            else:
                total_sent = total_sent + sent

def format_timestamp(timestamp: float):
    """
    Formats the given floating point timestamp into "yyyy-MM-dd'T'HH:mm:ss.SSSXX" format.

    @param timestamp: The timestamp to format.
    @return:          The formatted timestamp.
    """
    return datetime.datetime.utcfromtimestamp(timestamp).strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z"

def generate_mars_timestamp(t0_offset_milliseconds: int, t0_timestamp: float):
    """
    Generates a MARS timestamp in the format "yyyy-MM-dd'T'HH:mm:ss.SSSXX" by offsetting the T0 timestamp
    by the specified amount of milliseconds.

    @param t0_offset_milliseconds: The amount of time to offset from T0.
    @param t0_timestamp:           The T0 timestamp that TIAF timings will be offst from.
    @return:                       The formatted timestamp offset from T0 by the specified amount of milliseconds.
    """

    t0_offset_seconds = get_duration_in_seconds(t0_offset_milliseconds)
    t0_offset_timestamp = t0_timestamp + t0_offset_seconds
    return format_timestamp(t0_offset_timestamp)

def get_duration_in_seconds(duration_in_milliseconds: int):
    """
    Gets the specified duration in milliseconds (as used by TIAF) in seconds (as used my MARS documents).

    @param duration_in_milliseconds: The millisecond duration to transform into seconds.
    @return:                         The duration in seconds.
    """

    return duration_in_milliseconds * 0.001

def generate_mars_job(tiaf_result, driver_args, build_number: int):
    """
    Generates a MARS job document using the job meta-data used to drive the TIAF sequence.

    @param tiaf_result:  The result object generated by the TIAF script.
    @param driver_args:  The arguments specified to the driver script.
    @param driver_args:  The arguments specified to the driver script.
    @param build_number: The build number this job corresponds to.
    @return:            The MARS job document with the job meta-data.
    """

    mars_job = {key:tiaf_result[key] for key in 
    [
        SRC_COMMIT_KEY,
        DST_COMMIT_KEY,
        COMMIT_DISTANCE_KEY,
        SRC_BRANCH_KEY,
        DST_BRANCH_KEY,
        SUITE_KEY,
        SOURCE_OF_TRUTH_BRANCH_KEY,
        IS_SOURCE_OF_TRUTH_BRANCH_KEY,
        USE_TEST_IMPACT_ANALYSIS_KEY,
        HAS_CHANGE_LIST_KEY,
        HAS_HISTORIC_DATA_KEY,
        S3_BUCKET_KEY,
        RUNTIME_ARGS_KEY,
        RUNTIME_RETURN_CODE_KEY
    ]}

    mars_job[DRIVER_ARGS_KEY] = driver_args
    mars_job[BUILD_NUMBER_KEY] = build_number
    return mars_job

def generate_test_run_list(test_runs):
    """
    Generates a list of test run name strings from the list of TIAF test runs.

    @param test_runs: The list of TIAF test runs to generate the name strings from.
    @return:          The list of test run name strings.
    """

    test_run_list = []
    for test_run in test_runs:
        test_run_list.append(test_run[NAME_KEY])
    return test_run_list

def generate_mars_test_run_selections(test_run_selection, test_run_report, t0_timestamp: float):
    """
    Generates a list of MARS test run selections from a TIAF test run selection and report.

    @param test_run_selection: The TIAF test run selection.
    @param test_run_report:    The TIAF test run report.
    @param t0_timestamp:       The T0 timestamp that TIAF timings will be offst from.
    @return:                   The list of TIAF test runs.
    """

    mars_test_run_selection = {key:test_run_report[key] for key in 
    [        
        RESULT_KEY,
        NUM_PASSING_TEST_RUNS_KEY,
        NUM_FAILING_TEST_RUNS_KEY,
        NUM_EXECUTION_FAILURE_TEST_RUNS_KEY,
        NUM_TIMED_OUT_TEST_RUNS_KEY,
        NUM_UNEXECUTED_TEST_RUNS_KEY,
        TOTAL_NUM_PASSING_TESTS_KEY,
        TOTAL_NUM_FAILING_TESTS_KEY,
        TOTAL_NUM_DISABLED_TESTS_KEY
    ]}
    
    mars_test_run_selection[START_TIME_KEY] = generate_mars_timestamp(test_run_report[START_TIME_KEY], t0_timestamp)
    mars_test_run_selection[END_TIME_KEY] = generate_mars_timestamp(test_run_report[END_TIME_KEY], t0_timestamp)
    mars_test_run_selection[DURATION_KEY] = get_duration_in_seconds(test_run_report[DURATION_KEY])

    mars_test_run_selection[INCLUDED_TEST_RUNS_KEY] = test_run_selection[INCLUDED_TEST_RUNS_KEY]
    mars_test_run_selection[EXCLUDED_TEST_RUNS_KEY] = test_run_selection[EXCLUDED_TEST_RUNS_KEY]
    mars_test_run_selection[NUM_INCLUDED_TEST_RUNS_KEY] = test_run_selection[NUM_INCLUDED_TEST_RUNS_KEY]
    mars_test_run_selection[NUM_EXCLUDED_TEST_RUNS_KEY] = test_run_selection[NUM_EXCLUDED_TEST_RUNS_KEY]
    mars_test_run_selection[TOTAL_NUM_TEST_RUNS_KEY] = test_run_selection[TOTAL_NUM_TEST_RUNS_KEY]

    mars_test_run_selection[PASSING_TEST_RUNS_KEY] = generate_test_run_list(test_run_report[PASSING_TEST_RUNS_KEY])
    mars_test_run_selection[FAILING_TEST_RUNS_KEY] = generate_test_run_list(test_run_report[FAILING_TEST_RUNS_KEY])
    mars_test_run_selection[EXECUTION_FAILURE_TEST_RUNS_KEY] = generate_test_run_list(test_run_report[EXECUTION_FAILURE_TEST_RUNS_KEY])
    mars_test_run_selection[TIMED_OUT_TEST_RUNS_KEY] = generate_test_run_list(test_run_report[TIMED_OUT_TEST_RUNS_KEY])
    mars_test_run_selection[UNEXECUTED_TEST_RUNS_KEY] = generate_test_run_list(test_run_report[UNEXECUTED_TEST_RUNS_KEY])

    return mars_test_run_selection

def generate_test_runs_from_list(test_run_list: list):
    """
    Generates a list of TIAF test runs from a list of test target name strings.

    @param test_run_list: The list of test target names.
    @return:              The list of TIAF test runs.
    """

    test_run_list = {
    TOTAL_NUM_TEST_RUNS_KEY: len(test_run_list),
    NUM_INCLUDED_TEST_RUNS_KEY: len(test_run_list),
    NUM_EXCLUDED_TEST_RUNS_KEY: 0,
    INCLUDED_TEST_RUNS_KEY: test_run_list,
    EXCLUDED_TEST_RUNS_KEY: []
    }

    return test_run_list

def generate_mars_sequence(sequence_report: dict, mars_job: dict, change_list:dict, t0_timestamp: float):
    """
    Generates the MARS sequence document from the specified TIAF sequence report.

    @param sequence_report: The TIAF runtime sequence report.
    @param mars_job:        The MARS job for this sequence.
    @param change_list:     The change list for which the TIAF sequence was run.
    @param t0_timestamp:    The T0 timestamp that TIAF timings will be offst from.
    @return:                The MARS sequence document for the specified TIAF sequence report.
    """

    mars_sequence = {key:sequence_report[key] for key in 
    [
        SEQUENCE_TYPE_KEY, 
        RESULT_KEY, 
        POLICY_KEY,
        TOTAL_NUM_TEST_RUNS_KEY, 
        TOTAL_NUM_PASSING_TEST_RUNS_KEY,
        TOTAL_NUM_FAILING_TEST_RUNS_KEY,
        TOTAL_NUM_EXECUTION_FAILURE_TEST_RUNS_KEY,
        TOTAL_NUM_TIMED_OUT_TEST_RUNS_KEY,
        TOTAL_NUM_UNEXECUTED_TEST_RUNS_KEY,
        TOTAL_NUM_PASSING_TESTS_KEY,
        TOTAL_NUM_FAILING_TESTS_KEY,
        TOTAL_NUM_DISABLED_TESTS_KEY
    ]}

    mars_sequence[START_TIME_KEY] = generate_mars_timestamp(sequence_report[START_TIME_KEY], t0_timestamp)
    mars_sequence[END_TIME_KEY] = generate_mars_timestamp(sequence_report[END_TIME_KEY], t0_timestamp)
    mars_sequence[DURATION_KEY] = get_duration_in_seconds(sequence_report[DURATION_KEY])

    config = {key:sequence_report[key] for key in 
    [
        TEST_TARGET_TIMEOUT_KEY,
        GLOBAL_TIMEOUT_KEY,
        MAX_CONCURRENCY_KEY
    ]}

    test_run_selection = {}
    test_run_selection[SELECTED_KEY] = generate_mars_test_run_selections(sequence_report[SELECTED_TEST_RUNS_KEY], sequence_report[SELECTED_TEST_RUN_REPORT_KEY], t0_timestamp)
    if sequence_report[SEQUENCE_TYPE_KEY] == IMPACT_ANALYSIS_SEQUENCE_TYPE_KEY or sequence_report[SEQUENCE_TYPE_KEY] == SAFE_IMPACT_ANALYSIS_SEQUENCE_TYPE_KEY:
        total_test_runs = sequence_report[TOTAL_NUM_TEST_RUNS_KEY] + len(sequence_report[DISCARDED_TEST_RUNS_KEY])
        if total_test_runs > 0:
            test_run_selection[SELECTED_KEY][EFFICIENCY_KEY] = (1.0 - (test_run_selection[SELECTED_KEY][TOTAL_NUM_TEST_RUNS_KEY] / total_test_runs)) * 100
        else:
            test_run_selection[SELECTED_KEY][EFFICIENCY_KEY] = 100
        test_run_selection[DRAFTED_KEY] = generate_mars_test_run_selections(generate_test_runs_from_list(sequence_report[DRAFTED_TEST_RUNS_KEY]), sequence_report[DRAFTED_TEST_RUN_REPORT_KEY], t0_timestamp)
        if sequence_report[SEQUENCE_TYPE_KEY] == SAFE_IMPACT_ANALYSIS_SEQUENCE_TYPE_KEY:
            test_run_selection[DISCARDED_KEY] = generate_mars_test_run_selections(sequence_report[DISCARDED_TEST_RUNS_KEY], sequence_report[DISCARDED_TEST_RUN_REPORT_KEY], t0_timestamp)
    else:
        test_run_selection[SELECTED_KEY][EFFICIENCY_KEY] = 0

    mars_sequence[MARS_JOB_KEY] = mars_job
    mars_sequence[CONFIG_KEY] = config
    mars_sequence[TEST_RUN_SELECTION_KEY] = test_run_selection
    mars_sequence[CHANGE_LIST_KEY] = change_list

    return mars_sequence

def extract_mars_test_target(test_run, instrumentation, mars_job, t0_timestamp: float):
    """
    Extracts a MARS test target from the specified TIAF test run.

    @param test_run:        The TIAF test run.
    @param instrumentation: Flag specifying whether or not instrumentation was used for the test targets in this run.
    @param mars_job:        The MARS job for this test target.
    @param t0_timestamp:    The T0 timestamp that TIAF timings will be offst from.
    @return:                The MARS test target documents for the specified TIAF test target.
    """

    mars_test_run = {key:test_run[key] for key in 
    [
        NAME_KEY,
        RESULT_KEY,
        NUM_PASSING_TESTS_KEY,
        NUM_FAILING_TESTS_KEY,
        NUM_DISABLED_TESTS_KEY,
        COMMAND_ARGS_STRING
    ]}

    mars_test_run[START_TIME_KEY] = generate_mars_timestamp(test_run[START_TIME_KEY], t0_timestamp)
    mars_test_run[END_TIME_KEY] = generate_mars_timestamp(test_run[END_TIME_KEY], t0_timestamp)
    mars_test_run[DURATION_KEY] = get_duration_in_seconds(test_run[DURATION_KEY])

    mars_test_run[MARS_JOB_KEY] = mars_job
    mars_test_run[INSTRUMENTATION_KEY] = instrumentation
    return mars_test_run

def extract_mars_test_targets_from_report(test_run_report, instrumentation, mars_job, t0_timestamp: float):
    """
    Extracts the MARS test targets from the specified TIAF test run report.

    @param test_run_report: The TIAF runtime test run report.
    @param instrumentation: Flag specifying whether or not instrumentation was used for the test targets in this run.
    @param mars_job:        The MARS job for these test targets.
    @param t0_timestamp:    The T0 timestamp that TIAF timings will be offst from.
    @return:                The list of all MARS test target documents for the test targets in the TIAF test run report.
    """

    mars_test_targets = []

    for test_run in test_run_report[PASSING_TEST_RUNS_KEY]:
        mars_test_targets.append(extract_mars_test_target(test_run, instrumentation, mars_job, t0_timestamp))
    for test_run in test_run_report[FAILING_TEST_RUNS_KEY]:
        mars_test_targets.append(extract_mars_test_target(test_run, instrumentation, mars_job, t0_timestamp))
    for test_run in test_run_report[EXECUTION_FAILURE_TEST_RUNS_KEY]:
        mars_test_targets.append(extract_mars_test_target(test_run, instrumentation, mars_job, t0_timestamp))
    for test_run in test_run_report[TIMED_OUT_TEST_RUNS_KEY]:
        mars_test_targets.append(extract_mars_test_target(test_run, instrumentation, mars_job, t0_timestamp))
    for test_run in test_run_report[UNEXECUTED_TEST_RUNS_KEY]:
        mars_test_targets.append(extract_mars_test_target(test_run, instrumentation, mars_job, t0_timestamp))

    return mars_test_targets

def generate_mars_test_targets(sequence_report: dict, mars_job: dict, t0_timestamp: float):
    """
    Generates a MARS test target document for each test target in the TIAF sequence report.

    @param sequence_report: The TIAF runtime sequence report.
    @param mars_job:        The MARS job for this sequence.
    @param t0_timestamp:    The T0 timestamp that TIAF timings will be offst from.
    @return:                The list of all MARS test target documents for the test targets in the TIAF sequence report.
    """

    mars_test_targets = []

    # Determine whether or not the test targets were executed with instrumentation
    if sequence_report[SEQUENCE_TYPE_KEY] == SEED_SEQUENCE_TYPE_KEY or sequence_report[SEQUENCE_TYPE_KEY] == SAFE_IMPACT_ANALYSIS_SEQUENCE_TYPE_KEY or (sequence_report[SEQUENCE_TYPE_KEY] == IMPACT_ANALYSIS_SEQUENCE_TYPE_KEY and sequence_report[POLICY_KEY][DYNAMIC_DEPENDENCY_MAP_POLICY_KEY] == DYNAMIC_DEPENDENCY_MAP_POLICY_UPDATE_KEY):
        instrumentation = True
    else:
        instrumentation = False
    
    # Extract the MARS test target documents from each of the test run reports
    mars_test_targets += extract_mars_test_targets_from_report(sequence_report[SELECTED_TEST_RUN_REPORT_KEY], instrumentation, mars_job, t0_timestamp)
    if sequence_report[SEQUENCE_TYPE_KEY] == IMPACT_ANALYSIS_SEQUENCE_TYPE_KEY or sequence_report[SEQUENCE_TYPE_KEY] == SAFE_IMPACT_ANALYSIS_SEQUENCE_TYPE_KEY:
        mars_test_targets += extract_mars_test_targets_from_report(sequence_report[DRAFTED_TEST_RUN_REPORT_KEY], instrumentation, mars_job, t0_timestamp)
        if sequence_report[SEQUENCE_TYPE_KEY] == SAFE_IMPACT_ANALYSIS_SEQUENCE_TYPE_KEY:
            mars_test_targets += extract_mars_test_targets_from_report(sequence_report[DISCARDED_TEST_RUN_REPORT_KEY], instrumentation, mars_job, t0_timestamp)

    return mars_test_targets

def transmit_report_to_mars(mars_index_prefix: str, tiaf_result: dict, driver_args: list, build_number: int):
    """
    Transforms the TIAF result into the appropriate MARS documents and transmits them to MARS.

    @param mars_index_prefix: The index prefix to be used for all MARS documents.
    @param tiaf_result:       The result object from the TIAF script.
    @param driver_args:       The arguments passed to the TIAF driver script.
    """
    
    try:
        filebeat = FilebeatClient("localhost", 9000, 60)

        # T0 is the current timestamp that the report timings will be offset from
        t0_timestamp = datetime.datetime.now().timestamp()

        # Generate and transmit the MARS job document
        mars_job = generate_mars_job(tiaf_result, driver_args, build_number)
        filebeat.send_event(mars_job, f"{mars_index_prefix}.tiaf.job")

        if tiaf_result[REPORT_KEY]:
            # Generate and transmit the MARS sequence document
            mars_sequence = generate_mars_sequence(tiaf_result[REPORT_KEY], mars_job, tiaf_result[CHANGE_LIST_KEY], t0_timestamp)
            filebeat.send_event(mars_sequence, f"{mars_index_prefix}.tiaf.sequence")
            
            # Generate and transmit the MARS test target documents
            mars_test_targets = generate_mars_test_targets(tiaf_result[REPORT_KEY], mars_job, t0_timestamp)
            for mars_test_target in mars_test_targets:
                filebeat.send_event(mars_test_target, f"{mars_index_prefix}.tiaf.test_target")
    except FilebeatExn as e:
        logger.error(e)
    except KeyError as e:
        logger.error(f"The report does not contain the key {str(e)}.")