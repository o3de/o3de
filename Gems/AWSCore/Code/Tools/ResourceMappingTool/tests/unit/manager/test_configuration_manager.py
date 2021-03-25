"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from typing import List
from unittest import TestCase
from unittest.mock import (MagicMock, patch)

from manager.configuration_manager import ConfigurationManager
from model import constants


class TestConfigurationManager(TestCase):
    """
    ConfigurationManager unit test cases
    """
    _expected_directory_path: str = "dummy/directory/"
    _expected_config_files: List[str] = ["dummy.json"]
    _expected_account_id: str = "1234567890"
    _expected_region: str = "aws-global"
    _expected_configuration_manager: ConfigurationManager = ConfigurationManager()

    def test_get_instance_return_same_instance(self) -> None:
        assert ConfigurationManager.get_instance() is TestConfigurationManager._expected_configuration_manager

    def test_get_instance_raise_exception(self) -> None:
        self.assertRaises(Exception, ConfigurationManager)

    @patch("utils.aws_utils.get_default_region", return_value=_expected_region)
    @patch("utils.aws_utils.get_default_account_id", return_value=_expected_account_id)
    @patch("utils.file_utils.find_files_with_suffix_under_directory", return_value=_expected_config_files)
    @patch("utils.file_utils.check_path_exists", return_value=True)
    @patch("utils.file_utils.get_current_directory_path", return_value=_expected_directory_path)
    def test_setup_get_configuration_setup_as_expected(self, mock_get_current_directory_path: MagicMock,
                                                       mock_check_path_exists: MagicMock,
                                                       mock_find_files_with_suffix_under_directory: MagicMock,
                                                       mock_get_default_account_id: MagicMock,
                                                       mock_get_default_region: MagicMock) -> None:
        TestConfigurationManager._expected_configuration_manager.setup()
        mock_get_current_directory_path.assert_called_once()
        mock_check_path_exists.assert_called_once_with(TestConfigurationManager._expected_directory_path)
        mock_find_files_with_suffix_under_directory.assert_called_once_with(
            TestConfigurationManager._expected_directory_path, constants.RESOURCE_MAPPING_CONFIG_FILE_NAME_SUFFIX)
        mock_get_default_account_id.assert_called_once()
        mock_get_default_region.assert_called_once()
        assert TestConfigurationManager._expected_configuration_manager.configuration.config_directory == \
               TestConfigurationManager._expected_directory_path
        assert TestConfigurationManager._expected_configuration_manager.configuration.config_files == \
               TestConfigurationManager._expected_config_files
        assert TestConfigurationManager._expected_configuration_manager.configuration.account_id == \
               TestConfigurationManager._expected_account_id
        assert TestConfigurationManager._expected_configuration_manager.configuration.region == \
               TestConfigurationManager._expected_region
