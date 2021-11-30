"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import unittest.mock as mock
import psutil
import pytest
import subprocess
import unittest

import ly_test_tools.environment.process_utils as process_utils
from ly_test_tools import WINDOWS

pytestmark = pytest.mark.SUITE_smoke


class TestSubprocessCheckOutputWrapper(unittest.TestCase):

    @mock.patch('subprocess.check_output')
    @mock.patch('logging.Logger.error')
    def test_CheckOutput_FailingCommand_UsesCorrectLoggingLevel(self, mock_log_err, mock_sub_output):
        mock_sub_output.side_effect = subprocess.CalledProcessError(1, 'cmd', 'output')
        cmd = ['test', 'cmd']

        with pytest.raises(subprocess.CalledProcessError):
            process_utils.check_output(cmd)

        mock_log_err.assert_called_once()

    @mock.patch('subprocess.check_output')
    @mock.patch('logging.Logger.info')
    def test_CheckOutput_SuccessfulCommand_UsesCorrectLoggingLevel(self, mock_log_info, mock_sub_output):
        mock_sub_output.return_value = 'Output returned successfully'.encode()
        cmd = ['test', 'cmd']
        expected_logger_info_calls = 2

        process_utils.check_output(cmd)
        self.assertEqual(expected_logger_info_calls, mock_log_info.call_count)

    @mock.patch('subprocess.check_output')
    def test_CheckOutput_FailingCommand_RaisesCalledProcessError(self, mock_sub_output):
        mock_sub_output.side_effect = subprocess.CalledProcessError(1, 'cmd', 'output')
        cmd = ['test', 'cmd']

        with self.assertRaises(subprocess.CalledProcessError):
            process_utils.check_output(cmd)

    @mock.patch('subprocess.check_output')
    def test_CheckOutput_CmdPassedAsString_ReturnsOutput(self, mock_sub_output):
        expected_output = 'Output returned successfully'
        mock_sub_output.return_value = expected_output.encode()
        cmd = 'test cmd'

        actual_output = process_utils.check_output(cmd)
        mock_sub_output.assert_called_once()
        self.assertEqual(expected_output, actual_output)


class TestSubprocessCheckOutputWrapperSafe(unittest.TestCase):

    @mock.patch('subprocess.check_output')
    @mock.patch('logging.Logger.warning')
    def test_SafeCheckOutput_FailingCommand_UsesCorrectLoggingLevel(self, mock_log_warn, mock_sub_output):
        mock_sub_output.side_effect = subprocess.CalledProcessError(1, 'cmd', 'output')
        cmd = ['test', 'cmd']

        process_utils.safe_check_output(cmd)
        mock_log_warn.assert_called_once()

    @mock.patch('subprocess.check_output')
    @mock.patch('logging.Logger.info')
    def test_SafeCheckOutput_SuccessfulCommand_UsesCorrectLoggingLevel(self, mock_log_info, mock_sub_output):
        mock_sub_output.return_value = 'Output returned successfully'.encode()
        cmd = ['test', 'cmd']
        expected_logger_info_calls = 2

        process_utils.check_output(cmd)
        self.assertEqual(expected_logger_info_calls, mock_log_info.call_count)

    @mock.patch('subprocess.check_output')
    def test_SafeCheckOutput_FailingCommand_ReturnsOutput(self, mock_sub_output):
        expected_output = 'Output returned successfully'
        mock_sub_output.side_effect = subprocess.CalledProcessError(1, 'cmd', expected_output)
        cmd = ['test', 'cmd']

        actual_output = process_utils.safe_check_output(cmd)
        mock_sub_output.assert_called_once()
        self.assertEqual(expected_output, actual_output)

    @mock.patch('subprocess.check_output')
    def test_SafeCheckOutput_CmdPassedAsString_ReturnsOutput(self, mock_sub_output):
        expected_output = 'Output returned successfully'
        mock_sub_output.return_value = expected_output.encode()
        cmd = 'test cmd'

        actual_output = process_utils.safe_check_output(cmd)
        mock_sub_output.assert_called_once()
        self.assertEqual(expected_output, actual_output)


