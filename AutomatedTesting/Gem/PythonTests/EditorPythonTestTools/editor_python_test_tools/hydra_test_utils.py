"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import os
import tempfile

import ly_test_tools.log.log_monitor
import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.waiter as waiter
from ly_remote_console.remote_console_commands import RemoteConsole as RemoteConsole
from ly_remote_console.remote_console_commands import send_command_and_expect_response as send_command_and_expect_response
logger = logging.getLogger(__name__)


def teardown_editor(editor):
    """
    :param editor: Configured editor object
    :return:
    """
    process_utils.kill_processes_named('AssetProcessor.exe')
    logger.debug('Ensuring Editor is stopped')
    editor.ensure_stopped()


def launch_and_validate_results(request, test_directory, editor, editor_script, expected_lines, unexpected_lines=[],
                                halt_on_unexpected=False, run_python="--runpythontest", auto_test_mode=True, null_renderer=True, cfg_args=[],
                                timeout=300):
    """
    Runs the Editor with the specified script, and monitors for expected log lines.
    :param request: Special fixture providing information of the requesting test function.
    :param test_directory: Path to test directory that editor_script lives in.
    :param editor: Configured editor object to run test against.
    :param editor_script: Name of script that will execute in the Editor.
    :param expected_lines: Expected lines to search log for.
    :param unexpected_lines: Unexpected lines to search log for. Defaults to none.
    :param halt_on_unexpected: Halts test if unexpected lines are found. Defaults to False.
    :param run_python: Defaults to "--runpythontest", other option is "--runpython".
    :param auto_test_mode: Determines if Editor will launch in autotest_mode, suppressing modal dialogs. Defaults to True.
    :param null_renderer: Specifies the test does not require the renderer. Defaults to True.
    :param cfg_args: Additional arguments for CFG, such as LevelName.
    :param timeout: Length of time for test to run. Default is 60.
    """
    test_case = os.path.join(test_directory, editor_script)
    request.addfinalizer(lambda: teardown_editor(editor))
    logger.debug("Running automated test: {}".format(editor_script))
    editor.args.extend(["--skipWelcomeScreenDialog", "--regset=/Amazon/Settings/EnableSourceControl=false", 
                        "--regset=/Amazon/Preferences/EnablePrefabSystem=false", run_python, test_case,
                        "--runpythonargs", " ".join(cfg_args)])
    if auto_test_mode:
        editor.args.extend(["--autotest_mode"])
    if null_renderer:
        editor.args.extend(["-rhi=Null"])

    with editor.start():

        editorlog_file = os.path.join(editor.workspace.paths.project_log(), 'Editor.log')

        # Initialize the log monitor and set time to wait for log creation
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=editor, log_file_path=editorlog_file)
        log_monitor.log_creation_max_wait_time = timeout

        # Check for expected/unexpected lines
        log_monitor.monitor_log_for_lines(expected_lines=expected_lines, unexpected_lines=unexpected_lines,
                                          halt_on_unexpected=halt_on_unexpected, timeout=timeout)


def launch_and_validate_results_launcher(launcher, level, remote_console_instance, expected_lines, null_renderer=True,
                                         port_listener_timeout=120, log_monitor_timeout=300, remote_console_port=4600):
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
            if 'port={}'.format(port) in str(conn):
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
        send_command_and_expect_response(remote_console_instance,
                                         f"map {level}",
                                         "LEVEL_LOAD_COMPLETE", timeout=30)

        # Monitor the console for expected lines
        for line in expected_lines:
            assert remote_console_instance.expect_log_line(line, log_monitor_timeout), f"Expected line not found: {line}"


def remove_files(artifact_path, suffix):
    """
    Removes files with the specified suffix from the specified path
    :param artifact_path: Path to search for files
    :param suffix: File extension to remove
    """
    if not os.path.isdir(artifact_path):
        return

    for file_name in os.listdir(artifact_path):
        if file_name.endswith(suffix):
            os.remove(os.path.join(artifact_path, file_name))
