"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for ly_test_tools.log.log_monitor
"""

import io
import unittest.mock as mock
import pytest

import ly_test_tools.log.log_monitor
import ly_test_tools.launchers.platforms.base

pytestmark = pytest.mark.SUITE_smoke

mock_launcher = mock.MagicMock(ly_test_tools.launchers.platforms.base.Launcher)


def mock_log_monitor():
    """Returns a LogMonitor() object with all required parameters & resets attributes for each test."""
    log_monitor = ly_test_tools.log.log_monitor.LogMonitor(
        launcher=mock_launcher, log_file_path='mock_path')
    log_monitor.unexpected_lines_found = []
    log_monitor.expected_lines_not_found = []

    return log_monitor


class NotALauncher(object):
    """For simulating test failure when the wrong class is passed as a launcher parameter in LogMonitor()."""
    pass


@mock.patch('time.sleep', mock.MagicMock)
class TestLogMonitor(object):

    def test_Init_HasRequiredParams_ReturnsLogMonitorObject(self):
        assert ly_test_tools.log.log_monitor.LogMonitor(
            launcher=mock_launcher, log_file_path='some_log_file.log')

    def test_CheckExactMatch_HasMatch_ReturnsString(self):
        line = '9189.9998188 - INFO - [MainThread] - example.tests.test_system_example - Log Monitoring test 1'
        expected_line = 'Log Monitoring test 1'

        under_test = ly_test_tools.log.log_monitor.check_exact_match(line, expected_line)

        assert under_test == expected_line

    def test_CheckExactMatch_NoMatch_ReturnsNone(self):
        line = 'log line'
        expected_line = 'no match'

        under_test = ly_test_tools.log.log_monitor.check_exact_match(line, expected_line)

        assert under_test is None

    def test_CheckExactMatch_HasExactMatchWithEscapeCharacter_ReturnsString(self):
        line = '9189.9998188 - INFO - [MainThread] - Log Monitoring Test ($1/1).t text'

        under_test = ly_test_tools.log.log_monitor.check_exact_match(line, line)

        assert under_test == line

    def test_CheckExactMatch_NoMatchWithEscapeCharacter_ReturnsNone(self):
        line = r'<script\\x20type=\"text/javascript\">javascript:alert(1);</script>'
        expected_line = 'no match'

        under_test = ly_test_tools.log.log_monitor.check_exact_match(line, expected_line)

        assert under_test is None

    def test_CheckExactMatch_HasExactMatchWithEscapeCharacterAtEndOfString_ReturnsString(self):
        line = 'Testing with escape character as the last character in this (line)'
        under_test = ly_test_tools.log.log_monitor.check_exact_match(line, line)
        assert under_test == line

    def test_CheckExactMatch_HasExactMatchWithNonWordCharacterAtStartOfString_ReturnsString(self):
        line = '(Testing with non-word character as the first character in this line'
        under_test = ly_test_tools.log.log_monitor.check_exact_match(line, line)
        assert under_test == line

    def test_CheckSubstringMatch_SubstringMatchWithinWord_ReturnsNone(self):
        line = 'SubstringMatchWithinWord'
        expected_line = 'Match'
        under_test = ly_test_tools.log.log_monitor.check_exact_match(line, expected_line)
        assert under_test is None

    def test_CheckSubstringMatch_SubstringMatchBetweenWords_ReturnsMatch(self):
        line = 'Substring Match Within Word'
        expected_line = 'Match'
        under_test = ly_test_tools.log.log_monitor.check_exact_match(line, expected_line)
        assert under_test == expected_line

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_Monitor_UTF8StringsPresentAndExpected_Success(self):
        mock_file = io.StringIO('gr\xc3\xb6\xc3\x9feren pr\xc3\xbcfung \xd1\x82\xd0\xb5\xd1\x81\xd1\x82\xd1\x83\xd0\xb2\xd0\xb0\xd0\xbd\xd0\xbd\xd1\x8f\n\xc3\x80\xc3\x88\xc3\x8c\xc3\x92\xc3\x99\n\xc3\x85lpha\xc3\x9fravo\xc3\xa7harlie\n')
        mock_launcher.is_alive.side_effect = [True, True, True, False]

        with mock.patch('ly_test_tools.log.log_monitor.open', return_value=mock_file, create=True):
            mock_log_monitor().monitor_log_for_lines(['gr\xc3\xb6\xc3\x9feren pr\xc3\xbcfung \xd1\x82\xd0\xb5\xd1\x81\xd1\x82\xd1\x83\xd0\xb2\xd0\xb0\xd0\xbd\xd0\xbd\xd1\x8f',
                                                      '\xc3\x80\xc3\x88\xc3\x8c\xc3\x92\xc3\x99',
                                                      '\xc3\x85lpha\xc3\x9fravo\xc3\xa7harlie'])

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_Monitor_AllLinesFound_Success(self):
        mock_file = io.StringIO(u'a\nb\nc\n')
        mock_launcher.is_alive.side_effect = [True, True, True, False]

        with mock.patch('ly_test_tools.log.log_monitor.open', return_value=mock_file, create=True):
            mock_log_monitor().monitor_log_for_lines(['a', 'b', 'c'], ['d'])

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_Monitor_AllLinesNotFound_RaisesLogMonitorException(self):
        mock_file = io.StringIO(u'a\nb\nc\n')
        mock_launcher.is_alive.side_effect = [True, True, True, False]

        with mock.patch('ly_test_tools.log.log_monitor.open', return_value=mock_file, create=True):
            with pytest.raises(ly_test_tools.log.log_monitor.LogMonitorException):
                mock_log_monitor().monitor_log_for_lines(['a', 'b', 'c', 'd'], ['c'])

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_Monitor_SomeUnexpectedLinesFound_RaiseLogMonitorException(self):
        mock_file = io.StringIO(u'foo\nbar\n')
        mock_launcher.is_alive.side_effect = [True, True, True, False]

        with mock.patch('ly_test_tools.log.log_monitor.open', return_value=mock_file, create=True):
            with pytest.raises(ly_test_tools.log.log_monitor.LogMonitorException):
                mock_log_monitor().monitor_log_for_lines(['foo', 'bar'], ['bar'], halt_on_unexpected=True)

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_Monitor_ExpectedLinesNotFound_RaiseLogMonitorException(self):
        mock_file = io.StringIO(u'foo\nbar\n')
        mock_launcher.is_alive.side_effect = [True, True, True, False]

        with mock.patch('ly_test_tools.log.log_monitor.open', return_value=mock_file, create=True):
            with pytest.raises(ly_test_tools.log.log_monitor.LogMonitorException):
                mock_log_monitor().monitor_log_for_lines(['foo', 'not bar'], [''])

    @mock.patch('ly_test_tools.environment.waiter.wait_for', mock.MagicMock)
    @mock.patch('os.path.exists')
    def test_Monitor_NoLogPathExists_RaiseLogMonitorException(self, mock_path_exists):
        mock_path_exists.return_value = False

        with pytest.raises(ly_test_tools.log.log_monitor.LogMonitorException):
            mock_log_monitor().monitor_log_for_lines([''], [''])

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_Monitor_InvalidLauncherType_RaiseLogMonitorException(self):
        invalid_log_monitor = ly_test_tools.log.log_monitor.LogMonitor(
            launcher=NotALauncher, log_file_path='mock_path')

        with pytest.raises(ly_test_tools.log.log_monitor.LogMonitorException):
            invalid_log_monitor.monitor_log_for_lines(['foo'], [''])

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_Monitor_NoneTypeUnexpectedLines_CastsToList(self):
        mock_file = io.StringIO(u'foo\n')
        mock_launcher.is_alive.side_effect = [True, True, True, False]

        with mock.patch('ly_test_tools.log.log_monitor.open', return_value=mock_file, create=True):
            mock_log_monitor().monitor_log_for_lines(['foo'], None)

    @mock.patch('ly_test_tools.log.log_monitor.logging.Logger.warning')
    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_Monitor_NoneTypeExpectedLines_LogsWarningAndCastsToList(self, mock_log_warning):
        mock_file = io.StringIO(u'foo\n')
        mock_launcher.is_alive.side_effect = [True, True, True, False]

        with mock.patch('ly_test_tools.log.log_monitor.open', return_value=mock_file, create=True):
            mock_log_monitor().monitor_log_for_lines(None, ['bar'])
            mock_log_warning.assert_called_once()

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_Monitor_ExpectedLinesExactMatch_SucceedsOnExactMatch(self):
        mock_file = io.StringIO(u'exact match\n')
        mock_launcher.is_alive.side_effect = [True, True, True, False]

        with mock.patch('ly_test_tools.log.log_monitor.open', return_value=mock_file, create=True):
            mock_log_monitor().monitor_log_for_lines(['exact match', 'exact', 'match'], [])

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_Monitor_ExpectedLinesPartialMatch_RaisesLogMonitorException(self):
        mock_file = io.StringIO(u'exactlyy\n')
        mock_launcher.is_alive.side_effect = [True, True, True, False]

        with mock.patch('ly_test_tools.log.log_monitor.open', return_value=mock_file, create=True):
            with pytest.raises(ly_test_tools.log.log_monitor.LogMonitorException):
                mock_log_monitor().monitor_log_for_lines(['exactly'], [])

    def test_ValidateResults_Valid_ReturnsTrue(self):
        mock_lm = mock_log_monitor()
        mock_expected_lines = ['expected_foo']
        mock_unexpected_lines = ['unexpected_foo']
        mock_expected_lines_not_found = []
        mock_unexpected_lines_found = []

        under_test = mock_lm._validate_results(mock_expected_lines_not_found, mock_unexpected_lines_found,
                                               mock_expected_lines, mock_unexpected_lines)
        assert under_test

    def test_ValidateResults_ExpectedLineNotFound_RaisesException(self):
        mock_lm = mock_log_monitor()
        mock_expected_lines = ['expected_foo']
        mock_unexpected_lines = ['unexpected_foo']
        mock_expected_lines_not_found = ['expected_foo']
        mock_unexpected_lines_found = []

        with pytest.raises(ly_test_tools.log.log_monitor.LogMonitorException):
            mock_lm._validate_results(mock_expected_lines_not_found, mock_unexpected_lines_found, mock_expected_lines,
                                      mock_unexpected_lines)

    def test_ValidateResults_UnexpectedLineFound_RaisesException(self):
        mock_lm = mock_log_monitor()
        mock_expected_lines = ['expected_foo']
        mock_unexpected_lines = ['unexpected_foo']
        mock_expected_lines_not_found = []
        mock_unexpected_lines_found = ['unexpected_foo']

        with pytest.raises(ly_test_tools.log.log_monitor.LogMonitorException):
            mock_lm._validate_results(mock_expected_lines_not_found, mock_unexpected_lines_found, mock_expected_lines,
                                      mock_unexpected_lines)

    def test_ValidateResults_ExpectedNotFoundAndUnexpectedFound_RaisesException(self):
        mock_lm = mock_log_monitor()
        mock_expected_lines = ['expected_foo']
        mock_unexpected_lines = ['unexpected_foo']
        mock_expected_lines_not_found = ['expected_foo']
        mock_unexpected_lines_found = ['unexpected_foo']

        with pytest.raises(ly_test_tools.log.log_monitor.LogMonitorException):
            mock_lm._validate_results(mock_expected_lines_not_found, mock_unexpected_lines_found, mock_expected_lines,
                                      mock_unexpected_lines)