class TestSubprocessCheckCallWrapper(unittest.TestCase):

    @mock.patch('subprocess.check_call')
    @mock.patch('logging.Logger.error')
    def test_CheckCall_FailingCommand_UsesCorrectLoggingLevel(self, mock_log_err, mock_sub_call):
        mock_sub_call.side_effect = subprocess.CalledProcessError(1, 'cmd', 'output')
        cmd = ['test', 'cmd']

        with pytest.raises(subprocess.CalledProcessError):
            process_utils.check_call(cmd)

        mock_log_err.assert_called_once()

    @mock.patch('subprocess.check_call')
    @mock.patch('logging.Logger.info')
    def test_CheckOutput_SuccessfulCommand_UsesCorrectLoggingLevel(self, mock_log_info, mock_sub_call):
        mock_sub_call.return_value = 0
        cmd = ['test', 'cmd']
        expected_logger_info_calls = 2

        process_utils.check_call(cmd)
        self.assertEqual(expected_logger_info_calls, mock_log_info.call_count)

    @mock.patch('subprocess.check_call')
    def test_CheckCall_FailingCommand_RaisesCalledProcessError(self, mock_sub_call):
        mock_sub_call.side_effect = subprocess.CalledProcessError(1, 'cmd', 'output')
        cmd = ['test', 'cmd']

        with self.assertRaises(subprocess.CalledProcessError):
            process_utils.check_call(cmd)
            mock_sub_call.assert_called_once()

    @mock.patch('subprocess.check_call')
    def test_CheckCall_CmdPassedAsString_ReturnsSuccess(self, mock_sub_call):
        expected_retcode = 0
        mock_sub_call.returncode = expected_retcode
        cmd = 'test cmd'

        actual_retcode = process_utils.check_call(cmd)
        mock_sub_call.assert_called_once()
        self.assertEqual(expected_retcode, actual_retcode)


class TestSubprocessCheckCallWrapperSafe(unittest.TestCase):

    @mock.patch('subprocess.check_call')
    @mock.patch('logging.Logger.warning')
    def test_SafeCheckCall_FailingCommand_UsesCorrectLoggingLevel(self, mock_log_warn, mock_sub_call):
        mock_sub_call.side_effect = subprocess.CalledProcessError(1, 'cmd', 'output')
        cmd = ['test', 'cmd']

        process_utils.safe_check_call(cmd)
        mock_log_warn.assert_called_once()

    @mock.patch('subprocess.check_call')
    @mock.patch('logging.Logger.info')
    def test_CheckOutput_SuccessfulCommand_UsesCorrectLoggingLevel(self, mock_log_info, mock_sub_call):
        mock_sub_call.return_value = 0
        cmd = ['test', 'cmd']
        expected_logger_info_calls = 2

        process_utils.safe_check_call(cmd)
        self.assertEqual(expected_logger_info_calls, mock_log_info.call_count)

    @mock.patch('subprocess.check_call')
    def test_SafeCheckCall_FailingCommand_ReturnsFailureCode(self, mock_sub_call):
        expected_retcode = 1
        mock_sub_call.side_effect = subprocess.CalledProcessError(expected_retcode, 'cmd', 'output')
        cmd = ['test', 'cmd']

        actual_retcode = process_utils.safe_check_call(cmd)
        mock_sub_call.assert_called_once()
        self.assertEqual(expected_retcode, actual_retcode)

    @mock.patch('subprocess.check_call')
    def test_SafeCheckCall_CmdPassedAsString_ReturnsSuccess(self, mock_sub_call):
        expected_retcode = 0
        mock_sub_call.returncode = expected_retcode
        cmd = 'test cmd'

        actual_retcode = process_utils.safe_check_call(cmd)
        mock_sub_call.assert_called_once()
        self.assertEqual(expected_retcode, actual_retcode)


@pytest.mark.skipif(
    not WINDOWS,
    reason="tests.unit.test_process_utils is restricted to the Windows platform.")
