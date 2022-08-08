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
BINARY_PATH_KEY = 'tiaf_bin'
COMMON_CONFIG_KEY = "common"
WORKSPACE_KEY = "workspace"
ROOT_KEY = "root"
TEMP_KEY = "temp"
REPO_KEY = "repo"


@pytest.fixture
def test_data_file(build_directory):
    path = Path(build_directory+"/ly_test_impact_test_data.json")
    with open(path) as file:
        return json.load(file)


@pytest.fixture(scope='module', params=['profile', pytest.param('debug', marks=pytest.mark.skipif("True"))])
def build_type(request):
    """
    # debug build type disabled as we can't support testing this in AR as debug is not built
    """
    return request.param


@pytest.fixture
def config_path(build_type, test_data_file):
    return test_data_file[BUILD_INFO_KEY][build_type][CONFIG_PATH_KEY]


@pytest.fixture
def binary_path(config_data):
    return config_data[COMMON_CONFIG_KEY][REPO_KEY][BINARY_PATH_KEY]


@pytest.fixture()
def report_path(build_type, config_data, mock_uuid):
    return config_data[COMMON_CONFIG_KEY][WORKSPACE_KEY][TEMP_KEY][ROOT_KEY]+"\\report."+mock_uuid.hex+".json"


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
    args['suite'] = "main"
    args['test_failure_policy'] = "continue"
    return args


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
    runtime_args['suite'] = "--suite=main"
    return runtime_args


@pytest.fixture
def cpp_default_runtime_args(default_runtime_args):
    return default_runtime_args
