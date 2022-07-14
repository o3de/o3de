#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


from logging import getLogger
import os
from tiaf import TestImpact
from tiaf_driver import main
from pathlib import Path
import pytest
import subprocess
import uuid

logging = getLogger("tiaf")


class TestTIAFUnitTests():

    @pytest.fixture
    def build_root(self):
        if not os.environ.get('OUTPUT_DIRECTORY'):
            return "C:\\o3de\\build"
        return os.environ.get('OUTPUT_DIRECTORY')

    @pytest.fixture
    def tiaf_args(self, build_root):
        args = {}
        args['config'] = build_root+"/bin/TestImpactFramework/profile/Persistent/tiaf.json"
        args['src_branch'] = "123"
        args['dst_branch'] = "123"
        args['commit'] = "foobar"
        args['build_number'] = 1
        args['suite'] = "main"
        args['test_failure_policy'] = "continue"
        return args

    @pytest.fixture
    def mock_runtime(self, mocker):
        mocker.patch('subprocess.run')

    @pytest.fixture(autouse=True)
    def mock_uuid(self, mocker):
        universal_uuid = uuid.uuid4()
        mocker.patch('uuid.uuid4', return_value=universal_uuid)
        return universal_uuid
    
    @pytest.fixture
    def default_runtime_args(self, mock_uuid, build_root):
        runtime_args = {}
        runtime_args['bin'] = build_root+"\\bin\\profile\\tiaf.exe"
        runtime_args['sequence'] = "--sequence=seed"
        runtime_args['safemode'] = "--safemode=off"
        runtime_args['test_failure_policy'] = "--fpolicy=continue"
        runtime_args['report'] = "--report=C:\\o3de\\build\\bin\\TestImpactFramework\\profile\\Temp\\report."+mock_uuid.hex+".json"
        runtime_args['suite'] = "--suite=main"
        return runtime_args

    def test_valid_config(self, caplog, tiaf_args, mock_runtime, mocker, default_runtime_args):
        # given:
        args = tiaf_args

        # when:
        main(args)
        # then:
        subprocess.run.assert_called_with(list(default_runtime_args.values()))