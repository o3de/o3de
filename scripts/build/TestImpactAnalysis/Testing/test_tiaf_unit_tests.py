#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


from logging import getLogger
from typing import Counter
from test_impact import CPPTestImpact as TestImpact
from tiaf_driver import main
import pytest
logging = getLogger("tiaf")


def assert_list_content_equal(list1, list2):
    assert Counter(list1) == Counter(list2)


class TestTiafInitialiseStorage():

    def test_create_TestImpact_no_s3_bucket_name(self, caplog, tiaf_args, config_data, mocker):
        # given:
        # default args
        expected_storage_args = config_data, tiaf_args['suite'], tiaf_args[
            'commit']
        mock_local = mocker.patch(
            "persistent_storage.PersistentStorageLocal.__init__", side_effect=SystemError(), return_value=None)
        # when:
        tiaf = TestImpact(tiaf_args)

        # then:
        mock_local.assert_called_with(
            *expected_storage_args)

    @pytest.mark.parametrize("bucket_name,top_level_dir", [("test_bucket", None), ("test_bucket", "test_dir")])
    def test_create_TestImpact_s3_bucket_name_supplied(self, caplog, tiaf_args, mocker, bucket_name, top_level_dir, config_data):
        # given:
        tiaf_args['s3_bucket'] = bucket_name
        tiaf_args['s3_top_level_dir'] = top_level_dir
        mock_storage = mocker.patch(
            "persistent_storage.PersistentStorageS3.__init__", side_effect=SystemError())

        expected_storage_args = config_data, tiaf_args['suite'], tiaf_args[
            'commit'], bucket_name, top_level_dir, tiaf_args['src_branch']
        # when:
        tiaf = TestImpact(tiaf_args)

        # then:
        mock_storage.assert_called_with(*expected_storage_args)

    def test_create_TestImpact_s3_bucket_name_not_supplied(self, caplog, tiaf_args, mock_runtime, default_runtime_args, mocker, config_data):
        # given:
        tiaf_args['s3_bucket'] = None
        tiaf_args['s3_top_level_dir'] = None
        mock_storage = mocker.patch(
            "persistent_storage.PersistentStorageS3.__init__", return_value=None)

        # when:
        tiaf = TestImpact(tiaf_args)

        # then:
        mock_storage.assert_not_called()

    def test_create_TestImpact_s3_top_level_dir_bucket_name_not_supplied(self, caplog, tiaf_args, mock_runtime, default_runtime_args, mocker, config_data):
        # given:
        tiaf_args['s3_bucket'] = None
        tiaf_args['s3_top_level_dir'] = "test_dir"
        mock_storage = mocker.patch(
            "persistent_storage.PersistentStorageS3.__init__", return_value=None)

        expected_storage_args = config_data, tiaf_args['suite'], tiaf_args[
            'commit'], None, None, tiaf_args['src_branch']
        # when:
        tiaf = TestImpact(tiaf_args)

        # then:
        mock_storage.assert_not_called()


