#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import json
from pathlib import Path
import pytest
import uuid

BUILD_INFO_KEY = 'build_info'
CONFIG_PATH_KEY = 'config'
BINARY_PATH_KEY = 'runtime_bin'
COMMON_CONFIG_KEY = "common"
JENKINS_KEY = "jenkins"
RUNTIME_BIN_KEY = "runtime_bin"
ENABLED_KEY = "enabled"
WORKSPACE_KEY = "workspace"
ROOT_KEY = "root"
TEMP_KEY = "temp"
REPO_KEY = "repo"
RELATIVE_PATHS_KEY = "relative_paths"
HISTORIC_KEY = 'historic'
HISTORIC_SEQUENCES_KEY = "historic_sequences"
ACTIVE_KEY = "active"
HISTORIC_KEY = "historic"
TEST_IMPACT_DATA_FILE_KEY = "test_impact_data_file"
PREVIOUS_TEST_RUN_DATA_FILE_KEY = "previous_test_run_data_file"
LAST_COMMIT_HASH_KEY = "last_commit_hash"
COVERAGE_DATA_KEY = "coverage_data"
PREVIOUS_TEST_RUNS_KEY = "previous_test_runs"
HISTORIC_DATA_FILE_KEY = "data"
REPORT_KEY = "reports"

@pytest.fixture
def test_data_file(build_directory):
    path = Path(build_directory+"/ly_test_impact_test_data.json")
    with open(path) as file:
        return json.load(file)


@pytest.fixture(params=[pytest.param('profile', marks=pytest.mark.skipif("False")), pytest.param('debug', marks=pytest.mark.skipif("True"))])
def build_type(request):
    """
    # debug build type disabled as we can't support testing this in AR as debug is not built
    """
    return request.param


@pytest.fixture
def storage_config(runtime_type, config_data):
    args_from_config = {}
    args_from_config['active_workspace'] = config_data[runtime_type][WORKSPACE_KEY][ACTIVE_KEY][ROOT_KEY]
    args_from_config['historic_workspace'] = config_data[runtime_type][WORKSPACE_KEY][HISTORIC_KEY][ROOT_KEY]
    args_from_config['unpacked_coverage_data_file'] = config_data[runtime_type][
        WORKSPACE_KEY][ACTIVE_KEY][RELATIVE_PATHS_KEY][TEST_IMPACT_DATA_FILE_KEY]
    args_from_config['previous_test_run_data_file'] = config_data[runtime_type][WORKSPACE_KEY][
        ACTIVE_KEY][RELATIVE_PATHS_KEY][PREVIOUS_TEST_RUN_DATA_FILE_KEY]
    args_from_config['historic_data_file'] = config_data[runtime_type][WORKSPACE_KEY][
        HISTORIC_KEY][RELATIVE_PATHS_KEY][HISTORIC_DATA_FILE_KEY]
    args_from_config['temp_workspace'] = config_data[runtime_type][WORKSPACE_KEY][TEMP_KEY][ROOT_KEY]
    return args_from_config

@pytest.fixture
def skip_if_test_targets_disabled(runtime_type, config_data):
    tiaf_bin = Path(config_data[runtime_type][RUNTIME_BIN_KEY])
    # We need the runtime to be enabled and the runtime binary to exist to run tests
    if not config_data[runtime_type][JENKINS_KEY][ENABLED_KEY] or not tiaf_bin.is_file():
        pytest.skip("Test targets are disabled for this runtime, test will be skipped.")


@pytest.fixture
def config_path(build_type, test_data_file):
    return test_data_file[BUILD_INFO_KEY][build_type][CONFIG_PATH_KEY]


@pytest.fixture
def binary_path(config_data, runtime_type):
    return config_data[runtime_type][BINARY_PATH_KEY]


@pytest.fixture()
def report_path(runtime_type, config_data, mock_uuid):
    return config_data[runtime_type][WORKSPACE_KEY][TEMP_KEY][REPORT_KEY]+"\\report."+mock_uuid.hex+".json"


@pytest.fixture
def config_data(config_path):
    with open(config_path) as f:
        return json.load(f)


@pytest.fixture
def tiaf_args(config_path):
    args = {}
    args['config'] = config_path
    args['src_branch'] = "123"
    args['dst_branch'] = "123"
    args['commit'] = "foobar"
    args['build_number'] = 1
    args['suites'] = "main"
    args['test_failure_policy'] = "continue"
    return args


@pytest.fixture
def main_args(tiaf_args, runtime_type, request):
    tiaf_args['runtime_type'] = runtime_type
    return tiaf_args


@pytest.fixture(params=["native", "python"])
def runtime_type(request):
    return request.param


@pytest.fixture
def mock_runtime(mocker):
    return mocker.patch('subprocess.run')


@pytest.fixture(autouse=True)
def mock_uuid(mocker):
    universal_uuid = uuid.uuid4()
    mocker.patch('uuid.uuid4', return_value=universal_uuid)
    return universal_uuid


@pytest.fixture
def default_runtime_args(mock_uuid, report_path):
    runtime_args = {}
    runtime_args['sequence'] = "--sequence=seed"
    runtime_args['test_failure_policy'] = "--fpolicy=continue"
    runtime_args['report'] = "--report=" + \
        str(report_path).replace("/", "\\")
    runtime_args['suites'] = "--suites=main"
    return runtime_args


@pytest.fixture
def cpp_default_runtime_args(default_runtime_args):
    return default_runtime_args
