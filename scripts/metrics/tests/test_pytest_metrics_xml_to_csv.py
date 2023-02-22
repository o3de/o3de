"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for metrics_xml_to_csv.py
"""
import unittest
import unittest.mock as mock

import pytest_metrics_xml_to_csv


class TestMetricsXMLtoCSV(unittest.TestCase):

    def test_DetermineTestResult_FailedResult_ReturnsCorrectly(self):
        mock_node = mock.MagicMock()
        mock_node.find.return_value = True

        under_test = pytest_metrics_xml_to_csv._determine_test_result(mock_node)

        assert under_test == 'failed'

    def test_DetermineTestResult_PassedResult_ReturnsCorrectly(self):
        mock_node = mock.MagicMock()
        mock_node.find.return_value = None

        under_test = pytest_metrics_xml_to_csv._determine_test_result(mock_node)

        assert under_test == 'passed'

    @mock.patch('pytest_metrics_xml_to_csv._determine_test_result')
    @mock.patch('ly_test_tools.cli.codeowners_hint.get_codeowners')
    @mock.patch('xml.etree.ElementTree.parse')
    def test_ParsePytestXMLToCsv_HappyPath_WorksCorrectly(self, mock_parse, mock_get_codeowners, mock_determine_results):
        mock_xml = mock.MagicMock()
        mock_entry = mock.MagicMock()
        mock_entry.attrib = {
            'name': 'mock_test_name',
            'time': 1,
            'file': mock.MagicMock(),
        }
        mock_testcases = [
            mock_entry
        ]
        mock_xml.findall.return_value = mock_testcases
        mock_get_codeowners.return_value = (None, 'mock_codeowner', None)
        mock_parse.return_value.getroot.return_value = mock_xml
        mock_writer = mock.MagicMock()
        mock_xml_path = mock.MagicMock()
        mock_determine_results.return_value = 'passed'

        pytest_metrics_xml_to_csv.parse_pytest_xmls_to_csv(mock_xml_path, mock_writer)

        mock_writer.writerow.assert_called_once_with({
            'test_name': 'mock_test_name',
            'duration_seconds': 1,
            'status': 'passed',
            'sig_owner': 'mock_codeowner'
        })
