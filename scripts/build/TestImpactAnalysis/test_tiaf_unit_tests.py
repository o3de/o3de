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
import json
from pathlib import Path
import pytest
import subprocess
import uuid

logging = getLogger("tiaf")


class TestTIAFUnitTests():

    @pytest.fixture
    def test_data_file(self):
        path = Path("scripts/build/TestImpactAnalysis/Testing/test_data.json")
        with open(path) as file:
            return json.load(file)

    @pytest.fixture(scope='module', params=['profile', 'debug'])
    def build_type(self, request):
        return request.param

    @pytest.fixture
    def config_path(self, build_type, test_data_file):
        return test_data_file['configs'][build_type]

    @pytest.fixture
    def binary_path(self, build_type, test_data_file):
        return test_data_file['binary'][build_type]

    @pytest.fixture()
    def report_path(self, build_type, test_data_file, mock_uuid):
        return test_data_file['report_dir'][build_type]+"\\report."+mock_uuid.hex+".json"

    @pytest.fixture
    def config_data(self, config_path):
        with open(config_path) as f:
            return json.load(f)

    @pytest.fixture
    def tiaf_args(self, config_path):
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
    def mock_runtime(self, mocker):
        return mocker.patch('subprocess.run')

    @pytest.fixture(autouse=True)
    def mock_uuid(self, mocker):
        universal_uuid = uuid.uuid4()
        mocker.patch('uuid.uuid4', return_value=universal_uuid)
        return universal_uuid

    
 
    @pytest.fixture
    def default_runtime_args(self, mock_uuid, binary_path, report_path):
        runtime_args = {}
        runtime_args['bin'] = str(binary_path).replace("/","\\")
        runtime_args['sequence'] = "--sequence=seed" 
        runtime_args['safemode'] = "--safemode=off"
        runtime_args['test_failure_policy'] = "--fpolicy=continue"
        runtime_args['report'] = "--report="+str(report_path).replace("/","\\")
        runtime_args['suite'] = "--suite=main"
        return runtime_args

    def test_config_files_exist(self, config_path):
        # given:
        # config path, and files have been built
        # when:
        # we look for the file
        # then:
        assert os.path.exists(config_path)

    def test_valid_config(self, caplog, tiaf_args, mock_runtime, mocker, default_runtime_args):
        # given:
        # default arguments
        # when:
        main(tiaf_args)
        # then:
        subprocess.run.assert_called_with(list(default_runtime_args.values()))

    def test_invalid_config(self, caplog, tiaf_args, mock_runtime, tmp_path_factory, default_runtime_args):
        # given:
        invalid_file = tmp_path_factory.mktemp("test") / "test_file.txt"
        with open(invalid_file, 'w+') as f:
            f.write("fake json")
        tiaf_args['config'] = invalid_file
        test_string = "Exception caught by TIAF driver: 'Config file does not contain valid JSON, stopping TIAF'."

        # when:
        main(tiaf_args)

        # then:
        assert test_string in caplog.messages 

    def test_valid_src_branch(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['src_branch'] = "development"
        test_string = "Src branch: 'development'."
        # when:
        main(tiaf_args)

        # then:
        assert test_string in caplog.messages
    
    def test_invalid_src_branch(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['src_branch'] = "not_a_real_branch"
        test_string = "Src branch: 'not_a_real_branch'."

        # when:
        main(tiaf_args)

        # then:
        assert test_string in caplog.messages

    def test_valid_dst_branch(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['dst_branch'] = "development"
        test_string = "Dst branch: 'development'."
        # when:
        main(tiaf_args)

        # then:
        assert test_string in caplog.messages
    
    def test_invalid_dst_branch(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['dst_branch'] = "not_a_real_branch"
        test_string = "Dst branch: 'not_a_real_branch'."

        # when:
        main(tiaf_args)

        # then:
        assert test_string in caplog.messages

    def test_valid_commit(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['commit'] = "9a15f038807ba8b987c9e689952d9271ef7fd086"
        test_string = "Commit: '9a15f038807ba8b987c9e689952d9271ef7fd086'."

        # when:
        main(tiaf_args)

        # then:
        assert test_string in caplog.messages
    
    def test_invalid_commit(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['commit'] = "foobar"
        test_string = "Commit: 'foobar'."

        # when:
        main(tiaf_args)

        # then:
        assert test_string in caplog.messages

    def test_no_s3_bucket_name(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        # default args
        test_string = "Attempting to access persistent storage for the commit 'foobar' for suite 'main'"
        
        # when:
        main(tiaf_args)

        # then:
        assert test_string in caplog.messages

    def test_s3_bucket_name_valid_credentials(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        pass

    

    