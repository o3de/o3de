"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


UI Apps: AutomatedTesting.GameLauncher
Launch AutomatedTesting.GameLauncher with Simple level
Test should run in both gpu and non gpu
"""

import pytest
import psutil

import ly_test_tools.environment.waiter as waiter
import editor_python_test_tools.hydra_test_utils as editor_test_utils
from ly_remote_console.remote_console_commands import RemoteConsole as RemoteConsole
from ly_remote_console.remote_console_commands import (
    send_command_and_expect_response as send_command_and_expect_response,
)


@pytest.mark.parametrize("launcher_platform", ["windows"])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["Simple"])
class TestRemoteConsoleLoadLevelWorks(object):
    @pytest.fixture
    def remote_console_instance(self, request):
        console = RemoteConsole()

        def teardown():
            if console.connected:
                console.stop()

        request.addfinalizer(teardown)

        return console

    def test_RemoteConsole_LoadLevel_Works(self, launcher, level, remote_console_instance, launcher_platform):
        expected_lines = ['Level system is loading "Simple"']

        editor_test_utils.launch_and_validate_results_launcher(launcher, level, remote_console_instance, expected_lines, null_renderer=False)
