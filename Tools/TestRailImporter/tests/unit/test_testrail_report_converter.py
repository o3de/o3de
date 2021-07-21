"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for ~/lib/testrail_importer/testrail_tools/testrail_report_converter.py
"""
try:  # py2
    import mock
except ImportError:  # py3
    import unittest.mock as mock
import pytest

import lib.testrail_importer.testrail_tools.testrail_settings as settings
import lib.testrail_importer.testrail_tools.testrail_report_converter as testrail_report_converter

mock_testrail_report_converter = testrail_report_converter.TestRailReportConverter()


@pytest.mark.unit
class TestReportConverter:

    def test_EmptyStrings_DictHasEmptyStrings_ReturnsDictWithNone(self):
        under_test = mock_testrail_report_converter._empty_strings_to_none(dictionary={'key1': 'value',
                                                                                       'key2': ''})

        assert under_test['key1'] == 'value'
        assert under_test['key2'] is None

    @mock.patch(
        'lib.testrail_importer.testrail_tools.testrail_report_converter.TestRailReportConverter._parse_test_case_ids')
    @mock.patch(
        'lib.testrail_importer.testrail_tools.testrail_report_converter.TestRailReportConverter._parse_test_status_ids')
    @mock.patch('lib.testrail_importer.testrail_tools.testrail_report_converter.ElementTree')
    def test_ProcessSingleSuite_HappyPath_ReturnsList(self, mock_element_tree, mock_parse_status_ids,
                                                      mock_parse_case_ids):
        mock_log_node = mock.MagicMock()
        mock_log_node.text = 'dummy log text'
        mock_case_node = mock.MagicMock()
        mock_property_node = mock.MagicMock()
        mock_property_node.attrib = {
            'name': 'test_case_id',
            'value': 'C1'  # Please include the 'C' so the parsing is tested.
        }
        mock_case_node.attrib = {
            'classname': 'test_module.TestClassName',
            'line': '1',
            'name': 'test_TestMethodName_TestCondition_TestExpectedResult',
            'file': 'test_module.py',
            'time': '1.111111111'
        }
        mock_case_node.get.return_value = None
        mock_case_node.findall.return_value = [
            mock_log_node]  # mock for case_node.findall("system-out")

        mock_parse_status_ids.return_value = {
            'custom_test_failure_message': 'dummy_failure',
            'custom_failure_text_location': 'dummy_location',
            'status_id': 1
        }
        mock_parse_case_ids.return_value = [1, 2]
        mock_element_tree.attrib = {'name': 'dummy_name'}
        mock_element_tree.findall.return_value = [
            mock_case_node]  # mock for test_suite_xml_node.findall('testcase')
        mock_element_tree.getchildren.side_effect = [  # mock for case_node.getchildren()[0].getchildren()
            mock_property_node,
            [mock_property_node]
        ]

        under_test = mock_testrail_report_converter._process_single_test_suite(mock_element_tree)

        assert under_test == [
            {'case_ids': mock_parse_case_ids.return_value,
             'custom_automated_test_name': mock_case_node.attrib['name'],
             'custom_elapse': mock_case_node.attrib['time'],
             'custom_failure_text_location': mock_parse_status_ids.return_value['custom_failure_text_location'],
             'custom_runner': mock_element_tree.attrib['name'],
             'custom_test_failure_message': mock_parse_status_ids.return_value['custom_test_failure_message'],
             'log': mock_log_node.text,
             'status_id': mock_parse_status_ids.return_value['status_id']}
        ]

    @mock.patch(
        'lib.testrail_importer.testrail_tools.testrail_report_converter.TestRailReportConverter._parse_failure_result')
    def test_ParseStatusIDs_PassStatus_ReturnPassDict(self, mock_parse_failure):
        mock_log_node = mock.MagicMock()
        mock_log_node.text = 'dummy log text'
        mock_case_node = mock.MagicMock()
        mock_case_node.attrib = {
            'classname': 'test_module.TestClassName',
            'line': '1',
            'name': 'test_TestMethodName_TestCondition_TestExpectedResult',
            'file': 'test_module.py',
            'time': '1.111111111'
        }
        mock_case_node.get.return_value = None
        mock_case_node.findall.return_value = [
            mock_log_node]  # mock for case_node.findall("system-out")
        mock_case_node.find.return_value = None  # mock for case_node.find("error"), case_node.find("skipped")
        mock_case_node.findall.return_value = None  # mock for case_node.findall("failure")
        mock_parse_failure.return_value = ''

        under_test = mock_testrail_report_converter._parse_test_status_ids(mock_case_node)

        assert under_test['status_id'] == settings.TESTRAIL_STATUS_IDS['pass_id']

    @mock.patch(
        'lib.testrail_importer.testrail_tools.testrail_report_converter.TestRailReportConverter._parse_failure_result')
    def test_ParseStatusIDs_SkippedStatus_ReturnSkippedDict(self, mock_parse_failure):
        mock_log_node = mock.MagicMock()
        mock_log_node.text = 'dummy log text'
        mock_case_node = mock.MagicMock()
        mock_case_node.attrib = {
            'classname': 'test_module.TestClassName',
            'line': '1',
            'name': 'test_TestMethodName_TestCondition_TestExpectedResult',
            'file': 'test_module.py',
            'time': '1.111111111'
        }
        mock_case_node.get.return_value = None
        mock_case_node.findall.return_value = [
            mock_log_node]  # mock for case_node.findall("system-out")
        mock_case_node.find.side_effect = [  # mock for case_node.find("error"), case_node.find("skipped")
            None,
            mock_case_node
        ]
        mock_case_node.findall.return_value = None  # mock for case_node.findall("failure")
        mock_parse_failure.return_value = ''

        under_test = mock_testrail_report_converter._parse_test_status_ids(mock_case_node)

        assert under_test['status_id'] == settings.TESTRAIL_STATUS_IDS['na_id']

    def test_ParseStatusIDs_ErrorStatus_ReturnFailureDict(self):
        mock_log_node = mock.MagicMock()
        mock_log_node.text = 'dummy log text'
        mock_case_node = mock.MagicMock()
        mock_case_node.attrib = {
            'classname': 'test_module.TestClassName',
            'line': '1',
            'name': 'test_TestMethodName_TestCondition_TestExpectedResult',
            'file': 'test_module.py',
            'time': '1.111111111'
        }
        mock_case_node.get.return_value = None
        mock_case_node.findall.return_value = [
            mock_log_node]  # mock for case_node.findall("system-out")
        mock_error_node = mock.MagicMock()
        mock_error_node.attrib = {'message': 'dummy error message'}
        mock_error_node.text = 'dummy error location'
        mock_case_node.find.side_effect = [  # mock for case_node.find("error"), case_node.find("skipped")
            mock_error_node,
            None
        ]
        mock_case_node.findall.return_value = None  # mock for case_node.findall("failure")

        under_test = mock_testrail_report_converter._parse_test_status_ids(mock_case_node)

        assert under_test['status_id'] == settings.TESTRAIL_STATUS_IDS['fail_id']
        assert under_test['custom_test_error_message'] == 'dummy error message'
        assert under_test['custom_error_text_location'] == 'dummy error location'

    def test_ParseStatusIDs_FailureStatus_ReturnFailureDict(self):
        mock_log_node = mock.MagicMock()
        mock_log_node.text = 'dummy log text'
        mock_case_node = mock.MagicMock()
        mock_case_node.attrib = {
            'classname': 'test_module.TestClassName',
            'line': '1',
            'name': 'test_TestMethodName_TestCondition_TestExpectedResult',
            'file': 'test_module.py',
            'time': '1.111111111'
        }
        mock_case_node.get.return_value = None
        mock_case_node.findall.return_value = [
            mock_log_node]  # mock for case_node.findall("system-out")
        mock_failure_node = mock.MagicMock()
        mock_failure_node.attrib = {'message': 'dummy failure message'}
        mock_failure_node.text = 'dummy failure text'
        mock_failure_nodes = [mock_failure_node, mock_failure_node]
        mock_case_node.find.return_value = None  # mock for case_node.find("error"), case_node.find("skipped")
        mock_case_node.findall.return_value = mock_failure_nodes  # mock for case_node.findall("failure")

        under_test = mock_testrail_report_converter._parse_test_status_ids(mock_case_node)

        assert under_test['status_id'] == settings.TESTRAIL_STATUS_IDS['fail_id']
        assert under_test['failure_message'] == (
            '-------------------- Failure 1 --------------------\n\n'
            'dummy failure message\n\n'
            '-------------------- Failure 2 --------------------\n\n'
            'dummy failure message\n'
        )
        assert under_test['failure_text_location'] == (
            '-------------------- Failure 1 --------------------\n\n'
            'dummy failure text\n\n'
            '-------------------- Failure 2 --------------------\n\n'
            'dummy failure text\n'
        )

    def test_ParseCaseIDs_HappyPath_ReturnsListOfInts(self):
        mock_property_node = mock.MagicMock()
        mock_property_node.attrib = {
            'name': 'test_case_id',
            'value': 'C1'  # Please include the 'C' so the parsing is tested.
        }
        under_test = mock_testrail_report_converter._parse_test_case_ids(
            [mock_property_node],
            testrail_report_converter.ID_KEY)

        assert under_test == [1]

    def test_ParseFailure_HasOneFailure_ReturnsFailureDict(self):
        mock_failure_node = mock.MagicMock()
        mock_failure_node.attrib = {'message': 'dummy failure message'}
        mock_failure_node.text = 'dummy failure text'
        under_test = mock_testrail_report_converter._parse_failure_result(
            None,
            [mock_failure_node])

        assert under_test['custom_test_failure_message'] == 'dummy failure message'
        assert under_test['custom_failure_text_location'] == 'dummy failure text'

    @mock.patch(
        'lib.testrail_importer.testrail_tools.testrail_report_converter.TestRailReportConverter._parse_test_case_ids')
    @mock.patch(
        'lib.testrail_importer.testrail_tools.testrail_report_converter.TestRailReportConverter._parse_test_status_ids')
    @mock.patch('lib.testrail_importer.testrail_tools.testrail_report_converter.ElementTree')
    def test_XMLToList_HappyPath_ReturnsListOfDicts(self, mock_element_tree, mock_parse_status_ids,
                                                    mock_parse_case_ids):
        mock_tree = mock.MagicMock()
        mock_log_node = mock.MagicMock()
        mock_log_node.text = 'dummy log text'
        mock_property_node = mock.MagicMock()
        mock_property_node.attrib = {
            'name': 'test_case_id',
            'value': 'C1'  # Please include the 'C' so the parsing is tested.
        }
        mock_case_node = mock.MagicMock()
        mock_case_node.attrib = {
            'classname': 'test_module.TestClassName',
            'line': '1',
            'name': 'test_TestMethodName_TestCondition_TestExpectedResult',
            'file': 'test_module.py',
            'time': '1.111111111'
        }
        mock_case_node.get.return_value = None
        mock_case_node.findall.return_value = [
            mock_log_node]  # mock for case_node.findall("system-out")
        mock_tree.getroot.return_value = mock_case_node
        mock_element_tree.parse.return_value = mock_tree
        mock_parse_status_ids.return_value = {
            'custom_test_failure_message': 'dummy_failure',
            'custom_failure_text_location': 'dummy_location',
            'status_id': 1
        }
        mock_parse_case_ids.return_value = [1, 2]
        mock_element_tree.attrib = {'name': 'dummy_name'}
        mock_element_tree.findall.return_value = [
            mock_case_node]  # mock for test_suite_xml_node.findall('testcase')
        mock_element_tree.getchildren.side_effect = [  # mock for case_node.getchildren()[0].getchildren()
            mock_property_node,
            [mock_property_node]
        ]

        under_test = mock_testrail_report_converter.xml_to_list_of_dicts("path/to/xml/file.xml")

        mock_element_tree.parse.assert_called_once()
        mock_tree.getroot.assert_called_once()

        for test in under_test:
            assert test['status_id'] == settings.TESTRAIL_STATUS_IDS['pass_id']
