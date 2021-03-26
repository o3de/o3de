"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

#
# This is a pytest module to test the in-Editor Python API from PythonEditorFuncs
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

# Use the built-in workspace and editor fixtures.
# These will configure the requested project and run the editor.
workspace = fixtures.use_fixture(fixtures.builtin_workspace_fixture, scope='function')
editor = fixtures.use_fixture(fixtures.editor, scope='function')

logger = logging.getLogger(__name__)


@pytest.mark.parametrize("platform,configuration,project,spec", [
    pytest.param("win_x64_vs2017", "profile", "AutomatedTesting", "all", marks=pytest.mark.skipif(not WINDOWS_LAUNCHER, reason="Only supported on Windows hosts")),
])
class TestLightningArcPropertyRanges(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor):
        def teardown():
            editor.ensure_stopped()

            file_utils.delete_level(editor, "LightningArcTestLevel")

        request.addfinalizer(teardown)

    def test_change_properties(self, request, editor, project):
        logger.debug("Running automated test")

        request.addfinalizer(editor.ensure_stopped)

        editor.deploy()
        editor.launch(["--exec", "@engroot@/Tests/graphics/ly107748_LightningArcProperties.cfg"])

        editorlog_file = os.path.join(editor.workspace.release.paths.project_log(), 'Editor.log')

        # LY-107861 LY-108088
        # expected failure cases are commented out pending implementation of property validation by hydra
        expected_lines = [
            "Created new entity.",
            "Lightning Arc component added to entity.",
            #"ChangeProperty m_config|Arc Parameters|Segment Count to 0 failed.",
            "ChangeProperty m_config|Arc Parameters|Segment Count to 1 succeeded.",
            "ChangeProperty m_config|Arc Parameters|Segment Count to 25 succeeded.",
            "ChangeProperty m_config|Arc Parameters|Segment Count to 50 succeeded.",
            "ChangeProperty m_config|Arc Parameters|Segment Count to 70 succeeded.",
            #"ChangeProperty m_config|Arc Parameters|Segment Count to 75 failed.",
            #"ChangeProperty m_config|Arc Parameters|Segment Count to 100 failed.",
            #"ChangeProperty m_config|Arc Parameters|Point Count to 0 failed.",
            "ChangeProperty m_config|Arc Parameters|Point Count to 1 succeeded.",
            "ChangeProperty m_config|Arc Parameters|Point Count to 25 succeeded.",
            "ChangeProperty m_config|Arc Parameters|Point Count to 50 succeeded.",
            "ChangeProperty m_config|Arc Parameters|Point Count to 70 succeeded.",
            #"ChangeProperty m_config|Arc Parameters|Point Count to 75 failed.",
            #"ChangeProperty m_config|Arc Parameters|Point Count to 100 failed.",
            ]

        test_tools.shared.log_monitor.monitor_for_expected_lines(editor, editorlog_file, expected_lines)

        # Rely on the test script to quit after running
        editor.run(test_tools.launchers.phase.WaitForLauncherToQuit(editor, 10))
