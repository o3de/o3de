"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for metrics_xml_to_csv.py
"""
import unittest
import unittest.mock as mock
import os

import ctest_metrics_xml_to_csv


class TestMetricsXMLtoCSV(unittest.TestCase):

    @mock.patch("ctest_metrics_xml_to_csv.datetime.datetime")
    def test_GetDefaultCSVFilename_SingleDigit_HasZeroes(self, mock_datetime):
        mock_date = mock.MagicMock()
        mock_date.month = 1
        mock_date.day = 2
        mock_date.year = "xxxx"
        mock_datetime.now.return_value = mock_date

        under_test = ctest_metrics_xml_to_csv._get_default_csv_filename()

        assert under_test == "xxxx_01_02.csv"

    @mock.patch("ctest_metrics_xml_to_csv.datetime.datetime")
    def test_GetDefaultCSVFilename_DoubleDigit_NoZeroes(self, mock_datetime):
        mock_date = mock.MagicMock()
        mock_date.month = 11
        mock_date.day = 12
        mock_date.year = "xxxx"
        mock_datetime.now.return_value = mock_date

        under_test = ctest_metrics_xml_to_csv._get_default_csv_filename()

        assert under_test == "xxxx_11_12.csv"

    @mock.patch('os.path.exists', mock.MagicMock(side_effect=[True, True]))
    @mock.patch('builtins.open')
    def test_GetTestXMLPath_PathExists_ReturnsCorrect(self, mock_open):
        mock_build_path = 'mock_build_path'
        mock_xml_file = 'mock_xml_file'
        mock_opened_file = mock.MagicMock()
        mock_opened_file.__enter__.return_value.readline.return_value = 'mock_folder_name'
        mock_open.return_value = mock_opened_file

        under_test = ctest_metrics_xml_to_csv._get_test_xml_path(mock_build_path, mock_xml_file)

        assert under_test == os.path.join(mock_build_path, ctest_metrics_xml_to_csv.TESTING_DIR, 'mock_folder_name',
                                          mock_xml_file)
