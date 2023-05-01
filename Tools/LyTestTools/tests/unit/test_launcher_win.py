"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit Tests for windows launcher-wrappers: all are sanity code-path tests, since no interprocess actions should be taken
"""

import os
import unittest.mock as mock

import pytest

import ly_test_tools.launchers
import ly_test_tools.launchers.exceptions

pytestmark = pytest.mark.SUITE_smoke


class TestWinLauncher(object):

    def test_BinaryPath_DummyPath_AddPathToExe(self):
        dummy_path = "dummy_workspace_path"
        dummy_project = "dummy_project"
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.build_directory.return_value = dummy_path
        mock_workspace.project = dummy_project
        launcher = ly_test_tools.launchers.WinLauncher(mock_workspace, ["some_args"])

        under_test = launcher.binary_path()
        expected = os.path.join(f'{dummy_path}',
                                f'{dummy_project}.GameLauncher.exe')

        assert expected == under_test

    @mock.patch('ly_test_tools.launchers.WinLauncher.binary_path', mock.MagicMock)
    @mock.patch('subprocess.Popen')
    def test_Launch_DummyArgs_ArgsPassedToPopen(self, mock_subprocess):
        dummy_args = ["some_args"]
        launcher = ly_test_tools.launchers.WinLauncher(mock.MagicMock(), dummy_args)

        launcher.launch()

        mock_subprocess.assert_called_once()
        name, args, kwargs = mock_subprocess.mock_calls[0]
        unpacked_args = args[0]  # args is a list inside a tuple
        assert len(dummy_args) > 0, "accidentally removed dummy_args"
        for expected_arg in dummy_args:
            assert expected_arg in unpacked_args

    @mock.patch('ly_test_tools.launchers.WinLauncher.is_alive')
    def test_Kill_MockAliveFalse_SilentSuccess(self, mock_alive):
        mock_alive.return_value = False
        mock_proc = mock.MagicMock()
        launcher = ly_test_tools.launchers.WinLauncher(mock.MagicMock(), ["dummy"])
        launcher._proc = mock_proc

        launcher.stop()

        mock_proc.kill.assert_called_once()
        mock_alive.assert_called()

    def test_IsAlive_NoProc_False(self):
        launcher = ly_test_tools.launchers.WinLauncher(mock.MagicMock(), ["dummy"])
        under_test = launcher.is_alive()
        assert under_test is False

    def test_IsAlive_MockProcNotReturned_True(self):
        mock_proc = mock.MagicMock()
        mock_proc.poll.return_value = None
        launcher = ly_test_tools.launchers.WinLauncher(mock.MagicMock(), ["dummy"])
        launcher._proc = mock_proc

        under_test = launcher.is_alive()

        assert under_test is True

    def test_IsAlive_MockProcHasReturned_False(self):
        mock_proc = mock.MagicMock()
        mock_proc.poll.return_value = 0
        launcher = ly_test_tools.launchers.WinLauncher(mock.MagicMock(), ["dummy"])
        launcher._proc = mock_proc

        under_test = launcher.is_alive()

        assert under_test is False

    def test_GetPid_HasProcess_ReturnsPid(self):
        mock_pid = 11111
        mock_launcher = ly_test_tools.launchers.WinLauncher(mock.MagicMock(), ["dummy"])
        mock_proc = mock.MagicMock()
        mock_proc.pid = mock_pid
        mock_launcher._proc = mock_proc

        assert mock_launcher.get_pid() == mock_pid

    def test_GetPid_HasNoProcess_ReturnsNone(self):
        mock_launcher = ly_test_tools.launchers.WinLauncher(mock.MagicMock(), ["dummy"])
        mock_proc = None
        mock_launcher._proc = mock_proc

        assert mock_launcher.get_pid() is None

    def test_GetReturnCode_HasProcess_CallsPoll(self):
        mock_launcher = ly_test_tools.launchers.WinLauncher(mock.MagicMock(), ["dummy"])
        mock_proc = mock.MagicMock()
        mock_launcher._proc = mock_proc

        mock_launcher.get_returncode()
        mock_proc.poll.assert_called_once()

    @mock.patch('subprocess.Popen.poll')
    def test_GetReturnCode_HasNoProcess_ReturnsNone(self, under_test):
        mock_launcher = ly_test_tools.launchers.WinLauncher(mock.MagicMock(), ["dummy"])
        mock_launcher._proc = None

        mock_launcher.get_returncode()
        under_test.assert_not_called()

    @mock.patch('ly_test_tools.launchers.WinLauncher.get_returncode')
    def test_CheckReturnCode_Called_CallsGetReturncode(self, under_test):
        mock_launcher = ly_test_tools.launchers.WinLauncher(mock.MagicMock(), ["dummy"])
        under_test.return_value = 0

        mock_launcher.check_returncode()
        under_test.assert_called()

    @mock.patch('ly_test_tools.launchers.WinLauncher.get_returncode')
    def test_CheckReturnCode_ReturnCodeIsZero_ReturnsNone(self, mock_get_returncode):
        mock_launcher = ly_test_tools.launchers.WinLauncher(mock.MagicMock(), ["dummy"])
        mock_get_returncode.return_value = 0

        under_test = mock_launcher.check_returncode()
        assert under_test is None

    @mock.patch('ly_test_tools.launchers.WinLauncher.get_returncode')
    def test_CheckReturnCode_ReturnCodeIsNonZero_RaisesError(self, mock_get_returncode):
        mock_launcher = ly_test_tools.launchers.WinLauncher(mock.MagicMock(), ["dummy"])
        mock_get_returncode.return_value = 1

        with pytest.raises(ly_test_tools.launchers.exceptions.CrashError):
            mock_launcher.check_returncode()


class TestWinEditor(object):

    def test_BinaryPath_DummyPath_AddPathToExe(self):
        dummy_path = "dummy_workspace_path"
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.build_directory.return_value = dummy_path
        launcher = ly_test_tools.launchers.WinEditor(mock_workspace, ["some_args"])

        under_test = launcher.binary_path()

        assert "Editor.exe" in under_test
        assert dummy_path in under_test, "workspace path unexpectedly missing"


class TestDedicatedWinLauncher(object):

    def test_BinaryPath_DummyPath_AddPathToExe(self):
        dummy_path = "dummy_workspace_path"
        dummy_project = "dummy_project"
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.build_directory.return_value = dummy_path
        mock_workspace.project = dummy_project
        launcher = ly_test_tools.launchers.DedicatedWinLauncher(mock_workspace, ["some_args"])

        under_test = launcher.binary_path()
        expected = os.path.join(f'{dummy_path}',
                                f'{dummy_project}.ServerLauncher.exe')

        assert under_test == expected

    def test_Build_MockWorkspace_DedicatedBuildRequested(self):
        mock_workspace = mock.MagicMock()
        launcher = ly_test_tools.launchers.DedicatedWinLauncher(mock_workspace, ["some_args"])

        launcher.workspace.build(dedicated=True)

        mock_workspace.build.assert_called_once_with(dedicated=True)


class TestWinAtomToolsLauncher(object):

    @mock.patch('ly_test_tools.launchers.platforms.win.launcher.os.path.isfile')
    def test_BinaryPath_DummyPathExeExists_AddPathToExe(self, mock_os_path_isfile):
        dummy_path = "dummy_workspace_path"
        dummy_app_file_name = 'SomeCustomLauncher'
        mock_os_path_isfile.return_value = True
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.build_directory.return_value = dummy_path
        launcher = ly_test_tools.launchers.WinAtomToolsLauncher(mock_workspace, dummy_app_file_name, ["some_args"])

        under_test = launcher.binary_path()

        assert dummy_app_file_name in under_test, f"executable named {dummy_app_file_name} not found"
        assert dummy_path in under_test, "workspace path unexpectedly missing "

    @mock.patch('ly_test_tools.launchers.platforms.win.launcher.os.path.isfile')
    def test_BinaryPath_DummyPathExeDoesNotExist_RaiseSetupError(self, mock_os_path_isfile):
        dummy_path = "dummy_workspace_path"
        dummy_app_file_name = 'SomeCustomLauncher'
        mock_os_path_isfile.return_value = False
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.build_directory.return_value = dummy_path

        under_test = ly_test_tools.launchers.WinAtomToolsLauncher(mock_workspace, dummy_app_file_name, ["some_args"])

        with pytest.raises(ly_test_tools.launchers.exceptions.SetupError):
            under_test.binary_path()
