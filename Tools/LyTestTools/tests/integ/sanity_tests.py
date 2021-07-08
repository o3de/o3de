"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

A sanity test for the built-in fixtures.
Launch the windows launcher attached to the currently installed instance.
"""
import logging
import pytest

import ly_test_tools
import ly_test_tools.launchers.launcher_helper as launcher_helper
import ly_test_tools.builtin.helpers as helpers
import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.waiter as waiter

logger = logging.getLogger(__name__)

# Note: For device testing, device ids must exist in ~/ly_test_tools/devices.ini, see README.txt for more info.


@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomatedTestingProject(object):

    def test_StartGameLauncher_Sanity(self, project):
        # Kill processes that may interfere with the test
        process_utils.kill_processes_named(names=process_utils.LY_PROCESS_KILL_LIST, ignore_extensions=True)

        try:
            # Create the Workspace object
            workspace = helpers.create_builtin_workspace(project=project)

            # Create the Launcher object and add args
            launcher = launcher_helper.create_launcher(workspace)
            launcher.args.extend(['-NullRenderer'])

            # Call the game client executable
            with launcher.start():
                # Wait for the process to exist
                waiter.wait_for(lambda: process_utils.process_exists(f"{project}.GameLauncher.exe", ignore_extensions=True))
        finally:
            # Clean up processes after the test is finished
            process_utils.kill_processes_named(names=process_utils.LY_PROCESS_KILL_LIST, ignore_extensions=True)

    @pytest.mark.skipif(not ly_test_tools.WINDOWS, reason="Editor currently only functions on Windows")
    def test_StartEditor_Sanity(self, project):
        # Kill processes that may interfere with the test
        process_utils.kill_processes_named(names=process_utils.LY_PROCESS_KILL_LIST, ignore_extensions=True)

        try:
            # Create the Workspace object
            workspace = helpers.create_builtin_workspace(project=project)

            # Create the Launcher object and add args
            editor = launcher_helper.create_editor(workspace)
            editor.args.extend(['-NullRenderer', '-autotest_mode'])

            # Call the Editor executable
            with editor.start():
                # Wait for the process to exist
                waiter.wait_for(lambda: process_utils.process_exists("Editor", ignore_extensions=True))
        finally:
            # Clean up processes after the test is finished
            process_utils.kill_processes_named(names=process_utils.LY_PROCESS_KILL_LIST, ignore_extensions=True)
