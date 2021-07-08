"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#
# This is a pytest module to test the in-Editor Python API from EditorLevelComponentAPIBus
#
import pytest
pytest.importorskip('test_tools')
import time
import logging
import os
import shutil

from test_tools import WINDOWS_LAUNCHER
import test_tools.shared.log_monitor
import test_tools.launchers.phase
import test_tools.builtin.fixtures as fixtures

import shared.file_utils as file_utils

# Use the built-in workspace and editor fixtures.
# These will configure the requested project and run the editor.
workspace = fixtures.use_fixture(fixtures.builtin_workspace_fixture, scope='function')
editor = fixtures.use_fixture(fixtures.editor, scope='function')

logger = logging.getLogger(__name__)


@pytest.mark.parametrize("platform,configuration,project,spec", [
    pytest.param("win_x64_vs2017", "profile", "AutomatedTesting", "all", marks=pytest.mark.skipif(not WINDOWS_LAUNCHER, reason="Only supported on Windows hosts")),
])
class TestLevelComponentAutomation(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor):
        def teardown():
            editor.ensure_stopped()
            file_utils.delete_level(editor, "LevelComponentTest")

        request.addfinalizer(teardown)

    def test_Component(self, request, editor, project):
        logger.debug("Running automated test")

        request.addfinalizer(editor.ensure_stopped)

        editor.deploy()
        editor.launch(["--exec", "@engroot@/Tests/hydra/LevelComponentCommands.cfg"])

        editorlog_file = os.path.join(editor.workspace.release.paths.project_log(), 'Editor.log')

        expected_lines = [
            "Component List returned correctly",
            "Type Ids List returned correctly",
            "Type Names List returned correctly",
            "Level Component API validated"
        ]

        test_tools.shared.log_monitor.monitor_for_expected_lines(editor, editorlog_file, expected_lines)

        # Rely on the test script to quit after running
        editor.run(test_tools.launchers.phase.WaitForLauncherToQuit(editor, 10))
