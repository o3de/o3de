#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from logging import getLogger
import os
from tiaf_driver import main
import json
from pathlib import Path
import pytest
import subprocess
import uuid
from botocore.stub import Stubber
from botocore import session
from boto3 import client


@pytest.fixture
def test_data_file():
    path = Path(
        "../o3de/scripts/build/TestImpactAnalysis/Testing/test_data.json")
    with open(path) as file:
        return json.load(file)


@pytest.fixture(scope='module', params=['profile', 'debug'])
def build_type(request):
    return request.param


@pytest.fixture
def config_path(build_type, test_data_file):
    return test_data_file['configs'][build_type]


@pytest.fixture
def binary_path(build_type, test_data_file):
    return test_data_file['binary'][build_type]


@pytest.fixture()
def report_path(build_type, test_data_file, mock_uuid):
    return test_data_file['report_dir'][build_type]+"\\report."+mock_uuid.hex+".json"


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
def default_runtime_args(mock_uuid, binary_path, report_path):
    runtime_args = {}
    runtime_args['sequence'] = "--sequence=seed"
    runtime_args['safemode'] = "--safemode=off"
    runtime_args['test_failure_policy'] = "--fpolicy=continue"
    runtime_args['report'] = "--report=" + \
        str(report_path).replace("/", "\\")
    runtime_args['suite'] = "--suite=main"
    return runtime_args
