#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


from cmath import exp
from email.policy import default
from logging import getLogger
import os
from tiaf import TestImpact
from tiaf_driver import main
import json
from pathlib import Path
import pytest
import subprocess
import uuid
from botocore.stub import Stubber
from botocore import session
from boto3 import client

logging = getLogger("tiaf")


class TestTIAFUnitTests():

    @pytest.fixture
    def s3_stub(self):
        with Stubber(client('s3')) as stubber:
            yield stubber
            stubber.assert_no_pending_responses()

    @pytest.fixture
    def mock_s3_resource(self, mocker, s3_stub):
        return mocker.patch("boto3.client", return_value=s3_stub)

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
        runtime_args['bin'] = str(binary_path).replace("/", "\\")
        runtime_args['sequence'] = "--sequence=seed"
        runtime_args['safemode'] = "--safemode=off"
        runtime_args['test_failure_policy'] = "--fpolicy=continue"
        runtime_args['report'] = "--report=" + \
            str(report_path).replace("/", "\\")
        runtime_args['suite'] = "--suite=main"
        return runtime_args

    def call_args_equal_expected_args(self, call_args, expected_args):
        set_expected_args = set(expected_args)
        for args_list in call_args:
            if set(args_list) == set_expected_args:
                return True
        return False

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
        with pytest.raises(SystemExit):
            main(tiaf_args)
        # then:
        assert self.call_args_equal_expected_args(mock_runtime.call_args.args, default_runtime_args.values())

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
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        assert test_string in caplog.messages

    def test_invalid_src_branch(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['src_branch'] = "not_a_real_branch"
        test_string = "Src branch: 'not_a_real_branch'."

        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        assert test_string in caplog.messages

    def test_valid_dst_branch(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['dst_branch'] = "development"
        test_string = "Dst branch: 'development'."
        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        assert test_string in caplog.messages

    def test_invalid_dst_branch(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['dst_branch'] = "not_a_real_branch"
        test_string = "Dst branch: 'not_a_real_branch'."

        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        assert test_string in caplog.messages

    def test_valid_commit(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['commit'] = "9a15f038807ba8b987c9e689952d9271ef7fd086"
        test_string = "Commit: '9a15f038807ba8b987c9e689952d9271ef7fd086'."

        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        assert test_string in caplog.messages

    def test_invalid_commit(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['commit'] = "foobar"
        test_string = "Commit: 'foobar'."

        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        assert test_string in caplog.messages

    def test_no_s3_bucket_name(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        # default args
        test_string = "Attempting to access persistent storage for the commit 'foobar' for suite 'main'"

        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        assert test_string in caplog.messages

    def test_s3_bucket_name_valid_credentials(self, caplog, tiaf_args, mock_runtime, default_runtime_args, s3_stub, mocker):
        tiaf_args['s3_bucket'] = "test_bucket"

        test_response = {}

        s3_stub.add_response('get_object', expected_params={
                             "Bucket": "example-bucket"}, service_response={})

        mocker.patch("boto3.client", return_value=s3_stub)
        main(tiaf_args)

    def test_mars_index_prefix_is_supplied(self, caplog, tiaf_args, mock_runtime, mocker):
        # given:
        tiaf_args['mars_index_prefix'] = "test_prefix"
        mock_mars = mocker.patch("mars_utils.transmit_report_to_mars")
        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        mock_mars.assert_called()

    def test_mars_index_prefix_is_not_supplied(self, caplog, tiaf_args, mock_runtime, mocker):
        # given:
        # default_args
        mock_mars = mocker.patch("mars_utils.transmit_report_to_mars")
        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        mock_mars.assert_not_called()

    def test_valid_test_suite_name(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        # default args

        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        assert self.call_args_equal_expected_args(mock_runtime.call_args.args, default_runtime_args.values())

    def test_invalid_test_suite_name(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['suite'] = "foobar"
        default_runtime_args['suite'] = "--suite=foobar"
        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        assert self.call_args_equal_expected_args(mock_runtime.call_args.args, default_runtime_args.values())

    @pytest.mark.parametrize("policy",["continue","abort","ignore"])
    def test_valid_failure_policy(self, caplog, tiaf_args, mock_runtime, default_runtime_args, policy):
        # given:
        tiaf_args['test_failure_policy'] = policy
        default_runtime_args['test_failure_policy'] = "--fpolicy="+policy

        # set src branch to be different than dst branch so that we can get a regular sequence, otherwise TIAF will default to a see sequence and overwrite our failure policy
        tiaf_args['src_branch'] = 122
        default_runtime_args['sequence'] = "--sequence=regular"
        default_runtime_args['ipolicy'] = "--ipolicy=continue"
        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)
        # then:
        assert self.call_args_equal_expected_args(mock_runtime.call_args.args, default_runtime_args.values())

    @pytest.mark.parametrize("safemode, arg_val",[(True, "on"),(None, "off")])
    def test_safe_mode_arguments(self, caplog, tiaf_args, mock_runtime, default_runtime_args, safemode, arg_val):
        tiaf_args['safe_mode'] = safemode
        default_runtime_args['safemode'] = "--safemode="+arg_val

        with pytest.raises(SystemExit):
            main(tiaf_args)
        # then:
        assert self.call_args_equal_expected_args(mock_runtime.call_args.args, default_runtime_args.values())

    def test_exclude_file_not_supplied(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        # default args

        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        assert self.call_args_equal_expected_args(mock_runtime.call_args.args, default_runtime_args.values())

    def test_exclude_file_supplied(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['exclude_file'] = "testpath"
        default_runtime_args['exclude'] = "--exclude=testpath"
        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)
        
        # then:
        assert self.call_args_equal_expected_args(mock_runtime.call_args.args, default_runtime_args.values())