class TestCloseWindowsProcess(unittest.TestCase):

    @mock.patch('ly_test_tools.WINDOWS', False)
    def test_CloseWindowsProccess_NotOnWindows_Error(self):
        with pytest.raises(NotImplementedError):
            process_utils.close_windows_process(1)

    def test_CloseWindowsProccess_IdNone_Error(self):
        with pytest.raises(TypeError):
            process_utils.close_windows_process(None)

    @mock.patch('psutil.Process')
    def test_CloseWindowsProccess_ProcDNE_Error(self, mock_psutil):
        mock_proc = mock.MagicMock()
        mock_proc.is_running.return_value = False
        mock_psutil.return_value = mock_proc

        with pytest.raises(TypeError):
            process_utils.close_windows_process(None)

    @mock.patch('psutil.Process')
    @mock.patch('ctypes.windll.user32.EnumWindows')
    @mock.patch('ctypes.windll', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.waiter.wait_for', mock.MagicMock())
    def test_CloseProcess_MockedWindll_VerifyMock(self, mock_enum, mock_psutil):
        mock_proc = mock.MagicMock()
        mock_proc.is_running.return_value = True
        mock_psutil.return_value = mock_proc

        process_utils.close_windows_process(3)

        mock_enum.assert_called_once()


class TestProcessMatching(unittest.TestCase):

    @mock.patch("ly_test_tools.environment.process_utils._safe_get_processes")
    def test_ProcExists_HasExtension_Found(self, mock_get_proc):
        name = "dummy.exe"
        proc_mock = mock.MagicMock()
        proc_mock.name.return_value = name
        mock_get_proc.return_value = [proc_mock]

        result = process_utils.process_exists(name)

        self.assertTrue(result)
        proc_mock.name.assert_called()

    @mock.patch("ly_test_tools.environment.process_utils._safe_get_processes")
    def test_ProcExists_NoExtension_Ignored(self, mock_get_proc):
        name = "dummy.exe"
        proc_mock = mock.MagicMock()
        proc_mock.name.return_value = name
        mock_get_proc.return_value = [proc_mock]

        result = process_utils.process_exists("dummy")

        self.assertFalse(result)
        proc_mock.name.assert_called()

    @mock.patch("ly_test_tools.environment.process_utils._safe_get_processes")
    def test_ProcExistsIgnoreExtension_NoExtension_Found(self, mock_get_proc):
        name = "dummy.exe"
        proc_mock = mock.MagicMock()
        proc_mock.name.return_value = name
        mock_get_proc.return_value = [proc_mock]

        result = process_utils.process_exists("dummy", ignore_extensions=True)

        self.assertTrue(result)
        proc_mock.name.assert_called()

    @mock.patch('ly_test_tools.environment.process_utils._safe_kill_processes')
    @mock.patch('ly_test_tools.environment.process_utils._safe_get_processes')
    def test_KillProcNamed_ExactMatch_Killed(self, mock_get_proc, mock_kill_proc):
        name = "dummy.exe"
        proc_mock = mock.MagicMock()
        proc_mock.name.return_value = name
        mock_get_proc.return_value = [proc_mock]

        process_utils.kill_processes_named("dummy.exe", ignore_extensions=False)
        mock_kill_proc.assert_called()
        proc_mock.name.assert_called()

    @mock.patch('ly_test_tools.environment.process_utils._safe_kill_processes')
    @mock.patch('ly_test_tools.environment.process_utils._safe_get_processes')
    def test_KillProcNamed_NearMatch_Ignore(self, mock_get_proc, mock_kill_proc):
        name = "dummy.exe"
        proc_mock = mock.MagicMock()
        proc_mock.name.return_value = name
        mock_get_proc.return_value = [proc_mock]

        process_utils.kill_processes_named("dummy", ignore_extensions=False)
        mock_kill_proc.assert_not_called()
        proc_mock.name.assert_called()

    @mock.patch('ly_test_tools.environment.process_utils._safe_kill_processes')
    @mock.patch('ly_test_tools.environment.process_utils._safe_get_processes')
    def test_KillProcNamed_NearMatchIgnoreExtension_Kill(self, mock_get_proc, mock_kill_proc):
        name = "dummy.exe"
        proc_mock = mock.MagicMock()
        proc_mock.name.return_value = name
        mock_get_proc.return_value = [proc_mock]

        process_utils.kill_processes_named("dummy", ignore_extensions=True)
        mock_kill_proc.assert_called()
        proc_mock.name.assert_called()

    @mock.patch('ly_test_tools.environment.process_utils._safe_kill_processes')
    @mock.patch('ly_test_tools.environment.process_utils._safe_get_processes')
    def test_KillProcNamed_ExactMatchIgnoreExtension_Killed(self, mock_get_proc, mock_kill_proc):
        name = "dummy.exe"
        proc_mock = mock.MagicMock()
        proc_mock.name.return_value = name
        mock_get_proc.return_value = [proc_mock]

        process_utils.kill_processes_named("dummy.exe", ignore_extensions=True)
        mock_kill_proc.assert_called()
        proc_mock.name.assert_called()

    @mock.patch('ly_test_tools.environment.process_utils._safe_kill_processes', mock.MagicMock)
    @mock.patch('ly_test_tools.environment.process_utils._safe_get_processes')
    @mock.patch('os.path.exists')
    def test_KillProcFrom_MockKill_SilentSuccess(self, mock_path, mock_get_proc):
        mock_path.return_value = True
        proc_mock = mock.MagicMock()
        mock_get_proc.return_value = [proc_mock]

        process_utils.kill_processes_started_from("dummy_path")

    @mock.patch('ly_test_tools.environment.process_utils._safe_kill_process')
    @mock.patch('psutil.Process')
    def test_KillProcPid_ProcRunning_Killed(self, mock_psutil, mock_kill):
        mock_proc = mock.MagicMock()
        mock_proc.is_running.return_value = True
        mock_psutil.return_value = mock_proc

        process_utils.kill_process_with_pid(1)

        mock_kill.assert_called()

    @mock.patch('ly_test_tools.environment.process_utils._safe_kill_processes', mock.MagicMock)
    @mock.patch('psutil.Process')
    def test_KillProcPid_NoProc_SilentPass(self, mock_psutil):
        mock_proc = mock.MagicMock()
        mock_proc.is_running.return_value = False
        mock_psutil.return_value = mock_proc

        process_utils.kill_process_with_pid(1)

    @mock.patch('ly_test_tools.environment.process_utils._safe_kill_processes', mock.MagicMock)
    @mock.patch('psutil.Process')
    def test_KillProcPidRaiseOnMissing_NoProc_Raises(self, mock_psutil):
        mock_proc = mock.MagicMock()
        mock_proc.is_running.return_value = False
        mock_psutil.return_value = mock_proc

        with self.assertRaises(RuntimeError):
            process_utils.kill_process_with_pid(1, raise_on_missing=True)

    def test_SafeKillProc_HappyPath_Success(self):
        proc_mock = mock.MagicMock()
        proc_mock.is_running.return_value = False

        process_utils._safe_kill_process(proc_mock)

        proc_mock.kill.assert_called()
        proc_mock.is_running.assert_called()

    @mock.patch('ly_test_tools.environment.waiter.wait_for')
    @mock.patch('logging.Logger.warning')
    def test_SafeKillProc_KillRetriesFail_LogsFailure(self, mock_log_warn, mock_wait):
        mock_wait.side_effect = psutil.AccessDenied()
        proc_mock = mock.MagicMock()

        process_utils._safe_kill_process(proc_mock)

        proc_mock.kill.assert_called()
        mock_wait.assert_called()
        mock_log_warn.assert_called()

    @mock.patch('psutil.wait_procs')
    @mock.patch('logging.Logger.warning')
    def test_SafeKillProcList_RaisesError_NoRaiseAndLogsError(self, mock_log_warn, mock_wait_procs):
        mock_wait_procs.side_effect = psutil.PermissionError()
        proc_mock = mock.MagicMock()

        process_utils._safe_kill_processes(proc_mock)

        mock_wait_procs.assert_called()
        mock_log_warn.assert_called()

    @mock.patch('psutil.process_iter')
    @mock.patch('logging.Logger.debug')
    def test_SafeGetProc_CannotAccess_LogAndReturnNone(self, mock_log_debug, mock_psiter):
        mock_psiter.side_effect = psutil.Error()

        under_test = process_utils._safe_get_processes()

        self.assertIsNone(under_test)
        mock_log_debug.assert_called()

    @mock.patch('psutil.process_iter')
    @mock.patch('logging.Logger.debug')
    def test_SafeGetProc_HappyPathDummy_ReturnDummy(self, mock_log_debug, mock_psiter):
        dummy = object()
        mock_psiter.return_value = dummy

        under_test = process_utils._safe_get_processes()

        self.assertIs(under_test, dummy)
        mock_log_debug.assert_not_called()