class TestTIAFUnitTests():

    def test_create_TestImpact_valid_config(self, caplog, tiaf_args, mock_runtime, mocker, default_runtime_args):
        # given:
        # default arguments
        # when:
        tiaf = TestImpact(tiaf_args)
        # then:
        assert_list_content_equal(
            tiaf.runtime_args, default_runtime_args.values())

    def test_create_TestImpact_invalid_config(self, caplog, tiaf_args, mock_runtime, tmp_path_factory, default_runtime_args):
        # given:
        invalid_file = tmp_path_factory.mktemp("test") / "test_file.txt"
        with open(invalid_file, 'w+') as f:
            f.write("fake json")
        tiaf_args['config'] = invalid_file

        # when & then:
        with pytest.raises(SystemError) as exc_info:
            tiaf = TestImpact(tiaf_args)

    @ pytest.mark.parametrize("branch_name", ["development", "not_a_real_branch"])
    def test_create_TestImpact_src_branch(self, caplog, tiaf_args, mock_runtime, default_runtime_args, branch_name):
        # given:
        tiaf_args['src_branch'] = branch_name
        # when:
        tiaf = TestImpact(tiaf_args)
        # then:
        assert tiaf.source_branch == branch_name

    @ pytest.mark.parametrize("branch_name", ["development", "not_a_real_branch"])
    def test_create_TestImpact_dst_branch(self, caplog, tiaf_args, mock_runtime, default_runtime_args, branch_name):
        # given:
        tiaf_args['dst_branch'] = branch_name
        # when:
        tiaf = TestImpact(tiaf_args)

        # then:
        assert tiaf.destination_branch == branch_name

    @ pytest.mark.parametrize("commit", ["9a15f038807ba8b987c9e689952d9271ef7fd086", "foobar"])
    def test_create_TestImpact_commit(self, caplog, tiaf_args, mock_runtime, default_runtime_args, commit):
        # given:
        tiaf_args['commit'] = commit

        # when:
        tiaf = TestImpact(tiaf_args)

        # then:
        assert tiaf.destination_commit == commit

    def test_run_Tiaf_mars_index_prefix_is_supplied(self, caplog, tiaf_args, mock_runtime, mocker):
        # given:
        tiaf_args['mars_index_prefix'] = "test_prefix"
        mock_mars = mocker.patch("mars_utils.transmit_report_to_mars")
        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        mock_mars.assert_called()

    def test_run_Tiaf_mars_index_prefix_is_not_supplied(self, caplog, tiaf_args, mock_runtime, mocker):
        # given:
        # default_args
        mock_mars = mocker.patch("mars_utils.transmit_report_to_mars")
        # when:
        with pytest.raises(SystemExit):
            main(tiaf_args)

        # then:
        mock_mars.assert_not_called()

    def test_create_TestImpact_valid_test_suite_name(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        # default args

        # when:
        tiaf = TestImpact(tiaf_args)
        # then:
        assert_list_content_equal(
            tiaf.runtime_args, default_runtime_args.values())

    def test_create_TestImpact_invalid_test_suite_name(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['suite'] = "foobar"
        default_runtime_args['suite'] = "--suite=foobar"
        # when:
        tiaf = TestImpact(tiaf_args)

        # then:
        assert_list_content_equal(
            tiaf.runtime_args, default_runtime_args.values())

    @ pytest.mark.parametrize("policy", ["continue", "abort", "ignore"])
    def test_create_TestImpact_valid_failure_policy(self, caplog, tiaf_args, mock_runtime, default_runtime_args, policy):
        # given:
        tiaf_args['test_failure_policy'] = policy
        default_runtime_args['test_failure_policy'] = "--fpolicy="+policy

        # set src branch to be different than dst branch so that we can get a regular sequence, otherwise TIAF will default to a see sequence and overwrite our failure policy
        tiaf_args['src_branch'] = 122
        default_runtime_args['sequence'] = "--sequence=regular"
        default_runtime_args['ipolicy'] = "--ipolicy=continue"
        # when:
        tiaf = TestImpact(tiaf_args)
        # then:
        assert_list_content_equal(
            tiaf.runtime_args, default_runtime_args.values())

    @ pytest.mark.parametrize("safemode, arg_val", [(True, "on"), (None, "off")])
    def test_create_TestImpact_safe_mode_arguments(self, caplog, tiaf_args, mock_runtime, default_runtime_args, safemode, arg_val):
        # given:
        tiaf_args['safe_mode'] = safemode
        default_runtime_args['safemode'] = "--safemode="+arg_val

        # when:
        tiaf = TestImpact(tiaf_args)
        # then:
        assert_list_content_equal(
            tiaf.runtime_args, default_runtime_args.values())

    def test_create_TestImpact_exclude_file_not_supplied(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        # default args

        # when:
        tiaf = TestImpact(tiaf_args)

        # then:
        assert_list_content_equal(
            tiaf.runtime_args, default_runtime_args.values())

    def test_create_TestImpact_exclude_file_supplied(self, caplog, tiaf_args, mock_runtime, default_runtime_args):
        # given:
        tiaf_args['exclude_file'] = "testpath"
        default_runtime_args['exclude'] = "--exclude=testpath"
        # when:
        tiaf = TestImpact(tiaf_args)

        # then:
        assert_list_content_equal(
            tiaf.runtime_args, default_runtime_args.values())
