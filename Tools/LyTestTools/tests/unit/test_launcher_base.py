"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit Tests for base launcher-wrapper: all are sanity code-path tests, since no interprocess actions should be taken
"""

import os
import unittest.mock as mock

import pytest

import ly_test_tools.launchers
import ly_test_tools.launchers.launcher_helper
import ly_test_tools._internal.managers.artifact_manager

pytestmark = pytest.mark.SUITE_smoke


class TestBaseLauncher:

    def test_Construct_TestDoubles_BaseLauncherCreated(self):
        mock_workspace = mock.MagicMock()
        mock_project_log_path = 'c:/mock_project/log/'
        mock_workspace.paths.project_log.return_value = mock_project_log_path
        under_test = ly_test_tools.launchers.Launcher(mock_workspace, ["some_args"])
        assert isinstance(under_test, ly_test_tools.launchers.Launcher)
        return under_test

    def test_Construct_StringArgs_TypeError(self):
        mock_workspace = mock.MagicMock()
        with pytest.raises(TypeError):
            ly_test_tools.launchers.Launcher(mock_workspace, "bad_args")

    def test_BinaryPath_Unimplemented_NotImplementedError(self):
        launcher = self.test_Construct_TestDoubles_BaseLauncherCreated()
        with pytest.raises(NotImplementedError):
            launcher.binary_path()

    def test_EnsureStopped_IsAliveUnimplemented_NotImplementedError(self):
        launcher = self.test_Construct_TestDoubles_BaseLauncherCreated()
        with pytest.raises(NotImplementedError):
            launcher.ensure_stopped()

    def test_IsAlive_Unimplemented_NotImplementedError(self):
        launcher = self.test_Construct_TestDoubles_BaseLauncherCreated()
        with pytest.raises(NotImplementedError):
            launcher.is_alive()

    def test_Kill_Unimplemented_NotImplementedError(self):
        launcher = self.test_Construct_TestDoubles_BaseLauncherCreated()
        with pytest.raises(NotImplementedError):
            launcher.kill()

    def test_Launch_Unimplemented_NotImplementedError(self):
        launcher = self.test_Construct_TestDoubles_BaseLauncherCreated()
        with pytest.raises(NotImplementedError):
            launcher.launch()

    @mock.patch('os.listdir', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.waiter.wait_for', mock.MagicMock(return_value=True))
    def test_Start_UnimplementedLauncher_NotImplementedError(self):
        launcher = self.test_Construct_TestDoubles_BaseLauncherCreated()
        with pytest.raises(NotImplementedError):
            launcher.start()

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.launch')
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.setup')
    def test_Start_MockImplementedLauncher_SetupLaunch(self, mock_setup, mock_launch):
        launcher = self.test_Construct_TestDoubles_BaseLauncherCreated()
        launcher.start()
        mock_setup.assert_called_once()
        mock_launch.assert_called_once()

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.launch')
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.setup')
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.stop')
    def test_WithStart_MockImplementedLauncher_SetupLaunchStop(self, mock_stop, mock_setup, mock_launch):
        launcher = self.test_Construct_TestDoubles_BaseLauncherCreated()
        with launcher.start():
            pass
        mock_stop.assert_called_once()
        mock_setup.assert_called_once()
        mock_launch.assert_called_once()

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.setup')
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.stop')
    def test_WithStart_ErrorDuringStart_StopNotCalled(self, mock_stop, mock_setup):
        mock_setup.side_effect = BufferError
        launcher = self.test_Construct_TestDoubles_BaseLauncherCreated()
        with pytest.raises(BufferError):
            with launcher.start():
                pass
        mock_stop.assert_not_called()
        mock_setup.assert_called_once()

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.launch', mock.MagicMock)
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.setup', mock.MagicMock)
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.stop')
    def test_WithStart_ErrorAfterStart_StopCalled(self, mock_stop):
        launcher = self.test_Construct_TestDoubles_BaseLauncherCreated()
        with pytest.raises(BufferError):
            with launcher.start():
                raise BufferError
        mock_stop.assert_called_once()

    def test_Stop_UnimplementedLauncher_NotImplementedError(self):
        launcher = self.test_Construct_TestDoubles_BaseLauncherCreated()
        with pytest.raises(NotImplementedError):
            launcher.stop()

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.kill')
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.ensure_stopped')
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.teardown')
    def test_Stop_MockImplementedLauncher_KillTeardown(self, mock_teardown, mock_ensure, mock_kill):
        launcher = self.test_Construct_TestDoubles_BaseLauncherCreated()
        launcher.stop()
        mock_kill.assert_called_once()
        mock_teardown.assert_called_once()
        mock_ensure.assert_called_once()

    @mock.patch('os.listdir', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.waiter.wait_for', mock.MagicMock(return_value=True))
    def test_Setup_TestDoublesNoAPOpen_StartAssetProcessor(self):
        mock_workspace = mock.MagicMock()
        mock_start_ap = mock.MagicMock()
        mock_workspace.asset_processor.start = mock_start_ap
        mock_project_log_path = 'c:/mock_project/log/'
        mock_workspace.paths.project_log.return_value = mock_project_log_path

        under_test = ly_test_tools.launchers.Launcher(mock_workspace, ["some_args"])
        under_test.setup()

        under_test.workspace.asset_processor.start.assert_called_once()

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.save_project_log_files', mock.MagicMock())
    def test_Teardown_TestDoubles_StopAppAndStopAssetProcessor(self):
        mock_workspace = mock.MagicMock()
        mock_stop_ap = mock.MagicMock()
        mock_workspace.asset_processor.stop = mock_stop_ap
        under_test = ly_test_tools.launchers.Launcher(mock_workspace, ["some_args"])

        under_test.teardown()

        mock_stop_ap.assert_called_once()

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.save_project_log_files')
    def test_Teardown_TeardownCalled_CallsSaveProjectLogFiles(self, under_test):
        mock_workspace = mock.MagicMock()
        mock_args = ['foo']
        mock_launcher = ly_test_tools.launchers.Launcher(mock_workspace, mock_args)

        mock_launcher.teardown()
        under_test.assert_called_once()

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    @mock.patch('ly_test_tools._internal.managers.artifact_manager.ArtifactManager.save_artifact')
    @mock.patch('os.listdir')
    def test_SaveProjectLogFiles_LogFilesExist_SavesOnlyLogs(self, mock_listdir, under_test):
        mock_log = 'foo.log'
        mock_txt = 'foo.txt'
        mock_args = ['foo']
        mock_project_log_path = 'c:/mock_project/log/'
        assert_path = os.path.join(mock_project_log_path, mock_log)

        mock_listdir.return_value = [mock_log, mock_txt]
        mock_workspace = mock.MagicMock()
        mock_artifact_manager = ly_test_tools._internal.managers.artifact_manager.ArtifactManager(mock.MagicMock())
        mock_workspace.artifact_manager = mock_artifact_manager
        mock_workspace.paths.project_log.return_value = mock_project_log_path

        mock_launcher = ly_test_tools.launchers.Launcher(mock_workspace, mock_args)
        mock_launcher.save_project_log_files()

        under_test.assert_called_once_with(assert_path, amount=100)

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    @mock.patch('ly_test_tools._internal.managers.artifact_manager.ArtifactManager.save_artifact')
    @mock.patch('os.listdir')
    def test_SaveProjectLogFiles_DmpFilesExist_SavesOnlyDmps(self, mock_listdir, under_test):
        mock_dmp = 'foo.dmp'
        mock_txt = 'foo.txt'
        mock_args = ['foo']
        mock_project_log_path = 'c:/mock_project/log/'
        assert_path = os.path.join(mock_project_log_path, mock_dmp)

        mock_listdir.return_value = [mock_dmp, mock_txt]
        mock_workspace = mock.MagicMock()
        mock_artifact_manager = ly_test_tools._internal.managers.artifact_manager.ArtifactManager(mock.MagicMock())
        mock_workspace.artifact_manager = mock_artifact_manager
        mock_workspace.paths.project_log.return_value = mock_project_log_path

        mock_launcher = ly_test_tools.launchers.Launcher(mock_workspace, mock_args)
        mock_launcher.save_project_log_files()

        under_test.assert_called_once_with(assert_path, amount=100)


class TestLauncherBuilder(object):
    """
    Fixture builder/helper in launchers.launcher
    """
    def test_CreateLauncher_DummyWorkspace_DefaultLauncher(self):
        dummy_workspace = mock.MagicMock()
        under_test = ly_test_tools.launchers.launcher_helper.create_launcher(
            dummy_workspace, ly_test_tools.HOST_OS_EDITOR)
        assert isinstance(under_test, ly_test_tools.launchers.Launcher)

    def test_CreateDedicateLauncher_DummyWorkspace_DefaultLauncher(self):
        dummy_workspace = mock.MagicMock()
        under_test = ly_test_tools.launchers.launcher_helper.create_dedicated_launcher(
            dummy_workspace, ly_test_tools.HOST_OS_DEDICATED_SERVER)
        assert isinstance(under_test, ly_test_tools.launchers.Launcher)

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_CreateEditor_DummyWorkspace_DefaultLauncher(self):
        dummy_workspace = mock.MagicMock()
        dummy_workspace.paths.build_directory.return_value = 'dummy'
        under_test = ly_test_tools.launchers.launcher_helper.create_editor(
            dummy_workspace, ly_test_tools.HOST_OS_GENERIC_EXECUTABLE)
        assert isinstance(under_test, ly_test_tools.launchers.Launcher)

    def test_CreateEditor_InvalidPlatform_ValidLauncherStillReturned(self):
        dummy_workspace = mock.MagicMock()
        dummy_workspace.paths.build_directory.return_value = 'dummy'
        under_test = ly_test_tools.launchers.launcher_helper.create_editor(
            dummy_workspace, 'does not exist')
        assert isinstance(under_test, ly_test_tools.launchers.Launcher)
