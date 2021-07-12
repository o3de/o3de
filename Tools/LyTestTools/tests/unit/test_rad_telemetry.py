"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit Tests for ~/ly_test_tools/report/rad_telemetry.py
"""

import unittest.mock as mock
import os
import pytest

import ly_test_tools.report.rad_telemetry
from ly_test_tools import WINDOWS

pytestmark = pytest.mark.SUITE_smoke


_RAD_DEFAULT_PORT = 4719

_CREATE_NEW_PROCESS_GROUP = 0x00000200
_DETACHED_PROCESS = 0x00000008
_WINDOWS_FLAGS = _CREATE_NEW_PROCESS_GROUP | _DETACHED_PROCESS

RAD_TOOLS_SUBPATH = os.path.join("dev", "Gems", "RADTelemetry", "Tools")


@pytest.mark.skipif(
    not WINDOWS,
    reason="tests.unit.test_rad_telemetry is restricted to the Windows platform.")
class TestRADTelemetry:

    @mock.patch('ly_test_tools.environment.process_utils.check_call')
    @mock.patch('ly_test_tools.environment.process_utils.safe_check_call')
    def test_SetFirewallRules_ShowRuleResultNotZero_CallsAddRule(self, mock_safe_call, mock_call):
        ly_test_tools.report.rad_telemetry.set_firewall_rules()

        mock_safe_call.call_args_list = [
            mock.call(['netsh', 'advfirewall', 'firewall', 'show', 'rule', 'name=RADTelemetry', 'dir=in']),
            mock.call(['netsh', 'advfirewall', 'firewall', 'show', 'rule', 'name=RADTelemetry', 'dir=out']),
        ]
        mock_call.call_args_list = [
            mock.call(
                ['netsh', 'advfirewall', 'firewall', 'add', 'rule', 'name=RADTelemetry', 'dir=in',
                 'action=allow', 'protocol=TCP', 'localport={}'.format(_RAD_DEFAULT_PORT)]),
            mock.call(
                ['netsh', 'advfirewall', 'firewall', 'add', 'rule', 'name=RADTelemetry', 'dir=out',
                 'action=allow', 'protocol=TCP', 'localport={}'.format(_RAD_DEFAULT_PORT)]),
        ]

        assert mock_call.call_count == 2
        assert mock_safe_call.call_count == 2

    @mock.patch('ly_test_tools.environment.process_utils.check_call')
    @mock.patch('ly_test_tools.environment.process_utils.safe_check_call')
    def test_SetFirewallRules_ShowRuleResultEqualsZero_AddRuleNotCalled(self, mock_safe_call, mock_call):
        mock_safe_call.return_value = 0
        ly_test_tools.report.rad_telemetry.set_firewall_rules()

        mock_call.assert_not_called()
        assert mock_safe_call.call_count == 2

    @mock.patch('subprocess.Popen')
    def test_LaunchServer_ValidDevPath_PopenSuccess(self, mock_popen):
        mock_server_path = os.path.join('dev_path', RAD_TOOLS_SUBPATH, "tm_server.exe")

        ly_test_tools.report.rad_telemetry.launch_server('dev_path')

        mock_popen.assert_called_once_with([mock_server_path], creationflags=_WINDOWS_FLAGS, close_fds=True)

    @mock.patch('ly_test_tools.environment.process_utils.kill_processes_started_from')
    def test_TerminateServer_ValidDevPath_KillsRADProcess(self, mock_kill_process):
        mock_rad_path = os.path.join('dev_path', RAD_TOOLS_SUBPATH)

        ly_test_tools.report.rad_telemetry.terminate_servers('dev_path')

        mock_kill_process.assert_called_once_with(mock_rad_path)

    @mock.patch('ly_test_tools.environment.process_utils.check_output')
    def test_TerminateServer_ValidDevPath_KillsRADProcess(self, mock_call):
        mock_get_folder_path = os.path.join('dev_path', RAD_TOOLS_SUBPATH, "tm_server.exe")
        mock_call.return_value = 'test'

        under_test = ly_test_tools.report.rad_telemetry.get_capture_path('dev_path')

        mock_call.assert_called_once_with([mock_get_folder_path])
        assert under_test == 'test'
