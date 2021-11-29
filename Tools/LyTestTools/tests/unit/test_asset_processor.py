"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for ly_test_tools.o3de.asset_processor
"""
import datetime
import unittest.mock as mock
import os

import pytest

import ly_test_tools._internal.managers.workspace
import ly_test_tools._internal.managers.abstract_resource_locator
import ly_test_tools.o3de.asset_processor

pytestmark = pytest.mark.SUITE_smoke

mock_initial_path = "mock_initial_path"
mock_engine_root = "mock_engine_root"
mock_dev_path = "mock_dev_path"
mock_build_directory = 'mock_build_directory'
mock_project = 'mock_project'
mock_project_path = os.path.join('some', 'dir', mock_project)


@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.abspath',
            mock.MagicMock(return_value=mock_initial_path))
@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_engine_root',
            mock.MagicMock(return_value=(mock_engine_root, mock_dev_path)))
@mock.patch('ly_test_tools.o3de.asset_processor.logger.warning', mock.MagicMock())
class TestAssetProcessor(object):

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    def test_Init_DefaultParams_MembersSetCorrectly(self, mock_workspace):
        under_test = ly_test_tools.o3de.asset_processor.AssetProcessor(mock_workspace)

        assert under_test._workspace == mock_workspace
        assert under_test._port is not None
        assert under_test._ap_proc is None

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    @mock.patch('subprocess.Popen')
    @mock.patch('ly_test_tools.o3de.asset_processor.AssetProcessor.connect_socket')
    @mock.patch('ly_test_tools.o3de.asset_processor.ASSET_PROCESSOR_PLATFORM_MAP', {'foo': 'bar'})
    @mock.patch('time.sleep', mock.MagicMock())
    def test_Start_NoneRunning_ProcStarted(self, mock_connect, mock_popen, mock_workspace):
        mock_ap_path = 'mock_ap_path'
        mock_workspace.asset_processor_platform = 'foo'
        mock_workspace.paths.asset_processor.return_value = mock_ap_path
        mock_workspace.project = mock_project
        mock_workspace.paths.project.return_value = mock_project_path
        under_test = ly_test_tools.o3de.asset_processor.AssetProcessor(mock_workspace)
        under_test.enable_asset_processor_platform = mock.MagicMock()
        under_test.wait_for_idle = mock.MagicMock()
        mock_proc_object = mock.MagicMock()
        mock_proc_object.poll.return_value = None
        mock_popen.return_value = mock_proc_object

        under_test.start(connect_to_ap=True)

        assert under_test._ap_proc is not None
        mock_popen.assert_called_once()
        assert '--zeroAnalysisMode' in mock_popen.call_args[0][0]
        mock_connect.assert_called()

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    @mock.patch('subprocess.Popen')
    @mock.patch('os.path.basename', mock.MagicMock(return_value=""))
    @mock.patch('os.path.dirname', mock.MagicMock(return_value=""))
    @mock.patch('ly_test_tools.environment.process_utils.kill_processes_with_name_not_started_from',
                mock.MagicMock(return_value=None))
    @mock.patch('ly_test_tools.environment.process_utils.process_exists', mock.MagicMock(return_value=True))
    @mock.patch('socket.socket.connect')
    def test_Start_ProcAlreadyRunning_ProcNotChanged(self, mock_connect, mock_popen, mock_workspace):
        under_test = ly_test_tools.o3de.asset_processor.AssetProcessor(mock_workspace)
        under_test.process_exists = mock.MagicMock(return_value=True)
        under_test.asset_processor_platform = mock.MagicMock(return_value=ly_test_tools.HOST_OS_PLATFORM)
        mock_proc = mock.MagicMock()
        under_test._ap_proc = mock_proc

        under_test.start(connect_to_ap=True)

        assert under_test._ap_proc == mock_proc
        mock_popen.assert_not_called()
        mock_connect.assert_not_called()

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    @mock.patch('ly_test_tools.o3de.asset_processor.waiter.wait_for')
    def test_Stop_ProcAlreadyRunning_ProcStopped(self, mock_waiter, mock_workspace):
        under_test = ly_test_tools.o3de.asset_processor.AssetProcessor(mock_workspace)
        under_test.get_process_list = mock.MagicMock(return_value=[mock.MagicMock()])
        under_test._control_connection = mock.MagicMock()
        under_test.get_pid = mock.MagicMock(return_value=0)
        under_test.send_quit = mock.MagicMock(return_value=True)
        mock_proc = mock.MagicMock()
        under_test._ap_proc = mock_proc

        under_test.stop()

        under_test.send_quit.assert_called_once()
        mock_waiter.assert_called_once()
        assert under_test._ap_proc is None

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    @mock.patch('subprocess.run')
    def test_BatchProcess_NoFastscanBatchCompletes_Success(self, mock_run, mock_workspace):
        mock_workspace.project = None
        mock_workspace.paths.project.return_value = mock_project_path
        under_test = ly_test_tools.o3de.asset_processor.AssetProcessor(mock_workspace)
        apb_path = mock_workspace.paths.asset_processor_batch()
        mock_run.return_value.returncode = 0
        result, _ = under_test.batch_process(1, False)

        assert result
        mock_run.assert_called_once_with([apb_path,
                                          f'--regset="/Amazon/AzCore/Bootstrap/project_path={mock_project_path}"',
                                          '--logDir', under_test.log_root()],
                                         close_fds=True, capture_output=False,
                                         timeout=1)

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    @mock.patch('subprocess.run')
    def test_BatchProcess_FastscanBatchCompletes_Success(self, mock_run, mock_workspace):
        mock_workspace.project = mock_project
        mock_workspace.paths.project.return_value = mock_project_path
        under_test = ly_test_tools.o3de.asset_processor.AssetProcessor(mock_workspace)
        apb_path = mock_workspace.paths.asset_processor_batch()
        mock_run.return_value.returncode = 0

        result = under_test.batch_process(1, True)

        assert result
        mock_run.assert_called_once_with([apb_path, '--zeroAnalysisMode',
                                          f'--regset="/Amazon/AzCore/Bootstrap/project_path={mock_project_path}"',
                                          '--logDir', under_test.log_root()],
                                         close_fds=True, capture_output=False,
                                         timeout=1)


    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    @mock.patch('subprocess.run')
    def test_BatchProcess_ReturnCodeFail_Failure(self, mock_run, mock_workspace):
        mock_workspace.project = None
        mock_workspace.paths.project.return_value = mock_project_path
        under_test = ly_test_tools.o3de.asset_processor.AssetProcessor(mock_workspace)
        apb_path = mock_workspace.paths.asset_processor_batch()
        mock_run.return_value.returncode = 1

        result, _ = under_test.batch_process(None, False)

        assert not result
        mock_run.assert_called_once()
        assert f'--regset="/Amazon/AzCore/Bootstrap/project_path={mock_project_path}"' in mock_run.call_args[0][0]


    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    def test_EnableAssetProcessorPlatform_AssetProcessorObject_Updated(self, mock_workspace):
        under_test = ly_test_tools.o3de.asset_processor.AssetProcessor(mock_workspace)

        under_test.enable_asset_processor_platform('foo')
        assert "foo" in under_test._enabled_platform_overrides

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    def test_BackupAPSettings_Called_CallsBackupAPSettings(self, mock_workspace):
        mock_temp_path = 'foo_path'
        mock_asset_processor = ly_test_tools.o3de.asset_processor.AssetProcessor(mock_workspace)
        mock_workspace.settings.get_temp_path.return_value = mock_temp_path

        mock_asset_processor.backup_ap_settings()

        mock_workspace.settings.backup_asset_processor_settings.assert_called_with(mock_temp_path)

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    def test_RestoreAPSettings_Called_CallsRestoreAPSettings(self, mock_workspace):
        mock_temp_path = 'foo_path'
        mock_asset_processor = ly_test_tools.o3de.asset_processor.AssetProcessor(mock_workspace)
        mock_workspace.settings.get_temp_path.return_value = mock_temp_path

        mock_asset_processor.restore_ap_settings()

        mock_workspace.settings.restore_asset_processor_settings.assert_called_with(mock_temp_path)

    @mock.patch('ly_test_tools.o3de.asset_processor.AssetProcessor.restore_ap_settings')
    @mock.patch('ly_test_tools.o3de.asset_processor.AssetProcessor.stop')
    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    def test_Teardown_Called_CallsRestoreAPSettingsAndStop(self, mock_workspace, mock_stop, mock_restore_ap):
        mock_asset_processor = ly_test_tools.o3de.asset_processor.AssetProcessor(mock_workspace)

        mock_asset_processor.teardown()

        mock_stop.assert_called()
        mock_restore_ap.assert_called()
