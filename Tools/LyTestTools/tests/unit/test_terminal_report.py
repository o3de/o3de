"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Unit tests for ly_test_tools._internal.pytest_plugin.terminal_report
"""
import os
import pytest
import unittest.mock as mock

import ly_test_tools._internal.pytest_plugin.terminal_report as terminal_report

pytestmark = pytest.mark.SUITE_smoke


class TestTerminalReport(object):

    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.build_rerun_commands')
    def test_AddCommands_MockCommands_CommandsAdded(self, mock_build_commands):
        mock_build_commands.side_effect = lambda path, nodes: nodes
        mock_reporter = mock.MagicMock()
        header = 'This is a header'
        test_path = 'Foo'
        mock_node_ids = ['a', 'b']

        terminal_report._add_commands(mock_reporter, header, test_path, mock_node_ids)

        mock_reporter.write_line.assert_has_calls([
            mock.call(header),
            mock.call('a'),
            mock.call('b')
        ])

    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.build_rerun_commands')
    def test_AddCommands_NoCommands_ErrorWritten(self, mock_build_commands):
        mock_reporter = mock.MagicMock()
        header = 'This is a header'
        test_path = 'Foo'
        mock_node_ids = []

        terminal_report._add_commands(mock_reporter, header, test_path, mock_node_ids)

        calls = mock_reporter.write_line.mock_calls
        mock_build_commands.assert_not_called()
        assert calls[0] == mock.call(header)
        assert 'Error' in calls[1][1][0]

    @mock.patch('ly_test_tools._internal.pytest_plugin.terminal_report._add_commands')
    def test_TerminalSummary_NoErrorsNoFailures_EmptyReport(self, mock_add_commands):
        mock_report = mock.MagicMock()
        mock_report.stats.get.return_value = []

        terminal_report.pytest_terminal_summary(mock_report, 0)

        mock_add_commands.assert_not_called()
        mock_report.config.getoption.assert_not_called()
        mock_report.section.assert_not_called()

    @mock.patch('ly_test_tools._internal.pytest_plugin.terminal_report._add_commands')
    def test_TerminalSummary_ErrorsAndFailures_SectionsAdded(self, mock_add_commands):
        mock_report = mock.MagicMock()
        mock_node = mock.MagicMock()
        mock_node.nodeid = 'something'
        mock_report.stats.get.return_value = [mock_node, mock_node]

        terminal_report.pytest_terminal_summary(mock_report, 0)

        assert len(mock_add_commands.mock_calls) == 2
        mock_report.config.getoption.assert_called()
        mock_report.section.assert_called_once()

    @mock.patch('ly_test_tools._internal.pytest_plugin.terminal_report._add_commands', mock.MagicMock())
    @mock.patch('os.path.basename')
    def test_TerminalSummary_Failures_CallsWithBasename(self, mock_basename):
        mock_report = mock.MagicMock()
        mock_node = mock.MagicMock()
        mock_base = 'something'
        node_id = os.path.join('C:', mock_base)
        mock_node.nodeid = node_id
        mock_report.stats.get.side_effect = [[mock_node], []]  # first item is failure list

        terminal_report.pytest_terminal_summary(mock_report, 0)

        mock_basename.assert_called_with(node_id)

    @mock.patch('ly_test_tools._internal.pytest_plugin.terminal_report._add_commands', mock.MagicMock())
    @mock.patch('os.path.basename')
    def test_TerminalSummary_Errors_CallsWithBasename(self, mock_basename):
        mock_report = mock.MagicMock()
        mock_node = mock.MagicMock()
        mock_base = 'something'
        node_id = os.path.join('C:', mock_base)
        mock_node.nodeid = node_id
        mock_report.stats.get.side_effect = [[], [mock_node]]  # second item is error list

        terminal_report.pytest_terminal_summary(mock_report, 0)

        mock_basename.assert_called_with(node_id)

