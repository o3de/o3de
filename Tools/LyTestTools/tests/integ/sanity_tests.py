"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

A sanity test for the built-in fixtures.
Launch the windows launcher attached to the currently installed instance.
"""
# Import any dependencies for the test.
import logging
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

# Initialize a logger instance to hook all test logs together. The sub-logger pattern below makes it easy to track which file creates a log line.
logger = logging.getLogger(__name__)

# Note: For device testing, device ids must exist in ~/ly_test_tools/devices.ini, see README.txt for more info.


# First define the class `TestAutomatedTestingProject` to group test functions together.
# The example test contains two test functions: `test_StartGameLauncher_Sanity` and `test_StartEditor_Sanity`.
@pytest.mark.parametrize("project", ["AutomatedTesting"])
# The example test utilizes Pytest parameterization. The following sets the `project` parameter to `AutomatedTesting`
# for both test functions. Notice that the Pytest mark is defined at the class level to affect both test functions.
class TestAutomatedTestingProject(object):

    def test_StartGameLauncher_Sanity(self, project):
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

    def test_StartEditor_Sanity(self, project):
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
