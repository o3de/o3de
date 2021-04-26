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
import pytest

import ly_test_tools.environment.file_system as file_system

import editor_python_test_tools.hydra_test_utils as hydra

logger = logging.getLogger(__name__)
EDITOR_TIMEOUT = 60
TEST_DIRECTORY = os.path.join(os.path.dirname(__file__), "atom_hydra_scripts")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("level", ["tmp_level"])
class TestAtomEditorComponents(object):
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

    # It requires at least one test
    def test_Dummy(self, request, editor, level, workspace, project, launcher_platform):
        pass
