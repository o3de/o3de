"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit Tests for linux launcher-wrappers: all are sanity code-path tests, since no interprocess actions should be taken
"""
import os
import pytest
import unittest.mock as mock

import ly_test_tools.launchers

pytestmark = pytest.mark.SUITE_smoke


class TestLinuxLauncher(object):

    def test_Construct_TestDoubles_LinuxLauncherCreated(self):
        under_test = ly_test_tools.launchers.LinuxLauncher(mock.MagicMock(), ["some_args"])
        assert isinstance(under_test, ly_test_tools.launchers.Launcher)
        assert isinstance(under_test, ly_test_tools.launchers.LinuxLauncher)

    def test_BinaryPath_DummyPath_AddPathToApp(self):
        dummy_path = "dummy_workspace_path"
        dummy_project = "dummy_project"
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.build_directory.return_value = dummy_path
        mock_workspace.project = dummy_project
        launcher = ly_test_tools.launchers.LinuxLauncher(mock_workspace, ["some_args"])

        under_test = launcher.binary_path()
        expected = os.path.join(dummy_path, f"{dummy_project}.GameLauncher")

        assert under_test == expected

    @mock.patch('ly_test_tools.launchers.LinuxLauncher.binary_path', mock.MagicMock)
    @mock.patch('subprocess.Popen')
    def test_Launch_DummyArgs_ArgsPassedToPopen(self, mock_subprocess):
        dummy_args = ["some_args"]
        launcher = ly_test_tools.launchers.LinuxLauncher(mock.MagicMock(), dummy_args)

        launcher.launch()

        mock_subprocess.assert_called_once()
        name, args, kwargs = mock_subprocess.mock_calls[0]
        unpacked_args = args[0]  # args is a list inside a tuple
        assert len(dummy_args) > 0, "accidentally removed dummy_args"
        for expected_arg in dummy_args:
            assert expected_arg in unpacked_args

    @mock.patch('ly_test_tools.launchers.LinuxLauncher.is_alive')
    def test_Kill_MockAliveFalse_SilentSuccess(self, mock_alive):
        mock_alive.return_value = False
        mock_proc = mock.MagicMock()
        launcher = ly_test_tools.launchers.LinuxLauncher(mock.MagicMock(), ["dummy"])
        launcher._proc = mock_proc

        launcher.kill()

        mock_proc.kill.assert_called_once()
        mock_alive.assert_called_once()
