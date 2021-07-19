"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for ly_test_tools.o3de.shader_compiler
"""
import unittest.mock as mock

import pytest

import ly_test_tools._internal.managers.workspace
import ly_test_tools._internal.managers.abstract_resource_locator
import ly_test_tools.o3de.shader_compiler

pytestmark = pytest.mark.SUITE_smoke

mock_initial_path = "mock_initial_path"
mock_engine_root = "mock_engine_root"
mock_dev_path = "mock_dev_path"
mock_build_directory = 'mock_build_directory'
mock_project = 'mock_project'


@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.abspath',
            mock.MagicMock(return_value=mock_initial_path))
@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_engine_root',
            mock.MagicMock(return_value=(mock_engine_root, mock_dev_path)))
@mock.patch('ly_test_tools.o3de.asset_processor.logger.warning', mock.MagicMock())
class TestShaderCompiler(object):

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    def test_Init_MockWorkspace_MembersSetCorrectly(self, mock_workspace):
        under_test = ly_test_tools.o3de.shader_compiler.ShaderCompiler(mock_workspace)

        assert under_test._workspace == mock_workspace
        assert under_test._sc_proc is None

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    @mock.patch('subprocess.Popen')
    @mock.patch('ly_test_tools.WINDOWS', True)
    def test_Start_NoneRunning_ProcessStarted(self, mock_popen, mock_workspace):
        mock_shader_compiler_path = 'mock_shader_compiler_path'
        mock_workspace.paths.get_shader_compiler_path.return_value = mock_shader_compiler_path
        mock_popen.return_value = mock.MagicMock()
        under_test = ly_test_tools.o3de.shader_compiler.ShaderCompiler(mock_workspace)

        assert under_test._sc_proc is None

        under_test.start()

        assert under_test._sc_proc is not None
        mock_popen.assert_called_once_with(['RunAs', '/trustlevel:0x20000', mock_shader_compiler_path])

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    @mock.patch('subprocess.Popen')
    @mock.patch('ly_test_tools.o3de.shader_compiler.MAC', True)
    def test_Start_NotImplemented_ErrorRaised(self, mock_popen, mock_workspace):
        under_test = ly_test_tools.o3de.shader_compiler.ShaderCompiler(mock_workspace)

        assert under_test._sc_proc is None

        with pytest.raises(NotImplementedError):
            under_test.start()

        assert under_test._sc_proc is None
        mock_popen.assert_not_called()

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    @mock.patch('subprocess.Popen')
    @mock.patch('ly_test_tools.o3de.shader_compiler.logger.info')
    @mock.patch('ly_test_tools.o3de.shader_compiler.MAC', True)
    def test_Start_AlreadyRunning_ProcessNotChanged(self, mock_logger, mock_popen, mock_workspace):
        mock_shader_compiler_path = 'mock_shader_compiler_path'
        mock_workspace.paths.get_shader_compiler_path.return_value = mock_shader_compiler_path
        mock_popen.return_value = mock.MagicMock()
        under_test = ly_test_tools.o3de.shader_compiler.ShaderCompiler(mock_workspace)

        under_test._sc_proc = 'foo'

        under_test.start()

        assert under_test._sc_proc is not None
        mock_popen.assert_not_called()
        mock_logger.assert_called_once_with(
            'Attempted to start shader compiler at the path: {0}, ' 
            'but we already have one open!'.format(mock_shader_compiler_path))

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    @mock.patch('ly_test_tools.o3de.shader_compiler.process_utils.kill_processes_started_from')
    @mock.patch('ly_test_tools.o3de.shader_compiler.waiter.wait_for')
    def test_Stop_AlreadyRunning_ProcessStopped(self, mock_wait, mock_kill, mock_workspace):
        mock_shader_compiler_path = 'mock_shader_compiler_path'
        mock_workspace.paths.get_shader_compiler_path.return_value = mock_shader_compiler_path
        under_test = ly_test_tools.o3de.shader_compiler.ShaderCompiler(mock_workspace)

        under_test._sc_proc = 'foo'

        under_test.stop()

        assert under_test._sc_proc is None
        mock_kill.assert_called_once_with(mock_shader_compiler_path)
        mock_wait.assert_called_once()

    @mock.patch('ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager')
    @mock.patch('ly_test_tools.o3de.shader_compiler.process_utils.kill_processes_started_from')
    @mock.patch('ly_test_tools.o3de.shader_compiler.waiter.wait_for')
    @mock.patch('ly_test_tools.o3de.shader_compiler.logger.info')
    def test_Stop_NoneRunning_MessageLogged(self, mock_logger, mock_wait, mock_kill, mock_workspace):
        mock_shader_compiler_path = 'mock_shader_compiler_path'
        mock_workspace.paths.get_shader_compiler_path.return_value = mock_shader_compiler_path
        under_test = ly_test_tools.o3de.shader_compiler.ShaderCompiler(mock_workspace)

        under_test._sc_proc = None

        under_test.stop()

        assert under_test._sc_proc is None
        mock_kill.assert_not_called()
        mock_wait.assert_not_called()
        mock_logger.assert_called_once_with(
            'Attempted to stop shader compiler at the path: {0}, ' 
            'but we do not have any open!'.format(mock_shader_compiler_path))
