"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import pytest
import os
import unittest.mock as mock
import unittest

import ly_test_tools.o3de.editor_test_utils as editor_test_utils

pytestmark = pytest.mark.SUITE_smoke


class TestEditorTestUtils(unittest.TestCase):

    @mock.patch('ly_test_tools.environment.process_utils.kill_processes_named')
    def test_KillAllLyProcesses_IncludeAP_CallsCorrectly(self, mock_kill_processes_named):
        process_list = ['Editor', 'Profiler', 'RemoteConsole', 'o3de', 'AutomatedTesting.ServerLauncher',
                        'MaterialEditor', 'MaterialCanvas', 'AssetProcessor', 'AssetProcessorBatch', 'AssetBuilder']

        editor_test_utils.kill_all_ly_processes(include_asset_processor=True)
        mock_kill_processes_named.assert_called_once_with(process_list, ignore_extensions=True)

    @mock.patch('ly_test_tools.environment.process_utils.kill_processes_named')
    def test_KillAllLyProcesses_NotIncludeAP_CallsCorrectly(self, mock_kill_processes_named):
        ap_process_list = ['AssetProcessor', 'AssetProcessorBatch', 'AssetBuilder']

        editor_test_utils.kill_all_ly_processes(include_asset_processor=False)
        mock_kill_processes_named.assert_called_once()
        assert ap_process_list not in mock_kill_processes_named.call_args[0]

    def test_GetTestcaseModuleFilepath_NoExtension_ReturnsPYExtension(self):
        mock_module = mock.MagicMock()
        file_path = os.path.join('path', 'under_test')
        mock_module.__file__ = file_path

        assert file_path + '.py' == editor_test_utils.get_testcase_module_filepath(mock_module)

    def test_GetTestcaseModuleFilepath_PYExtension_ReturnsPYExtension(self):
        mock_module = mock.MagicMock()
        file_path = os.path.join('path', 'under_test.py')
        mock_module.__file__ = file_path

        assert file_path == editor_test_utils.get_testcase_module_filepath(mock_module)

    def test_GetModuleFilename_PythonModule_ReturnsFilename(self):
        mock_module = mock.MagicMock()
        file_path = os.path.join('path', 'under_test.py')
        mock_module.__file__ = file_path

        assert 'under_test' == editor_test_utils.get_module_filename(mock_module)

    def test_RetrieveLogPath_NormalProject_ReturnsLogPath(self):
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.project.return_value = 'mock_project_path'
        expected = os.path.join('mock_project_path', 'user', 'log_test_0')

        assert expected == editor_test_utils.retrieve_log_path(0, mock_workspace)

    @mock.patch('os.listdir')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('os.path.join', mock.MagicMock())
    @mock.patch('os.path.basename', mock.MagicMock())
    @mock.patch('os.path.isfile', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.waiter.wait_for', mock.MagicMock())
    def test_RetrieveCrashOutput_CrashLogExists_ReturnsLogInfo(self, mock_retrieve_log_path, mock_listdir):
        mock_retrieve_log_path.return_value = 'mock_path'
        mock_workspace = mock.MagicMock()
        mock_log = 'mock crash info'
        mock_listdir.return_value = ['mock_error_log.log']

        with mock.patch('builtins.open', mock.mock_open(read_data=mock_log)) as mock_file:
            assert mock_log == editor_test_utils.retrieve_crash_output(0, mock_workspace, 0)

    @mock.patch('os.listdir')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('os.path.isfile', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.waiter.wait_for', mock.MagicMock())
    def test_RetrieveCrashOutput_CrashLogNotExists_ReturnsError(self, mock_retrieve_log_path, mock_listdir):
        mock_retrieve_log_path.return_value = 'mock_log_path'
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.crash_log.return_value = 'mock_file.log'
        error_message = "No crash log available"
        mock_listdir.return_value = ['mock_file.log']

        assert error_message in editor_test_utils.retrieve_crash_output(0, mock_workspace, 0)

    @mock.patch('os.path.getmtime', mock.MagicMock())
    @mock.patch('os.rename')
    @mock.patch('time.strftime')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('os.path.exists')
    def test_CycleCrashReport_DmpExists_NamedCorrectly(self, mock_exists, mock_retrieve_log_path, mock_strftime,
                                                       mock_rename):
        mock_exists.side_effect = [False, False, True]
        mock_retrieve_log_path.return_value = 'mock_log_path'
        mock_workspace = mock.MagicMock()
        mock_strftime.return_value = 'mock_strftime'

        editor_test_utils.cycle_crash_report(0, mock_workspace)
        mock_rename.assert_called_once_with(os.path.join('mock_log_path', 'error.dmp'),
                                            os.path.join('mock_log_path', 'error_mock_strftime.dmp'))

    @mock.patch('os.path.getmtime', mock.MagicMock())
    @mock.patch('os.rename')
    @mock.patch('time.strftime')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('os.path.exists')
    def test_CycleCrashReport_LogExists_NamedCorrectly(self, mock_exists, mock_retrieve_log_path, mock_strftime,
                                                       mock_rename):
        mock_exists.side_effect = [False, True, False]
        mock_retrieve_log_path.return_value = 'mock_log_path'
        mock_workspace = mock.MagicMock()
        mock_strftime.return_value = 'mock_strftime'

        editor_test_utils.cycle_crash_report(0, mock_workspace)
        mock_rename.assert_called_once_with(os.path.join('mock_log_path', 'error.log'),
                                            os.path.join('mock_log_path', 'error_mock_strftime.log'))

    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.environment.waiter.wait_for', mock.MagicMock())
    def test_RetrieveEditorLogContent_CrashLogExists_ReturnsLogInfo(self, mock_retrieve_log_path):
        mock_retrieve_log_path.return_value = 'mock_log_path'
        mock_logname = 'mock_log.log'
        mock_workspace = mock.MagicMock()
        mock_log = 'mock log info'

        with mock.patch('builtins.open', mock.mock_open(read_data=mock_log)) as mock_file:
            assert f'[{mock_logname}]  {mock_log}' == editor_test_utils.retrieve_editor_log_content(0, mock_logname, mock_workspace)

    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.environment.waiter.wait_for', mock.MagicMock())
    def test_RetrieveEditorLogContent_CrashLogNotExists_ReturnsError(self, mock_retrieve_log_path):
        mock_retrieve_log_path.return_value = 'mock_log_path'
        mock_logname = 'mock_log.log'
        mock_workspace = mock.MagicMock()
        expected = f"-- Error reading {mock_logname}"

        assert expected in editor_test_utils.retrieve_editor_log_content(0, mock_logname, mock_workspace)

    def test_RetrieveLastRunTestIndexFromOutput_SecondTestFailed_Returns0(self):
        mock_test = mock.MagicMock()
        mock_test.__name__ = 'mock_test_name'
        mock_test_list = [mock_test]
        mock_editor_output = 'mock_test_name\n' \
                             'mock_test_name_1'

        assert 0 == editor_test_utils.retrieve_last_run_test_index_from_output(mock_test_list, mock_editor_output)

    def test_RetrieveLastRunTestIndexFromOutput_TenthTestFailed_Returns9(self):
        mock_test_list = []
        mock_editor_output = ''
        for x in range(10):
            mock_test = mock.MagicMock()
            mock_test.__name__ = f'mock_test_name_{x}'
            mock_test_list.append(mock_test)
            mock_editor_output += f'{mock_test.__name__}\n'
        mock_editor_output += 'mock_test_name_x'
        assert 9 == editor_test_utils.retrieve_last_run_test_index_from_output(mock_test_list, mock_editor_output)

    def test_RetrieveLastRunTestIndexFromOutput_FirstItemFailed_Returns0(self):
        mock_test_list = []
        mock_editor_output = ''
        for x in range(10):
            mock_test = mock.MagicMock()
            mock_test.__name__ = f'mock_test_name_{x}'
            mock_test_list.append(mock_test)

        assert 0 == editor_test_utils.retrieve_last_run_test_index_from_output(mock_test_list, mock_editor_output)

    @mock.patch('ly_test_tools.o3de.editor_test_utils._check_log_errors_warnings')
    @mock.patch('os.walk')
    def test_SaveFailedAssetJoblogs_ManyValidLogs_SavesCorrectly(self, mock_walk, mock_check_log):
        mock_workspace = mock.MagicMock()
        mock_walk.return_value = [['MockDirectory', None, ['mock_log.log']],
                                  ['MockDirectory2', None, ['mock_log2.log']]]
        mock_check_log.return_value = True

        editor_test_utils.save_failed_asset_joblogs(mock_workspace)

        assert mock_workspace.artifact_manager.save_artifact.call_count == 2

    @mock.patch('ly_test_tools.o3de.editor_test_utils._check_log_errors_warnings')
    @mock.patch('os.walk')
    def test_SaveFailedAssetJoblogs_ManyInvalidLogs_NoSaves(self, mock_walk, mock_check_log):
        mock_workspace = mock.MagicMock()
        mock_walk.return_value = [['MockDirectory', None, ['mock_log.log']],
                                  ['MockDirectory2', None, ['mock_log2.log']]]
        mock_check_log.return_value = False

        editor_test_utils.save_failed_asset_joblogs(mock_workspace)

        assert mock_workspace.artifact_manager.save_artifact.call_count == 0

    def test_CheckLogErrorWarnings_ValidLine_ReturnsTrue(self):
        mock_log = '~~1643759303647~~1~~00000000000009E0~~AssetBuilder~~S: 0 errors, 1 warnings'
        mock_log_path = mock.MagicMock()

        with mock.patch('builtins.open', mock.mock_open(read_data=mock_log)) as mock_file:
            expected = editor_test_utils._check_log_errors_warnings(mock_log_path)
        assert expected

    def test_CheckLogErrorWarnings_MultipleValidLine_ReturnsTrue(self):
        mock_log = 'foo\nfoo\n~~1643759303647~~1~~00000000000009E0~~AssetBuilder~~S: 1 errors, 1 warnings'
        mock_log_path = mock.MagicMock()

        with mock.patch('builtins.open', mock.mock_open(read_data=mock_log)) as mock_file:
            expected = editor_test_utils._check_log_errors_warnings(mock_log_path)
        assert expected

    def test_CheckLogErrorWarnings_InvalidLine_ReturnsFalse(self):
        mock_log = 'foo\n~~1643759303647~~1~~00000000000009E0~~AssetBuilder~~S: 0 errors, 0 warnings'
        mock_log_path = mock.MagicMock()

        with mock.patch('builtins.open', mock.mock_open(read_data=mock_log)) as mock_file:
            expected = editor_test_utils._check_log_errors_warnings(mock_log_path)
        assert not expected

    def test_CheckLogErrorWarnings_InvalidRegex_ReturnsTrue(self):
        mock_log = 'Invalid last line'
        mock_log_path = mock.MagicMock()

        with mock.patch('builtins.open', mock.mock_open(read_data=mock_log)) as mock_file:
            expected = editor_test_utils._check_log_errors_warnings(mock_log_path)
        assert expected
