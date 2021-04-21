"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import logging
import os
from pathlib import PurePath
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")
import ly_test_tools.environment.file_system as file_system

from atom_renderer.atom_utils import hydra_test_utils as hydra

logger = logging.getLogger(__name__)
EDITOR_TIMEOUT = 60
TEST_DIRECTORY = os.path.dirname(__file__)

# Go to the project root directory
PROJECT_DIRECTORY = PurePath(TEST_DIRECTORY)
if len(PROJECT_DIRECTORY.parents) > 5:
    for _ in range(5):
        PROJECT_DIRECTORY = PROJECT_DIRECTORY.parent


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestAllLevelsOpenClose(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project, level):
        # Cleanup our temp level
        file_system.delete(
            [os.path.join(workspace.paths.engine_root(), project, "Levels", "AtomLevels", level)], True, True)

        def teardown():
            # Cleanup our temp level
            file_system.delete(
                [os.path.join(workspace.paths.engine_root(), project, "Levels", "AtomLevels", level)], True, True)

        request.addfinalizer(teardown)

    @pytest.mark.test_case_id(
        "C34428159",
        "C34428160",
        "C34428161",
        "C34428162",
        "C34428163",
        "C34428165",
        "C34428166",
        "C34428167",
        "C34428158",
        "C34428172",
        "C34428173",
        "C34428174",
        "C34428175",
    )

    def test_AllLevelsOpenClose(self, request, editor, level, workspace, project, launcher_platform):

        cfg_args = [level]
        test_levels = os.path.join(str(PROJECT_DIRECTORY), "Levels", "AtomLevels")

        expected_lines = []
        for level in test_levels:
            expected_lines.append(f"Successfully opened {level}")

        unexpected_lines = [
            "failed to open",
            "Traceback (most recent call last):",
        ]

        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "AllLevelsOpenClose_test_case.py",
            timeout=EDITOR_TIMEOUT,
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            cfg_args=cfg_args,
        )
