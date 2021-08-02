"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for ly_test_tools._internal.pytest_plugin.case_id
"""

import unittest.mock as mock

import pytest

import ly_test_tools._internal.pytest_plugin.case_id as case_id

pytestmark = pytest.mark.SUITE_smoke


class TestCaseId(object):

    def test_Configure_MockConfig_ValuesAdded(self):
        mock_config = mock.MagicMock()

        case_id.pytest_configure(mock_config)

        mock_config.addinivalue_line.assert_called_once()

    def test_AddOption_MockParser_OptionsAdded(self):
        mock_parser = mock.MagicMock()

        case_id.pytest_addoption(mock_parser)

        mock_parser.addoption.assert_called_once()

    def test_MakeReportImpl_MarkerSimpleItem_TestCaseIdAdded(self):
        mock_report = mock.MagicMock()
        mock_report.when = 'call'
        mock_item = mock.MagicMock()
        mock_marker = mock.MagicMock()
        test_case = 123
        test_case_list = [str(test_case)]
        mock_marker.args = test_case_list
        mock_item.get_marker.return_value = mock_marker
        mock_xml = mock.MagicMock()
        mock_item.config._xml = mock_xml
        mock_node = mock.MagicMock()
        mock_xml.node_reporter.return_value = mock_node

        case_id._pytest_runtest_makereport_imp(mock_report, mock_item)

        mock_node.add_property.assert_called_once_with('test_case_id', test_case)

    def test_MakeReportImpl_ClosestMarkerSimpleItem_TestCaseIdAdded(self):
        mock_report = mock.MagicMock()
        mock_report.when = 'call'
        mock_item = mock.MagicMock()
        mock_marker = mock.MagicMock()
        test_case = 123
        test_case_list = [str(test_case)]
        mock_marker.args = test_case_list
        mock_item.get_marker.side_effect = AttributeError()
        mock_item.get_closest_marker.return_value = mock_marker
        mock_xml = mock.MagicMock()
        mock_item.config._xml = mock_xml
        mock_node = mock.MagicMock()
        mock_xml.node_reporter.return_value = mock_node

        case_id._pytest_runtest_makereport_imp(mock_report, mock_item)

        mock_item.get_closest_marker.assert_called_once()
        mock_node.add_property.assert_called_once_with('test_case_id', test_case)

    @mock.patch('ly_test_tools._internal.pytest_plugin.case_id.log')
    def test_MakeReportImpl_XmlReportError_WarningLogged(self, mock_logger):
        mock_report = mock.MagicMock()
        mock_report.when = 'call'
        mock_item = mock.MagicMock()
        mock_marker = mock.MagicMock()
        test_case = '123'
        test_case_list = [str(test_case)]
        mock_marker.args = test_case_list
        mock_item.get_marker.return_value = mock_marker
        mock_item.config = ''

        case_id._pytest_runtest_makereport_imp(mock_report, mock_item)

        mock_logger.warning.assert_called_once()

    def test_CollectionModifyItems_SingleTestCase_ItemsUpdated(self):
        ids = [12, 34, 56]
        id_to_filter = 34
        items = []
        deselected_items = []
        for id in ids:
            mock_item = mock.MagicMock()
            mock_marker = mock.MagicMock()
            mock_marker.args = [str(id)]
            mock_item.get_marker.return_value = mock_marker

            items.append(mock_item)

            if id != id_to_filter:
                deselected_items.append(mock_item)

        mock_config = mock.MagicMock()
        mock_config.option.test_case_ids = str(id_to_filter)

        case_id.pytest_collection_modifyitems(items, mock_config)

        assert len(items) == 1
        assert items[0].get_marker().args[0] == str(id_to_filter)
        mock_config.hook.pytest_deselected.assert_called_once_with(items=set(deselected_items))

    def test_ParseTestCaseIds_ValidStringIds_IdsParsed(self):
        ids = ['12', '34', '56']

        expected = {12, 34, 56}
        actual = case_id._parse_test_case_ids(ids)

        assert expected == actual

    def test_ParseTestCaseIds_ValidIntIds_IdsParsed(self):
        ids = [12, 34, 56]

        expected = {12, 34, 56}
        actual = case_id._parse_test_case_ids(ids)

        assert expected == actual

    def test_ParseTestCaseIds_InvalidIds_ExceptionRaised(self):
        ids = ['']

        with pytest.raises(case_id.TestCaseIDException):
            case_id._parse_test_case_ids(ids)

