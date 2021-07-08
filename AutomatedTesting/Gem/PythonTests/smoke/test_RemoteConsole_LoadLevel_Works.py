"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


UI Apps: AutomatedTesting.GameLauncher
Launch AutomatedTesting.GameLauncher with Simple level
Test should run in both gpu and non gpu
"""

import pytest
import psutil

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")
import ly_test_tools.environment.waiter as waiter
from ly_remote_console.remote_console_commands import RemoteConsole as RemoteConsole
from ly_remote_console.remote_console_commands import (
    send_command_and_expect_response as send_command_and_expect_response,
)


@pytest.mark.parametrize("launcher_platform", ["windows"])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["Simple"])
@pytest.mark.SUITE_sandbox
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

        self.launch_and_validate_results_launcher(launcher, level, remote_console_instance, expected_lines)

    def launch_and_validate_results_launcher(
        self,
        launcher,
        level,
        remote_console_instance,
        expected_lines,
        null_renderer=False,
        port_listener_timeout=120,
        log_monitor_timeout=300,
        remote_console_port=4600,
    ):
        """
        Runs the launcher with the specified level, and monitors Game.log for expected lines.
        :param launcher: Configured launcher object to run test against.
        :param level: The level to load in the launcher.
        :param remote_console_instance: Configured Remote Console object.
        :param expected_lines: Expected lines to search log for.
        :oaram null_renderer: Specifies the test does not require the renderer. Defaults to True.
        :param port_listener_timeout: Timeout for verifying successful connection to Remote Console.
        :param log_monitor_timeout: Timeout for monitoring for lines in Game.log
        :param remote_console_port: The port used to communicate with the Remote Console.
        """

        def _check_for_listening_port(port):
            """
            Checks to see if the connection to the designated port was established.
            :param port: Port to listen to.
            :return: True if port is listening.
            """
            port_listening = False
            for conn in psutil.net_connections():
                if "port={}".format(port) in str(conn):
                    port_listening = True
            return port_listening

        if null_renderer:
            launcher.args.extend(["-NullRenderer"])

        # Start the Launcher
        with launcher.start():

            # Ensure Remote Console can be reached
            waiter.wait_for(
                lambda: _check_for_listening_port(remote_console_port),
                port_listener_timeout,
                exc=AssertionError("Port {} not listening.".format(remote_console_port)),
            )
            remote_console_instance.start(timeout=30)

            # Load the specified level in the launcher
            send_command_and_expect_response(
                remote_console_instance, f"loadlevel {level}", "LEVEL_LOAD_END", timeout=30
            )

            # Monitor the console for expected lines
            for line in expected_lines:
                assert remote_console_instance.expect_log_line(
                    line, log_monitor_timeout
                ), f"Expected line not found: {line}"
