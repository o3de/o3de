"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for ly_test_tools._internal.pytest_plugin.failed_test_rerun_command
"""

import os
import pytest
import unittest.mock as mock

import ly_test_tools._internal.pytest_plugin.failed_test_rerun_command as failed_test_rerun_command

pytestmark = pytest.mark.SUITE_smoke


class TestRerunCommand(object):
    MOCK_FOO_EXE = os.path.join('foo', 'path')

    @mock.patch('os.path.join')
    @mock.patch('sys.argv', mock.MagicMock())
    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_GetLauncherCommand_PythonScriptFound_CmdReturned(self, mock_join):
        mock_python = 'python'
        mock_join.return_value = mock_python
        expected = f"{mock_python} -m pytest "

        under_test = failed_test_rerun_command._get_test_launcher_cmd()

        assert under_test == expected

    @mock.patch('sys.argv', mock.MagicMock())
    @mock.patch('os.path.abspath', mock.MagicMock())
    @mock.patch('os.path.join', mock.MagicMock())
    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.sys.executable', MOCK_FOO_EXE)
    @mock.patch('os.path.exists', mock.MagicMock(return_value=False))
    def test_GetLauncherCommand_PythonScriptNotFound_ExeReturned(self):
        expected = f"{TestRerunCommand.MOCK_FOO_EXE} -m pytest "

        under_test = failed_test_rerun_command._get_test_launcher_cmd()

        assert under_test == expected

    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.sys.executable', MOCK_FOO_EXE)
    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.WINDOWS', True)
    @mock.patch('sys.argv', mock.MagicMock())
    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_GetLauncherCommand_WindowsPythonInterpreter_WindowsPythonEntrypointReturned(self):
        python_script = 'python.cmd'
        expected = f"{os.path.join(TestRerunCommand.MOCK_FOO_EXE, python_script)} -m pytest "

        under_test = failed_test_rerun_command._get_test_launcher_cmd()
        assert under_test == expected

    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.sys.executable', MOCK_FOO_EXE)
    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.WINDOWS', False)
    @mock.patch('sys.argv', mock.MagicMock())
    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_GetLauncherCommand_NonWindowsPythonInterpreter_NonWindowsPythonEntrypointReturned(self):
        python_script = 'python.sh'
        expected = f"{os.path.join(TestRerunCommand.MOCK_FOO_EXE, python_script)} -m pytest "

        under_test = failed_test_rerun_command._get_test_launcher_cmd()

        assert under_test == expected

    @mock.patch('sys.argv', ['mock_python_exe', '-mock_arg1', '--mock_arg2', 'mock_arg3', 'mock_python_module'])
    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.sys.executable', MOCK_FOO_EXE)
    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.WINDOWS', True)
    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_GetLauncherCommand_WindowsPythonInterpreter_SysArgsAppended(self):
        under_test = failed_test_rerun_command._get_test_launcher_cmd()

        assert 'mock_python_exe' not in under_test
        assert 'mock_python_module' not in under_test
        assert '-mock_arg1' in under_test
        assert '--mock_arg2' in under_test
        assert 'mock_arg3' in under_test

    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.os.path.abspath')
    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.os.path.dirname')
    def test_FormatCommand_FileAsPathAndFullNodeId_CorrectCommand(self, mock_dirname, mock_abspath):
        launcher_cmd = 'python -m pytest '
        test_path = os.path.join('dirA', 'dirB', 'test_stuff.py')
        nodeid = 'test_stuff.py::test_Stuff_Something_Else[a]'
        mock_dirname.return_value = test_path
        mock_abspath.return_value = os.path.join(test_path, nodeid)

        expected = f'{launcher_cmd}{os.path.join(test_path, nodeid)}'
        actual = failed_test_rerun_command._format_cmd(launcher_cmd, test_path, nodeid)

        assert actual == expected

    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.os.path.abspath')
    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.os.path.dirname')
    def test_FormatCommand_FileAsPathAndFileAsNodeId_CorrectCommand(self, mock_dirname, mock_abspath):
        launcher_cmd = 'python -m pytest '
        test_path = os.path.join('dirA', 'dirB', 'test_stuff.py')
        nodeid = 'test_stuff.py'
        mock_dirname.return_value = test_path
        mock_abspath.return_value = os.path.join(test_path, nodeid)

        expected = f'{launcher_cmd}{test_path}'
        actual = failed_test_rerun_command._format_cmd(launcher_cmd, test_path, nodeid)

        assert actual == expected

    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.os.path.abspath')
    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.os.path.dirname')
    def test_FormatCommand_DirectoryAsPathAndFullNodeId_CorrectCommand(self, mock_dirname, mock_abspath):
        launcher_cmd = 'python -m pytest '
        test_path = os.path.join('dirA', 'dirB')
        nodeid = 'test_Stuff_Something_Else[a]'
        mock_dirname.return_value = test_path
        mock_abspath.return_value = os.path.join(test_path, os.path.normpath(nodeid))

        expected = f'{launcher_cmd}{os.path.join(test_path, nodeid)}'
        actual = failed_test_rerun_command._format_cmd(launcher_cmd, test_path, nodeid)

        assert actual == expected

    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.os.path.abspath')
    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.os.path.dirname')
    def test_FormatCommand_DirectoryAsPathAndFileAsNodeId_CorrectCommand(self, mock_dirname, mock_abspath):
        launcher_cmd = 'python -m pytest '
        test_path = os.path.join('dirA', 'dirB')
        nodeid = 'test_stuff.py'
        mock_dirname.return_value = test_path
        mock_abspath.return_value = os.path.join(test_path, os.path.normpath(nodeid))

        expected = f'{launcher_cmd}{os.path.join(test_path, nodeid)}'
        actual = failed_test_rerun_command._format_cmd(launcher_cmd, test_path, nodeid)

        assert actual == expected

    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command._get_test_launcher_cmd')
    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.os.path.abspath')
    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.os.path.dirname')
    @mock.patch('os.path.exists', mock.MagicMock(return_value=False))
    @mock.patch('ly_test_tools._internal.pytest_plugin.failed_test_rerun_command.os.environ', {})
    def test_BuildCommands_TwoNodeIds_CorrectCommands(self, mock_dirname, mock_abspath, mock_get_test_launcher_cmd):
        test_path = os.path.join('dirA', 'dirB')
        nodeids = ['test_stuff.py', 'test_something::test_Something_Somewhere_Somehow[a]']
        python_cmd = 'python -m pytest '
        mock_get_test_launcher_cmd.return_value = python_cmd
        mock_dirname.side_effect = [test_path, test_path]
        mock_abspath.side_effect = [os.path.join(test_path, os.path.normpath(nodeids[0])),
                                    os.path.join(test_path, os.path.normpath(nodeids[1]))]

        expected = [f'{python_cmd}{os.path.join(test_path, nodeids[0])}',
                    f'{python_cmd}{os.path.join(test_path, nodeids[1])}']
        actual = failed_test_rerun_command.build_rerun_commands(test_path, nodeids)

        assert actual == expected
