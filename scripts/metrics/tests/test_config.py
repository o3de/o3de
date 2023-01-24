"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for metrics_xml_to_csv.py
"""
import unittest
import unittest.mock as mock

import common.config


class TestConfig(unittest.TestCase):

    @mock.patch('builtins.open', mock.MagicMock())
    @mock.patch('yaml.safe_load')
    def test_LoadConfig_Override_LoadsCorrectly(self, mock_safe_load):
        mock_path = 'mock_path'
        mock_safe_load.return_value = {"mock_key": "mock_value"}
        new_mock_value = "mock_value2"
        mock_override = [['mock_key', new_mock_value]]

        common.config.load_config(mock_path, mock_override)
        under_test = common.config.read_config('mock_key')

        assert under_test == new_mock_value

    @mock.patch('builtins.open', mock.MagicMock())
    @mock.patch('yaml.safe_load')
    def test_ReadConfig_Override_ReadsCorrectly(self, mock_safe_load):
        mock_path = 'mock_path'
        mock_safe_load.return_value = {"mock_key": "mock_value"}
        mock_override = {'mock_key': 'mock_value2'}

        common.config.load_config(mock_path)
        under_test = common.config.read_config('mock_key', mock_override)

        assert under_test == 'mock_value2'

    @mock.patch('builtins.open', mock.MagicMock())
    @mock.patch('yaml.safe_load')
    def test_ReadConfig_NoOverride_ReadsCorrectly(self, mock_safe_load):
        mock_path = 'mock_path'
        mock_safe_load.return_value = {"mock_key": "mock_value"}

        common.config.load_config(mock_path)
        under_test = common.config.read_config('mock_key')

        assert under_test == 'mock_value'
