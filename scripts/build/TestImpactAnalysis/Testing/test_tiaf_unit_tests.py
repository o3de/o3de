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
import pytest
from tiaf_persistent_storage_s3 import PersistentStorageS3

logging = getLogger("tiaf")


class TestTIAFUnitTests():


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

    @pytest.mark.parametrize("bucket_name,top_level_dir",[("test_bucket", None), ("test_bucket", "test_dir")])
    def test_s3_bucket_name(self, caplog, tiaf_args, mock_runtime, default_runtime_args, mocker, bucket_name, top_level_dir, config_data):
        # given:
        tiaf_args['s3_bucket'] = bucket_name
        tiaf_args['s3_top_level_dir'] = top_level_dir
        mock_storage = mocker.patch("tiaf_persistent_storage_s3.PersistentStorageS3.__init__", return_value = None)

        expected_storage_args = config_data, tiaf_args['suite'], tiaf_args['commit'], bucket_name, top_level_dir, tiaf_args['src_branch']
        # when:
        main(tiaf_args)

        # then:
        mock_storage.assert_called_with(*expected_storage_args)

    def test_s3_bucket_name_not_supplied(self, caplog, tiaf_args, mock_runtime, default_runtime_args, mocker, config_data):
        # given:
        tiaf_args['s3_bucket'] = None
        tiaf_args['s3_top_level_dir'] = None
        mock_storage = mocker.patch("tiaf_persistent_storage_s3.PersistentStorageS3.__init__", return_value = None)

        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        mock_storage.assert_not_called()

    def test_s3_top_level_dir_bucket_name_not_supplied(self, caplog, tiaf_args, mock_runtime, default_runtime_args, mocker, config_data):
        # given:
        tiaf_args['s3_bucket'] = None
        tiaf_args['s3_top_level_dir'] = "test_dir"
        mock_storage = mocker.patch("tiaf_persistent_storage_s3.PersistentStorageS3.__init__", return_value = None)

        expected_storage_args = config_data, tiaf_args['suite'], tiaf_args['commit'], None, None, tiaf_args['src_branch']
        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        mock_storage.assert_not_called()


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