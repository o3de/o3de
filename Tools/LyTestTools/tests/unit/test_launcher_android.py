"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit Tests for android launcher-wrappers: all are sanity code-path tests, since no interprocess actions should be taken
"""

import unittest.mock as mock

import pytest

import ly_test_tools.launchers
import ly_test_tools.launchers.platforms.android.launcher
import ly_test_tools.launchers.exceptions
import ly_test_tools.mobile.android

pytestmark = pytest.mark.SUITE_smoke

VALID_ANDROID_CONFIG = {
    'android': {
        'id': '000000000000000'
    }
}
PACKAGE_NAME = "dummy_project_path"


class MockedWorkspace(object):

    def __init__(self):
        self.paths = mock.MagicMock()
        self.setup_assistant = mock.MagicMock()
        self.asset_processor = mock.MagicMock()
        self.shader_compiler = mock.MagicMock()
        self.settings = mock.MagicMock()

        self.paths.engine_root.return_value = 'engine_path'
        self.paths.build_directory.return_value = 'build_directory'
        self.paths.autoexec_file.return_value = 'autoexec.cfg'

        self.project = 'project_name'


@mock.patch('ly_test_tools.launchers.platforms.android.launcher.open', mock.mock_open())
class TestLauncherModule:

    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.json.loads')
    def test_GetPackageName_MockJsonHasKey_ReturnsValue(self, mock_json):
        dummy_name = "some_name"
        mock_json.return_value = {'android_settings': {'package_name': dummy_name}}

        under_test = ly_test_tools.launchers.platforms.android.launcher.get_package_name(PACKAGE_NAME)

        assert under_test == "some_name"

    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.json.loads')
    def test_GetPackageName_MockJsonMissingKey_SetupError(self, mock_json):
        mock_json.return_value = {"one": "two"}

        with pytest.raises(ly_test_tools.launchers.exceptions.SetupError):
            ly_test_tools.launchers.platforms.android.launcher.get_package_name(PACKAGE_NAME)

    @mock.patch('ly_test_tools.environment.process_utils.check_output')
    def test_GetPid_EarlyVersionAndExplodes_NoPid(self, mock_output):
        mock_output.side_effect = ['23', Exception]

        under_test = ly_test_tools.launchers.platforms.android.launcher.get_pid("dummy", ["dummy"])

        assert mock_output.call_count == 2
        assert not under_test

    @mock.patch('ly_test_tools.environment.process_utils.check_output')
    def test_GetPid_LaterVersionAndExplodes_NoPid(self, mock_output):
        mock_output.side_effect = ['25', Exception]

        under_test = ly_test_tools.launchers.platforms.android.launcher.get_pid("dummy", ["dummy"])

        assert mock_output.call_count == 2
        assert not under_test

    @mock.patch('ly_test_tools.environment.process_utils.check_output')
    def test_GetPid_PidHasSingleValue_ReturnPid(self, mock_output):
        mock_output.side_effect = ['25', '12']

        under_test = ly_test_tools.launchers.platforms.android.launcher.get_pid("dummy", ["dummy"])

        assert mock_output.call_count == 2
        assert under_test == '12'

    @mock.patch('ly_test_tools.environment.process_utils.check_output')
    def test_GetPid_PidHasMultipleValues_ReturnPid(self, mock_output):
        mock_output.side_effect = ['25', 'value 12 found']

        under_test = ly_test_tools.launchers.platforms.android.launcher.get_pid("dummy", ["dummy"])

        assert mock_output.call_count == 2
        assert under_test == '12'

    def test_GenerateLoadLevelCommand_HasArg_ReturnsLoadLevelCommand(self):
        args_list = ['random_arg', '+LoadLevel', 'map_name', 'another_arg']

        under_test = ly_test_tools.launchers.platforms.android.launcher.generate_android_loadlevel_command(args_list)

        assert under_test == 'LoadLevel map_name'

    def test_GenerateLoadLevelCommand_NoArg_ReturnsEmptyString(self):
        args_list = ['random_arg', 'stuff', 'map_name', 'another_arg']

        under_test = ly_test_tools.launchers.platforms.android.launcher.generate_android_loadlevel_command(args_list)

        assert under_test == ''

    def test_GenerateLoadLevelCommand_InvalidArgFormat_ReturnsEmptyString(self):
        args_list = ['random_arg', 'stuff', 'map_name', 'another_arg', '+LoadLevel']

        under_test = ly_test_tools.launchers.platforms.android.launcher.generate_android_loadlevel_command(args_list)

        assert under_test == ''


@mock.patch('os.listdir', mock.MagicMock())
@mock.patch('ly_test_tools.launchers.platforms.android.launcher.get_package_name',
            mock.MagicMock(return_value=PACKAGE_NAME))
@mock.patch('ly_test_tools.environment.file_system.create_backup', mock.MagicMock)
@mock.patch('ly_test_tools.environment.file_system.restore_backup', mock.MagicMock)
@mock.patch('ly_test_tools.mobile.android.check_adb_connection_state', mock.MagicMock)
@mock.patch('ly_test_tools.launchers.platforms.android.launcher.open', mock.mock_open())
class TestAndroidLauncher:

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    def test_ReadDeviceConfigINI_ReturnDeviceID_DeviceIDSet(self, mock_config):
        mock_config.return_value = VALID_ANDROID_CONFIG
        mock_workspace = MockedWorkspace()

        launcher = ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])

        assert launcher._device_id == VALID_ANDROID_CONFIG['android']['id']
        assert ['adb', '-s', launcher._device_id] == launcher._adb_prefix_command
        mock_config.assert_called_once()

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    def test_ReadDeviceConfigINI_ReturnsInvalidConfig_SetupError(self, mock_config):
        mock_config.return_value = {'device': {'id': 12345}}
        mock_workspace = MockedWorkspace()

        with pytest.raises(ly_test_tools.launchers.exceptions.SetupError):
            ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    @mock.patch('ly_test_tools.mobile.android.forward_tcp')
    @mock.patch('ly_test_tools.mobile.android.reverse_tcp')
    @mock.patch('ly_test_tools.mobile.android.undo_tcp_port_changes')
    def test_EnableAndroidCaps_SetsAndroidCaps_CallsReverseTCPForwardTCP(self, mock_undo_tcp, mock_reverse_tcp,
                                                                         mock_forward_tcp, mock_config):
        mock_config.return_value = VALID_ANDROID_CONFIG
        mock_workspace = MockedWorkspace()
        under_test = ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])
        under_test._enable_android_capabilities()

        assert mock_reverse_tcp.call_count == 2
        mock_forward_tcp.assert_called_once()
        mock_undo_tcp.assert_called_with(VALID_ANDROID_CONFIG['android']['id'])

    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.AndroidLauncher.backup_settings')
    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.AndroidLauncher.configure_settings')
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.AndroidLauncher._enable_android_capabilities')
    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.AndroidLauncher._is_valid_android_environment')
    @mock.patch('ly_test_tools.environment.waiter.wait_for', mock.MagicMock(return_value=True))
    def test_Setup_ValidSetup_SetupCallsSucceed(self, mock_valid_env, mock_enable_caps, mock_config,
                                                mock_configure_settings, mock_backup):
        mock_config.return_value = VALID_ANDROID_CONFIG
        mock_workspace = MockedWorkspace()
        mock_project_log_path = 'c:/mock_project/log/'
        mock_workspace.paths.project_log.return_value = mock_project_log_path

        under_test = ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])
        under_test.setup()

        mock_enable_caps.assert_called_once()
        mock_valid_env.assert_called_once()
        mock_backup.assert_called_once()
        mock_configure_settings.assert_called_once()
        mock_workspace.shader_compiler.start.assert_called_once()

    @mock.patch('ly_test_tools.mobile.android.can_run_android')
    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.AndroidLauncher.backup_settings', mock.MagicMock())
    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.AndroidLauncher.configure_settings',
                mock.MagicMock())
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.AndroidLauncher._enable_android_capabilities',
                mock.MagicMock())
    @mock.patch('ly_test_tools.environment.waiter.wait_for', mock.MagicMock(return_value=True))
    def test_Setup_InvalidSetupNoADB_RaisesNotImplementedErrorException(self, mock_config, mock_can_run_android):
        mock_config.return_value = VALID_ANDROID_CONFIG
        mock_workspace = MockedWorkspace()
        mock_can_run_android.return_value = False

        under_test = ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])

        with pytest.raises(NotImplementedError):
            under_test.setup()

    @mock.patch('ly_test_tools.mobile.android.get_devices')
    @mock.patch('ly_test_tools.mobile.android.can_run_android', mock.MagicMock(return_value=True))
    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.AndroidLauncher.backup_settings', mock.MagicMock())
    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.AndroidLauncher.configure_settings',
                mock.MagicMock())
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.AndroidLauncher._enable_android_capabilities',
                mock.MagicMock())
    @mock.patch('ly_test_tools.environment.waiter.wait_for', mock.MagicMock(return_value=True))
    def test_Setup_InvalidSetupNoDeviceConnected_RaisesSetupErrorException(self, mock_config, mock_get_devices):
        mock_config.return_value = VALID_ANDROID_CONFIG
        mock_workspace = MockedWorkspace()
        mock_get_devices.return_value = []

        under_test = ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])

        with pytest.raises(ly_test_tools.launchers.exceptions.SetupError):
            under_test.setup()

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher.restore_settings')
    @mock.patch('ly_test_tools.mobile.android.undo_tcp_port_changes', mock.MagicMock)
    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.AndroidLauncher._enable_android_capabilities',
                mock.MagicMock)
    def test_Teardown_ValidTeardown_TeardownSucceeds(self, mock_restore, mock_config):
        mock_config.return_value = VALID_ANDROID_CONFIG
        mock_workspace = MockedWorkspace()

        launcher = ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])
        launcher.teardown()

        mock_restore.assert_called_once()
        mock_workspace.shader_compiler.stop.assert_called_once()


    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    @mock.patch('ly_test_tools.environment.process_utils.check_output')
    @mock.patch('os.path.isfile', mock.MagicMock(return_value=False))
    def test_Launch_HappyPathNoAutoexec_CallsLaunchCmd(self, mock_call, mock_config):
        mock_config.return_value = VALID_ANDROID_CONFIG
        mock_workspace = MockedWorkspace()

        launcher = ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])
        launcher.launch()

        mock_call.assert_called_once_with([
            'adb', '-s', VALID_ANDROID_CONFIG['android']['id'], 'shell', 'monkey', '-p', PACKAGE_NAME,
            '-c', 'android.intent.category.LAUNCHER', '1'])

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    @mock.patch('ly_test_tools.environment.process_utils.check_output')
    @mock.patch('ly_test_tools.mobile.android.push_files_to_device', mock.MagicMock)
    @mock.patch('os.path.isfile', mock.MagicMock(return_value=True))
    def test_Launch_HappyPathHasAutoExec_PushesFilesToDevice(self, mock_push_files, mock_config):
        mock_config.return_value = VALID_ANDROID_CONFIG
        mock_workspace = MockedWorkspace()

        launcher = ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])
        launcher.launch()

        mock_push_files.assert_called_once()

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    @mock.patch('ly_test_tools.environment.process_utils.check_output')
    @mock.patch('os.path.isfile', mock.MagicMock(return_value=False))
    def test_Launch_MonkeyAbortedInLaunchResult_RaisesSetupError(self, mock_call, mock_config):
        mock_config.return_value = VALID_ANDROID_CONFIG
        mock_call.return_value = 'Monkey Aborted'
        mock_workspace = MockedWorkspace()

        launcher = ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])
        with pytest.raises(ly_test_tools.launchers.exceptions.SetupError):
            launcher.launch()

    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.get_pid')
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    def test_IsAlive_MockPidDNE_False(self, mock_config, mock_pid):
        mock_config.return_value = VALID_ANDROID_CONFIG
        mock_workspace = MockedWorkspace()

        launcher = ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])
        mock_pid.return_value = ""

        assert not launcher.is_alive()

    @mock.patch('ly_test_tools.launchers.platforms.android.launcher.get_pid')
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    def test_IsAlive_MockPidExists_True(self, mock_config, mock_pid):
        mock_config.return_value = VALID_ANDROID_CONFIG
        mock_workspace = MockedWorkspace()

        launcher = ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])
        mock_pid.return_value = "1234"

        assert launcher.is_alive()

    @mock.patch('ly_test_tools.environment.process_utils.check_call')
    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    @mock.patch('ly_test_tools.mobile.android.check_adb_connection_state',
                mock.MagicMock(return_value=ly_test_tools.mobile.android.SINGLE_DEVICE))
    def test_Kill_HappyPath_KillCommandSuccess(self, mock_config, mock_call):
        mock_config.return_value = VALID_ANDROID_CONFIG
        mock_workspace = MockedWorkspace()
        launcher = ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])

        # This is a direct call to a protected method, but the point of the test is to ensure functionality of this
        # protected method, so we will allow this exception
        launcher._kill()

        mock_call.assert_called_once_with(
            ['adb', '-s', VALID_ANDROID_CONFIG['android']['id'], 'shell', 'am', 'force-stop', PACKAGE_NAME])

    @mock.patch('ly_test_tools.launchers.platforms.base.Launcher._config_ini_to_dict')
    def test_BinaryPath_Called_RaisesNotImplementedError(self, mock_config):
        mock_config.return_value = VALID_ANDROID_CONFIG
        mock_workspace = MockedWorkspace()

        launcher = ly_test_tools.launchers.AndroidLauncher(mock_workspace, ["dummy"])
        with pytest.raises(NotImplementedError):
            launcher.binary_path()
