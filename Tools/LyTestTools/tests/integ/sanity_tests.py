"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

A sanity test for the built-in fixtures.
Launch the windows launcher attached to the currently installed instance.
"""
# Import any dependencies for the test.
import logging
import subprocess

import pytest

# Import any desired LTT modules from the package `ly_test_tools`. All LTT modules can be viewed at `Tools/LyTestTools/ly_test_tools`.
import ly_test_tools
# The `launchers.launcher_helper` module helps create Launcher objects which control the Open 3D Engine (O3DE) Editor and game clients.
import ly_test_tools.launchers.launcher_helper as launcher_helper
# The `builtin.helpers` module helps create the Workspace object, which controls the testing workspace in LTT.
import ly_test_tools.builtin.helpers as helpers
# The `environment` module contains tools that involve the system's environment such as processes or timed waiters.
import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.waiter as waiter
import ly_test_tools.o3de.asset_processor as ap

# Initialize a logger instance to hook all test logs together. The sub-logger pattern below makes it easy to track which file creates a log line.
logger = logging.getLogger(__name__)

# Note: For device testing, device ids must exist in ~/ly_test_tools/devices.ini, see README.txt for more info.


# First define the class `TestAutomatedTestingProject` to group test functions together.
# The example test contains two test functions: `test_StartGameLauncher_Sanity` and `test_StartEditor_Sanity`.
@pytest.mark.parametrize("project", ["AutomatedTesting"])
# The example test utilizes Pytest parameterization. The following sets the `project` parameter to `AutomatedTesting`
# for both test functions. Notice that the Pytest mark is defined at the class level to affect both test functions.
class TestAutomatedTestingProject(object):

    def DISABLED_StartGameLauncher_Sanity(self, project):
        """
        The `test_StartGameLauncher_Sanity` test function verifies that the O3DE game client launches successfully.
        Start the test by utilizing the `kill_processes_named` function to close any open O3DE processes that may
        interfere with the test. The Workspace object emulates the O3DE package by locating the engine and project
        directories. The Launcher object controls the O3DE game client and requires a Workspace object for
        initialization. Add the `-rhi=Null` arg to the executable call to disable GPU rendering. This allows the
        test to run on instances without a GPU. We launch the game client executable and wait for the process to exist.
        A try/finally block ensures proper test cleanup if issues occur during the test.
        """
        # Kill processes that may interfere with the test
        process_utils.kill_processes_named(names=process_utils.LY_PROCESS_KILL_LIST, ignore_extensions=True)

        try:
            # Create the Workspace object
            workspace = helpers.create_builtin_workspace(project=project)

            # Create the Launcher object and add args
            launcher = launcher_helper.create_launcher(workspace)
            launcher.args.extend(['-rhi=Null'])

            # Call the game client executable
            with launcher.start():
                # Wait for the process to exist
                waiter.wait_for(lambda: process_utils.process_exists(f"{project}.GameLauncher.exe", ignore_extensions=True))
        finally:
            # Clean up processes after the test is finished
            process_utils.kill_processes_named(names=process_utils.LY_PROCESS_KILL_LIST, ignore_extensions=True)

    def DISABLED_StartEditor_Sanity(self, project):
        """
        The `test_StartEditor_Sanity` test function is similar to the previous example with minor adjustments. A
        PyTest mark skips the test if the operating system is not Windows. We use the `create_editor` function instead
        of `create_launcher` to create an Editor type launcher instead of a game client type launcher. The additional
        `-autotest_mode` arg supresses modal dialogs from interfering with our test. We launch the Editor executable and
        wait for the process to exist.
        """
        # Kill processes that may interfere with the test
        process_utils.kill_processes_named(names=process_utils.LY_PROCESS_KILL_LIST, ignore_extensions=True)

        try:
            # Create the Workspace object
            workspace = helpers.create_builtin_workspace(project=project)

            # Create the Launcher object and add args
            editor = launcher_helper.create_editor(workspace)
            editor.args.extend(['-rhi=Null', '-autotest_mode'])

            # Call the Editor executable
            with editor.start():
                # Wait for the process to exist
                waiter.wait_for(lambda: process_utils.process_exists("Editor.exe", ignore_extensions=True))
        finally:
            # Clean up processes after the test is finished
            process_utils.kill_processes_named(names=process_utils.LY_PROCESS_KILL_LIST, ignore_extensions=True)

    def test_StartAP_Sanity(self, project):
        # Kill processes that may interfere with the test
        process_utils.kill_processes_named(names=process_utils.LY_PROCESS_KILL_LIST, ignore_extensions=True)

        try:
            # Create the Workspace object
            workspace = helpers.create_builtin_workspace(project=project)

            ap_manager = ap.AssetProcessor(workspace)

            # start
            ap_manager.create_temp_log_root()

            import os
            import time

            """
            
            def start(self, connection_timeout=30, quitonidle=False,
                      add_gem_scan_folders=None,
                      add_config_scan_folders=None,
                      accept_input=True, run_until_idle=False,
                      scan_folder_pattern=None, connect_to_ap=False):
            def gui_process(self, timeout=30, fastscan=True,
                            capture_output=False, platforms=None,
                            extra_params=None,
                            decode=True,
                            expect_failure=False, quitonidle=False,
                            accept_input=True):
            """

            command = ap_manager.build_ap_command(
                ap_path=os.path.abspath(workspace.paths.asset_processor()),
                fastscan=True,
                platforms=None,  # but this apparently adds the params in extra params
                extra_params=["--acceptInput", "--platforms", "linux", "--logDir", ap_manager._temp_log_root],
                add_gem_scan_folders=None,
                add_config_scan_folders=None,
                scan_folder_pattern=None)

            ap_exe_path = os.path.dirname(workspace.paths.asset_processor())

            ap_proc = subprocess.Popen(command, cwd=ap_exe_path, env=process_utils.get_display_env(), stdout=subprocess.PIPE)
            time.sleep(1)
            if ap_proc.poll() is not None:
                raise RuntimeError(f"AssetProcessor immediately quit with errorcode {ap_proc.returncode}")

            port = None

            def _get_port_from_log():
                nonlocal port
                if not os.path.exists(workspace.paths.ap_gui_log()):
                    logger.error("No GUI log at expected path")
                    return False

                log = ap.APLogParser(workspace.paths.ap_gui_log())
                if len(log.runs):
                    try:
                        port = log.runs[-1]["Control Port"]
                        return True
                    except Exception as ex:  # intentionally broad
                        logger.error("Failed to read port from file", exc_info=ex)
                return False

            waiter.wait_for(_get_port_from_log, timeout=10)

            import socket
            connection_socket = None

            def _attempt_connection():
                nonlocal port, connection_socket
                if ap_proc.poll() is not None:
                    raise RuntimeError(
                        f"Asset processor exited early with errorcode: {ap_proc.returncode}")

                connection_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                connection_socket.settimeout(60)
                connection_socket.connect(('127.0.0.1', port))

            waiter.wait_for(_attempt_connection, timeout=60)

            # TODO false? self.connect_listen()

            # wait for idle
            #self.send_message("waitforidle")
            connection_socket.sendall("waitforidle".encode())
            connection_socket.settimeout(300)
            result = connection_socket.recv(4096).decode()
            assert result == "idle", f"Did not get idle state from AP, message was instead: {result}"

            """
        except Exception:
            if ap_proc.poll() is not None:
                raise RuntimeError("Unexpectedly exited early")
            output = ""
            linecount = 0
            for line in iter(ap_proc.stdout.readline, ''):
                output += f"{line.rstrip()}\n"
                linecount += 1
                if linecount > 10000:
                    break
            raise RuntimeError(f"Error during AP test, with output:\n{output}")
            """

        finally:
            # Clean up processes after the test is finished
            process_utils.kill_processes_named(names=process_utils.LY_PROCESS_KILL_LIST, ignore_extensions=True)
