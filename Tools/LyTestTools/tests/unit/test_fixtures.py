"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for ly_test_tools._internal.pytest_plugin.test_tools_fixtures
"""

import datetime
import os
import unittest.mock as mock

import pytest

import ly_test_tools._internal.pytest_plugin.test_tools_fixtures as test_tools_fixtures

pytestmark = pytest.mark.SUITE_smoke


class TestFixtures(object):

    def test_AddOptionForLogs_MockParser_OptionAdded(self):
        mock_parser = mock.MagicMock()

        test_tools_fixtures.pytest_addoption(mock_parser)

        mock_call = mock_parser.addoption.mock_calls[0]
        """
        mock_call is the same as: call('--output_path', help='Set the folder name for the output logs.')
        which results in a 3-tuple with (name, args, kwargs), so --output_path is in the list of args
        """
        assert mock_call[1][0] == '--output-path'

    def test_RecordSuiteProperty_MockRequestWithEmptyXml_PropertyAdded(self):
        mock_request = mock.MagicMock()
        mock_xml = mock.MagicMock()
        mock_request.config._xml = mock_xml

        func = test_tools_fixtures._record_suite_property(mock_request)
        func('NewProperty', 'value')

        mock_xml.add_global_property.assert_called_once_with('NewProperty', 'value')

    def test_RecordSuiteProperty_MockRequestWithPropertyXml_PropertyUpdated(self):
        mock_request = mock.MagicMock()
        mock_xml = mock.MagicMock()
        mock_xml.global_properties = [('ExistingProperty', 'old value')]
        mock_request.config._xml = mock_xml

        func = test_tools_fixtures._record_suite_property(mock_request)
        func('ExistingProperty', 'new value')

        assert mock_xml.global_properties[0] == ('ExistingProperty', 'new value')
        mock_xml.add_global_property.assert_not_called()

    @mock.patch('ly_test_tools._internal.pytest_plugin.test_tools_fixtures.logger')
    def test_RecordSuiteProperty_MockRequestWithoutXml_NoOpReturned(self, mock_logger):
        mock_request = mock.MagicMock()
        mock_request.config._xml = None

        func = test_tools_fixtures._record_suite_property(mock_request)
        mock_logger.debug.assert_not_called()
        func('SomeProperty', 'some value')

        mock_logger.debug.assert_called_once()

    @mock.patch('os.makedirs')
    def test_LogsPath_CustomPathOption_CustomPathCreated(self, mock_makedirs):
        expected = 'SomePath'
        mock_config = mock.MagicMock()
        mock_config.getoption.return_value = expected

        actual = test_tools_fixtures._get_output_path(mock_config)

        assert actual == expected
        mock_makedirs.assert_called_once_with('SomePath', exist_ok=True)

    @mock.patch('ly_test_tools._internal.pytest_plugin.test_tools_fixtures.datetime',
                mock.Mock(now=lambda: datetime.datetime(2019, 10, 11)))
    @mock.patch('os.getcwd')
    @mock.patch('os.makedirs')
    def test_LogsPath_NoPathOption_DefaultPathCreated(self, mock_makedirs, mock_getcwd):
        mock_config = mock.MagicMock()
        mock_cwd = 'C:/foo'
        mock_getcwd.return_value = mock_cwd
        mock_config.getoption.return_value = None

        expected = os.path.join(mock_cwd,
                                'pytest_results',
                                '2019-10-11T00-00-00-000000')
        actual = test_tools_fixtures._get_output_path(mock_config)

        assert actual == expected
        mock_makedirs.assert_called_once_with(expected, exist_ok=True)

    def test_RecordBuildName_MockRecordFunction_MockFunctionCalled(self):
        mock_fn = mock.MagicMock()

        func = test_tools_fixtures._record_build_name(mock_fn)
        func('MyBuild')

        mock_fn.assert_called_once_with('build', 'MyBuild')

    @mock.patch('ly_test_tools._internal.pytest_plugin.test_tools_fixtures.datetime',
                mock.Mock(now=lambda: datetime.datetime(2019, 10, 11)))
    def test_RecordTimeStamp_MockRecordFunction_MockFunctionCalled(self):
        mock_fn = mock.MagicMock()

        test_tools_fixtures._record_test_timestamp(mock_fn)

        mock_fn.assert_called_once_with('timestamp', '2019-10-11T00-00-00-000000')

    @mock.patch('ly_test_tools._internal.pytest_plugin.test_tools_fixtures.datetime',
                mock.Mock(now=lambda: datetime.datetime(2019, 10, 11)))
    @mock.patch('socket.gethostname')
    @mock.patch('getpass.getuser')
    def test_RecordSuiteData_MockRecordFunction_MockFunctionCalled(self, mock_getuser, mock_gethostname):
        mock_getuser.return_value = 'foo@bar.baz'
        mock_gethostname.return_value = 'bar.baz'
        mock_fn = mock.MagicMock()

        test_tools_fixtures._record_suite_data(mock_fn)

        expected_calls = [mock.call('timestamp', '2019-10-11T00-00-00-000000'),
                          mock.call('hostname', 'bar.baz'),
                          mock.call('username', 'foo@bar.baz')]

        mock_fn.assert_has_calls(expected_calls)

    @mock.patch("ly_test_tools._internal.log.py_logging_util.initialize_logging", mock.MagicMock())
    @mock.patch("ly_test_tools.builtin.helpers.setup_builtin_workspace")
    @mock.patch("ly_test_tools.builtin.helpers.create_builtin_workspace")
    def test_Workspace_MockFixturesAndExecTeardown_ReturnWorkspaceRegisterTeardown(self, mock_create, mock_setup):
        test_module = 'example.tests.test_system_example'
        test_class = ('TestSystemExample.test_SystemTestExample_AllSupportedPlatforms_LaunchAutomatedTesting'
                      '[120-simple_jacklocomotion-AutomatedTesting-all-profile-win_x64_vs2017]')
        test_method = 'test_SystemTestExample_AllSupportedPlatforms_LaunchAutomatedTesting'
        artifact_folder_name = 'TheArtifactFolder'
        artifact_path = "PathToArtifacts"

        mock_request = mock.MagicMock()
        mock_request.addfinalizer = mock.MagicMock()
        mock_request.node.module.__name__ = test_module
        mock_request.node.getmodpath.return_value = test_class
        mock_request.node.originalname = test_method
        mock_workspace = mock.MagicMock()
        mock_workspace.artifact_manager.generate_folder_name.return_value = artifact_folder_name
        mock_workspace.artifact_manager.artifact_path = artifact_path
        mock_workspace.artifact_manager.gather_artifacts.return_value = os.path.join(artifact_path, 'foo.zip')
        mock_create.return_value = mock_workspace
        mock_property = mock.MagicMock()
        mock_build_name = mock.MagicMock()
        mock_logs = "foo"

        under_test = test_tools_fixtures._workspace(
            request=mock_request,
            build_directory='foo',
            project="",
            record_property=mock_property,
            record_build_name=mock_build_name,
            output_path=mock_logs,
            asset_processor_platform='ap_platform'
        )

        assert under_test is mock_workspace
        # verify additional commands are called
        mock_create.assert_called_once()
        mock_setup.assert_called_once_with(under_test, artifact_folder_name, mock_request.session.testscollected)

        # verify teardown was hooked but not called
        mock_request.addfinalizer.assert_called_once()
        mock_workspace.teardown.assert_not_called()

        # execute teardown hook from recorded call, and verify called
        mock_request.addfinalizer.call_args[0][0]()
        mock_workspace.teardown.assert_called_once()
        mock_workspace.artifact_manager.generate_folder_name.assert_called_with(
            test_module.split('.')[-1],  # 'example.tests.test_system_example' -> 'test_system_example'
            test_class.split('.')[0],  # 'TestSystemExample.test_SystemTestExample_...' -> 'TestSystemExample'
            test_method  # 'test_SystemTestExample_AllSupportedPlatforms_LaunchAutomatedTesting'
        )

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    @mock.patch("ly_test_tools.launchers.launcher_helper.create_game_launcher")
    def test_Launcher_MockHelper_Passthrough(self, mock_create):
        retval = mock.MagicMock()
        mock_create.return_value = retval
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.waf.return_value = "dummy"
        mock_workspace.paths.autoexec_file.return_value = "dummy2"
        file_handler = mock.mock_open()

        with mock.patch('ly_test_tools._internal.pytest_plugin.test_tools_fixtures.open', file_handler, create=True):
            with open(mock_workspace.paths.autoexec_file(), 'w') as autoexec_file:
                autoexec_file.write('map ' + 'level')

        under_test = test_tools_fixtures._launcher(mock.MagicMock(), mock_workspace, 'windows', 'level')

        mock_create.assert_called_once()
        assert retval is under_test

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    @mock.patch("ly_test_tools.launchers.launcher_helper.create_game_launcher")
    def test_Launcher_MockHelper_TeardownCalled(self, mock_create):
        retval = mock.MagicMock()
        retval.stop = mock.MagicMock()
        mock_create.return_value = retval
        mock_request = mock.MagicMock()
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.waf.return_value = "dummy"
        mock_workspace.paths.autoexec_file.return_value = "dummy2"
        file_handler = mock.mock_open()

        def _fail_finalizer():
            assert False, "teardown should have been added to finalizer"

        def _capture_finalizer(func):
            nonlocal _finalizer
            _finalizer = func

        _finalizer = _fail_finalizer
        mock_request.addfinalizer = _capture_finalizer

        with mock.patch('ly_test_tools._internal.pytest_plugin.test_tools_fixtures.open', file_handler, create=True):
            with open(mock_workspace.paths.autoexec_file(), 'w') as autoexec_file:
                autoexec_file.write('map ' + 'level')

        under_test = test_tools_fixtures._launcher(mock_request, mock_workspace, 'windows', 'level')

        assert retval is under_test
        assert _finalizer is not None
        _finalizer()
        retval.stop.assert_called_once()

    @mock.patch("ly_test_tools.launchers.launcher_helper.create_server_launcher")
    def test_DedicatedLauncher_MockHelper_Passthrough(self, mock_create):
        retval = mock.MagicMock()
        mock_create.return_value = retval

        under_test = test_tools_fixtures._dedicated_launcher(mock.MagicMock(), mock.MagicMock(), 'windows')

        mock_create.assert_called_once()
        assert retval is under_test

    @mock.patch("ly_test_tools.launchers.launcher_helper.create_server_launcher")
    def test_DedicatedLauncher_MockHelper_TeardownCalled(self, mock_create):
        retval = mock.MagicMock()
        retval.stop = mock.MagicMock()
        mock_request = mock.MagicMock()
        mock_create.return_value = retval

        def _fail_finalizer():
            assert False, "teardown should have been added to finalizer"

        def _capture_finalizer(func):
            nonlocal _finalizer
            _finalizer = func

        _finalizer = _fail_finalizer
        mock_request.addfinalizer = _capture_finalizer

        under_test = test_tools_fixtures._dedicated_launcher(mock_request, mock.MagicMock(), 'windows')

        mock_create.assert_called_once()
        assert retval is under_test
        assert _finalizer is not None
        _finalizer()
        retval.stop.assert_called_once()

    @mock.patch("ly_test_tools.launchers.launcher_helper.create_editor")
    def test_Editor_MockHelper_Passthrough(self, mock_create):
        retval = mock.MagicMock()
        mock_create.return_value = retval

        under_test = test_tools_fixtures._editor(mock.MagicMock(), mock.MagicMock(), 'windows_editor')

        mock_create.assert_called_once()
        assert retval is under_test

    @mock.patch("ly_test_tools.launchers.launcher_helper.create_editor")
    def test_Editor_MockHelper_TeardownCalled(self, mock_create):
        retval = mock.MagicMock()
        retval.stop = mock.MagicMock()
        mock_request = mock.MagicMock()
        mock_create.return_value = retval

        def _fail_finalizer():
            assert False, "teardown should have been added to finalizer"

        def _capture_finalizer(func):
            nonlocal _finalizer
            _finalizer = func

        _finalizer = _fail_finalizer
        mock_request.addfinalizer = _capture_finalizer

        under_test = test_tools_fixtures._editor(mock_request, mock.MagicMock(), 'windows_editor')

        mock_create.assert_called_once()
        assert retval is under_test
        assert _finalizer is not None
        _finalizer()
        retval.stop.assert_called_once()

    @mock.patch('ly_test_tools._internal.pytest_plugin.test_tools_fixtures.get_fixture_argument')
    @mock.patch('ly_test_tools._internal.managers.ly_process_killer.detect_lumberyard_processes')
    @mock.patch('ly_test_tools._internal.managers.ly_process_killer.kill_processes', mock.MagicMock())
    def test_AutomaticProcessKiller_ProcessKillList_KillsDetectedProcesses(self, mock_detect_processes,
                                                                           mock_get_fixture_argument):
        mock_processes_list = ['foo', 'bar', 'foobar']
        mock_detected_processes = ['foo', 'bar']
        mock_detect_processes.return_value = mock_detected_processes
        mock_get_fixture_argument.return_value = mock_processes_list

        under_test = test_tools_fixtures._automatic_process_killer(mock_processes_list)

        under_test.detect_lumberyard_processes.assert_called_with(processes_list=mock_processes_list)
        under_test.kill_processes.assert_called_with(processes_list=mock_detected_processes)

    @mock.patch('ly_test_tools.environment.watchdog.CrashLogWatchdog.start', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.watchdog.CrashLogWatchdog')
    def test_CrashLogWatchdog_Instantiates_CreatesWatchdog(self, under_test):
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.crash_log.return_value = mock.MagicMock()
        mock_request = mock.MagicMock()
        mock_request.addfinalizer = mock.MagicMock()
        mock_raise_on_crash = mock.MagicMock()
        mock_watchdog = test_tools_fixtures._crash_log_watchdog(mock_request, mock_workspace, mock_raise_on_crash)

        under_test.assert_called_once_with(mock_workspace.paths.crash_log.return_value, raise_on_condition=mock_raise_on_crash)

    @mock.patch('ly_test_tools.environment.watchdog.CrashLogWatchdog.start')
    def test_CrashLogWatchdog_Instantiates_StartsThread(self, under_test):
        mock_workspace = mock.MagicMock()
        mock_path = 'C:/foo'
        mock_workspace.paths.crash_log.return_value = mock_path
        mock_request = mock.MagicMock()
        mock_request.addfinalizer = mock.MagicMock()
        mock_raise_on_crash = mock.MagicMock()
        test_tools_fixtures._crash_log_watchdog(mock_request, mock_workspace, mock_raise_on_crash)

        under_test.assert_called_once()

    @mock.patch('ly_test_tools.environment.watchdog.CrashLogWatchdog.start', mock.MagicMock())
    def test_CrashLogWatchdog_Instantiates_AddsTeardown(self):
        mock_workspace = mock.MagicMock()
        mock_path = 'C:/foo'
        mock_workspace.paths.crash_log.return_value = mock_path
        mock_request = mock.MagicMock()
        mock_request.addfinalizer = mock.MagicMock()
        mock_raise_on_crash = mock.MagicMock()
        mock_watchdog = test_tools_fixtures._crash_log_watchdog(mock_request, mock_workspace, mock_raise_on_crash)

        mock_request.addfinalizer.assert_called_once()

    @mock.patch('ly_test_tools.environment.watchdog.CrashLogWatchdog.start', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.watchdog.CrashLogWatchdog.stop')
    def test_CrashLogWatchdog_Teardown_CallsStop(self, mock_stop):
        mock_workspace = mock.MagicMock()
        mock_path = 'C:/foo'
        mock_workspace.paths.crash_log.return_value = mock_path
        mock_request = mock.MagicMock()
        mock_request.addfinalizer = mock.MagicMock()
        mock_raise_condition = mock.MagicMock()
        mock_watchdog = test_tools_fixtures._crash_log_watchdog(mock_request, mock_workspace, mock_raise_condition)

        mock_request.addfinalizer.call_args[0][0]()
        mock_stop.assert_called_once()
