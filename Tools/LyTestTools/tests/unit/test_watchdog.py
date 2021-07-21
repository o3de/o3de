"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit Tests for watchdog.py
"""
import unittest
import unittest.mock as mock
import pytest

import ly_test_tools.environment.watchdog as watchdog

pytestmark = pytest.mark.SUITE_smoke


def mock_bool_fn():
    return


class TestWatchdog(unittest.TestCase):

    @mock.patch('threading.Event')
    @mock.patch('threading.Thread')
    def test_Watchdog_Instantiated_CreatesEventAndThread(self, mock_thread, mock_event):
        mock_watchdog = watchdog.Watchdog(mock_bool_fn)

        mock_event.assert_called_once()
        mock_thread.assert_called_once_with(target=mock_watchdog._watchdog, name=mock_watchdog.name)


@mock.patch('threading.Event', mock.MagicMock())
@mock.patch('threading.Thread', mock.MagicMock())
class TestWatchdogMethods(unittest.TestCase):
    def setUp(self):
        self.mock_watchdog = watchdog.Watchdog(mock_bool_fn)

    @mock.patch('threading.Thread.start')
    @mock.patch('threading.Event.clear')
    def test_Start_Called_ClearsEventAndStartsThread(self, mock_clear, mock_start):
        self.mock_watchdog.start()

        mock_clear.assert_called_once()
        mock_start.assert_called_once()

    @mock.patch('threading.Thread.join', mock.MagicMock())
    @mock.patch('threading.Event.set')
    def test_Stop_Called_CallsEventSet(self, under_test):
        self.mock_watchdog.stop()

        under_test.assert_called_once()

    @mock.patch('threading.Thread.join', mock.MagicMock())
    @mock.patch('threading.Event.set', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.watchdog.logging.Logger.error')
    def test_Stop_NoCaughtFailures_NoRaiseOrError(self, mock_error_log):
        self.mock_watchdog.caught_failure = False

        try:
            self.mock_watchdog.stop()
        except watchdog.WatchdogError as e:
            self.fail(f"Unexpected WatchdogError called. Error: {e}")
        mock_error_log.assert_not_called()

    @mock.patch('threading.Thread.join', mock.MagicMock())
    @mock.patch('threading.Event.set', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.watchdog.logging.Logger.error')
    def test_Stop_CaughtFailuresAndRaisesOnCondition_RaisesWatchdogError(self, mock_error_log):
        self.mock_watchdog.caught_failure = True
        self.mock_watchdog._raise_on_condition = True

        with pytest.raises(watchdog.WatchdogError):
            self.mock_watchdog.stop()
        mock_error_log.assert_not_called()

    @mock.patch('threading.Thread.join', mock.MagicMock())
    @mock.patch('threading.Event.set', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.watchdog.logging.Logger.error')
    def test_Stop_CaughtFailuresAndNotRaisesOnCondition_LogsError(self, mock_error_log):
        self.mock_watchdog.caught_failure = True
        self.mock_watchdog._raise_on_condition = False

        try:
            self.mock_watchdog.stop()
        except watchdog.WatchdogError as e:
            self.fail(f"Unexpected WatchdogError called. Error: {e}")
        mock_error_log.assert_called_once()

    @mock.patch('threading.Thread.join')
    def test_Stop_Called_CallsJoin(self, under_test):
        self.mock_watchdog.caught_failure = False
        self.mock_watchdog.stop()

        under_test.assert_called_once()

    @mock.patch('threading.Thread.join', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.watchdog.Watchdog.is_alive')
    @mock.patch('ly_test_tools.environment.watchdog.logging.Logger.error')
    def test_Stop_ThreadIsAlive_LogsError(self, under_test, mock_is_alive):
        mock_is_alive.return_value = True
        self.mock_watchdog.stop()

        under_test.assert_called_once()

    @mock.patch('threading.Thread.join', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.watchdog.Watchdog.is_alive')
    @mock.patch('ly_test_tools.environment.watchdog.logging.Logger.error')
    def test_Stop_ThreadNotAlive_NoLogsError(self, under_test, mock_is_alive):
        mock_is_alive.return_value = False
        self.mock_watchdog.stop()

        under_test.assert_not_called()

    @mock.patch('threading.Thread.is_alive')
    def test_IsAlive_Called_CallsIsAlive(self, under_test):
        self.mock_watchdog.is_alive()

        under_test.assert_called_once()

    @mock.patch('threading.Event.wait')
    def test_WatchdogRunner_ShutdownEventNotSet_CallsBoolFn(self, mock_event_wait):
        mock_event_wait.side_effect = [False, True]
        mock_bool_fn = mock.MagicMock()
        mock_bool_fn.return_value = False
        self.mock_watchdog._bool_fn = mock_bool_fn
        self.mock_watchdog._watchdog()

        self.mock_watchdog._bool_fn.assert_called_once()
        assert self.mock_watchdog.caught_failure == False

    def test_WatchdogRunner_ShutdownEventSet_NoCallsBoolFn(self):
        self.mock_watchdog._shutdown.set()
        mock_bool_fn = mock.MagicMock()
        self.mock_watchdog._bool_fn = mock_bool_fn
        under_test = self.mock_watchdog._watchdog()

        assert under_test is None
        self.mock_watchdog._bool_fn.assert_not_called()

    @mock.patch('threading.Event.wait')
    def test_WatchdogRunner_BoolFnReturnsTrue_SetsCaughtFailureToTrue(self, mock_event_wait):
        mock_event_wait.side_effect = [False, True]
        mock_bool_fn = mock.MagicMock()
        mock_bool_fn.return_value = True
        self.mock_watchdog._bool_fn = mock_bool_fn
        self.mock_watchdog._watchdog()

        assert self.mock_watchdog.caught_failure == True


class TestProcessUnresponsiveWatchdog(unittest.TestCase):
    mock_process_id = 11111
    mock_name = 'foo.exe'
    mock_process_not_resp_call = \
        '\r\n " \
        "Image Name                     PID Session Name        Session#    Mem Usage\r\n" \
        "========================= ======== ================ =========== ============\r\n" \
        "foo.exe                      %d Console                    1          0 K\r\n"' % mock_process_id
    mock_process_resp_call = "INFO: No tasks are running which match the specified criteria.\r\n"

    @mock.patch('psutil.Process', mock.MagicMock())
    def setUp(self):
        self.mock_watchdog = watchdog.ProcessUnresponsiveWatchdog(self.mock_process_id)
        self.mock_watchdog._process_name = self.mock_name

    @mock.patch('ly_test_tools.environment.process_utils.check_output')
    def test_ProcessNotResponding_ProcessResponsive_ReturnsFalse(self, mock_check_output):
        mock_check_output.return_value = self.mock_process_resp_call
        self.mock_watchdog._pid = self.mock_process_id
        under_test = self.mock_watchdog._process_not_responding()
        
        assert not under_test
        assert self.mock_watchdog._calculated_timeout_point is None

    @mock.patch('time.time')
    @mock.patch('ly_test_tools.environment.process_utils.check_output')
    def test_ProcessNotResponding_ProcessUnresponsiveNoTimeout_ReturnsFalse(self, mock_check_output, mock_time):
        mock_time.return_value = 1
        mock_check_output.return_value = self.mock_process_not_resp_call
        self.mock_watchdog._pid = self.mock_process_id
        under_test = self.mock_watchdog._process_not_responding()
        timeout_under_test = mock_time.return_value + self.mock_watchdog._unresponsive_timeout

        assert not under_test
        assert self.mock_watchdog._calculated_timeout_point == timeout_under_test

    @mock.patch('time.time')
    @mock.patch('ly_test_tools.environment.process_utils.check_output')
    def test_ProcessNotResponding_ProcessUnresponsiveReachesTimeout_ReturnsTrue(self, mock_check_output, mock_time):
        mock_time.return_value = 3
        mock_check_output.return_value = self.mock_process_not_resp_call
        self.mock_watchdog._pid = self.mock_process_id
        self.mock_watchdog._calculated_timeout_point = 2
        under_test = self.mock_watchdog._process_not_responding()

        assert under_test

    def test_GetPid_Called_ReturnsAttribute(self):
        self.mock_watchdog._pid = self.mock_process_id

        assert self.mock_watchdog.get_pid() == self.mock_watchdog._pid


class TestCrashLogWatchdog(unittest.TestCase):

    mock_log_path = 'C:/foo'

    @mock.patch('os.path.exists')
    def test_CrashExists_Called_CallsOsPathExists(self, under_test):
        under_test.side_effect = [False, mock.DEFAULT]
        mock_watchdog = watchdog.CrashLogWatchdog(self.mock_log_path)
        mock_watchdog._bool_fn()

        under_test.assert_called_with(self.mock_log_path)

    @mock.patch('os.path.exists')
    @mock.patch('os.remove')
    def test_CrashLogWatchdog_LogsExist_ClearsExistingLogs(self, under_test, mock_exists):
        mock_exists.return_value = True
        mock_watchdog = watchdog.CrashLogWatchdog(self.mock_log_path)

        under_test.assert_called_once_with(self.mock_log_path)

    @mock.patch('os.path.exists')
    @mock.patch('os.remove')
    def test_CrashLogWatchdog_LogsNotExist_NoClearsExistingLogs(self, under_test, mock_exists):
        mock_exists.return_value = False
        mock_watchdog = watchdog.CrashLogWatchdog(self.mock_log_path)

        under_test.assert_not_called()

    @mock.patch('threading.Thread.join', mock.MagicMock())
    @mock.patch('builtins.print')
    @mock.patch('builtins.open')
    @mock.patch('os.path.exists')
    def test_CrashLogWatchdogStop_LogsExists_OpensLogAndPrints(self, mock_exists, mock_open, mock_print):
        mock_exists.side_effect = [False, True]
        mock_watchdog = watchdog.CrashLogWatchdog(self.mock_log_path)
        mock_watchdog.caught_failure = True
        mock_watchdog._raise_on_condition = False

        mock_watchdog.stop()

        mock_open.assert_called_once_with(mock_watchdog._log_path, "r")
        assert mock_print.called

    @mock.patch('threading.Thread.join', mock.MagicMock())
    @mock.patch('builtins.print')
    @mock.patch('builtins.open')
    @mock.patch('os.path.exists')
    def test_CrashLogWatchdogStop_LogsNoExists_NoOpensAndNoPrintsLog(self, mock_exists, mock_open, mock_print):
        mock_exists.return_value = False
        mock_watchdog = watchdog.CrashLogWatchdog(self.mock_log_path)
        mock_watchdog.caught_failure = False
        mock_watchdog._raise_on_condition = False

        mock_watchdog.stop()
        assert not mock_open.called
        assert not mock_print.called
