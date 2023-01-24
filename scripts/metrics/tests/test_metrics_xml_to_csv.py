"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for metrics_xml_to_csv.py
"""
import unittest
import unittest.mock as mock
import os

import metrics_xml_to_csv


class TestMetricsXMLtoCSV(unittest.TestCase):

    @mock.patch("metrics_xml_to_csv.now_wrap")
    def test_GetDefaultCSVFilename_SingleDigit_HasZeroes(self, mock_now):
        mock_datetime = mock.MagicMock()
        mock_datetime.month = 1
        mock_datetime.day = 2
        mock_datetime.year = "xxxx"
        mock_now.return_value = mock_datetime

        metrics_xml_to_csv._get_default_csv_filename()

        assert metrics_xml_to_csv.DEFAULT_CSV_FILE == "xxxx_01_02.csv"

    @mock.patch("metrics_xml_to_csv.now_wrap")
    def test_GetDefaultCSVFilename_DoubleDigit_NoZeroes(self, mock_now):
        mock_datetime = mock.MagicMock()
        mock_datetime.month = 11
        mock_datetime.day = 12
        mock_datetime.year = "xxxx"
        mock_now.return_value = mock_datetime

        metrics_xml_to_csv._get_default_csv_filename()

        assert metrics_xml_to_csv.DEFAULT_CSV_FILE == "xxxx_11_12.csv"

    @mock.patch('os.path.exists', mock.MagicMock(side_effect=[True, True]))
    @mock.patch('builtins.open')
    @mock.patch('metrics_xml_to_csv.read_config')
    def test_GetTestXMLPath_(self, mock_read_config, mock_open):
        mock_read_config.side_effect = ['mock_testing_dir', 'mock_tag_file']
        mock_build_path = 'mock_build_path'
        mock_xml_file = 'mock_xml_file'
        mock_opened_file = mock.MagicMock()
        mock_opened_file.__enter__.return_value.readline.return_value = 'mock_folder_name'
        mock_open.return_value = mock_opened_file

        under_test = metrics_xml_to_csv._get_test_xml_path(mock_build_path, mock_xml_file)

        assert under_test == os.path.join(mock_build_path, 'mock_testing_dir', 'mock_folder_name', mock_xml_file)
