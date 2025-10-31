"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import psutil

import ly_test_tools.environment.waiter as waiter
from ly_remote_console.remote_console_commands import send_command_and_expect_response


def run_launcher_tests(launcher, levels, remote_console_instance, null_renderer=False,
                       port_listener_timeout=120, remote_console_port=4600, launch_ap=True):
    """
    Runs the launcher with the specified level, and monitors for clean exit code.
    :param launcher: Configured launcher object to run test against.
    :param levels: List of levels to load in the launcher.
    :param remote_console_instance: Configured Remote Console object.
    :param null_renderer: Specifies the test does not require the renderer. Defaults to True.
    :param port_listener_timeout: Timeout for verifying successful connection to Remote Console.
    :param remote_console_port: The port used to communicate with the Remote Console.
    :param launch_ap: Whether or not to launch AP. Defaults to True.
    """

    def _check_for_listening_port(port):
        """
        Checks to see if the connection to the designated port was established.
        :param port: Port to listen to.
        :return: True if port is listening.
        """
        port_listening = False
        for conn in psutil.net_connections():
            if 'port={}'.format(port) in str(conn):
                port_listening = True
        return port_listening

    if null_renderer:
        launcher.args.extend(["-rhi=Null"])

    # Start the Launcher
    launcher.start(launch_ap=launch_ap)
    assert launcher.is_alive(), "Launcher failed to start"

    # Ensure Remote Console can be reached
    waiter.wait_for(
        lambda: _check_for_listening_port(remote_console_port),
        port_listener_timeout,
        exc=AssertionError("Port {} not listening.".format(remote_console_port)),
    )
    remote_console_instance.start(timeout=30)

    # Load the specified level(s) in the launcher
    for level in levels:
        send_command_and_expect_response(remote_console_instance,
                                         f"LoadLevel {level}",
                                         f"Level load complete: '{level}'", timeout=30)
    remote_console_instance.stop()

    # Wait for test script to quit the launcher. If wait_for returns exc, test was not successful
    waiter.wait_for(lambda: not launcher.is_alive(), timeout=30)

    # Verify launcher quit successfully and did not crash
    ret_code = launcher.get_returncode()
    assert ret_code == 0, "Test(s) failed. See Game.log for details"
