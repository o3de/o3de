"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

import pytest
import os
import sys
from ly_test_tools.o3de.editor_test import EditorTestSuite

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from Tools.LyTestTools.ly_test_tools.environment import process_utils
from Tools.LyTestTools.ly_test_tools.launchers import launcher_helper
from Tools.LyTestTools.ly_test_tools.log.log_monitor import LogMonitor

import Tools.LyTestTools.ly_test_tools.environment.waiter as waiter

@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):
    def test_Multiplayer_SimpleGameServerLauncher_ConnectsSuccessfully(self, workspace, launcher_platform):
        unexpected_lines = []
        expected_lines = ["New outgoing connection to remote address:"]
        halt_on_unexpected = False
        timeout = 180

        # Start the AutomatedTesting.ServerLauncher.exe in hosting mode, no rendering mode, and wait for it to exist
        server_launcher = launcher_helper.create_server_launcher(workspace)
        server_launcher.args.extend(['+host', '-rhi=Null'])
        server_launcher.start()
        waiter.wait_for(lambda: process_utils.process_exists(f"AutomatedTesting.ServerLauncher.exe", ignore_extensions=True))

        # Start the AutomatedTesting.GameLauncher.exe in client mode, no rendering mode, and wait for it to exist
        game_launcher = launcher_helper.create_game_launcher(workspace)
        game_launcher.args.extend(['+connect', '-rhi=Null'])
        game_launcher.start()
        waiter.wait_for(lambda: process_utils.process_exists(f"AutomatedTesting.GameLauncher.exe", ignore_extensions=True))

        # Verify that the GameLauncher.exe was able to connect to the ServerLauncher.exe by checking the logs
        game_launcher_log_file = os.path.join(game_launcher.workspace.paths.project_log(), 'Game.log')
        game_launcher_log_monitor = LogMonitor(game_launcher, game_launcher_log_file)
        game_launcher_log_monitor.monitor_log_for_lines(expected_lines, unexpected_lines, halt_on_unexpected, timeout)
