"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Regression tests for the built-in fixtures.
"""
import logging
import os
import pytest

import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.file_system as file_system
import ly_test_tools.environment.waiter as waiter
from ly_test_tools import WINDOWS

pytestmark = pytest.mark.SUITE_periodic

logger = logging.getLogger(__name__)


@pytest.fixture(scope="function")
def editor_closed_checker(request):
    """
    Verifies that the Editor and AP processes have been terminated when the test ends.
    """
    test_name = request.node
    yield

    # The Editor fixture should've terminated the Editor and the AP processes
    processes = ['Editor', 'AssetProcessor']
    processes_found = []
    for process in processes:
        if process_utils.process_exists(process, True):
            processes_found.append(f"Process '{process}' should have been terminated by the fixture after the test"
                                   f" {test_name} finished.")
            process_utils.kill_processes_named(process, True)
    assert not processes_found, f"Editor fixture unexpectedly did not clean up open processes, processes still open: {processes_found}"


@pytest.fixture(scope="function")
def log_cleaner(workspace):
    """
    Removes Game and Editor logs before test execution
    """
    logs = ['Game.log', 'Editor.log']
    for log in logs:
        log_file = os.path.join(workspace.paths.project_log(), log)
        if os.path.exists(log_file):
            file_system.delete([log_file], True, False)


@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.usefixtures("log_cleaner")
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.skipif(not WINDOWS, reason="Editor currently only functions on Windows")
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestEditorFixture(object):

    def test_EditorNotClosed_FixtureStopsProcesses(self, editor_closed_checker, editor, launcher_platform):
        # Set autotest mode and disable GPU usage
        editor.args.extend(['-NullRenderer', '-autotest_mode'])
        log_file = os.path.join(editor.workspace.paths.project_log(), "Editor.log")

        editor.start()
        waiter.wait_for(lambda: os.path.exists(log_file), timeout=180)  # time out increased due to bug SPEC-3175
        assert editor.is_alive()

        # This test doesn't call editor.stop() explicitly. Instead it uses the editor_closed_checker fixture to verify
        # that the editor fixture closes the editor and AP processes.
