"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
C1564077: The Tools menu options function normally - New view interaction Model enabled
https://testrail.agscollab.com/index.php?/cases/view/1564077
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system
import ly_test_tools.environment.process_utils as process_utils

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 80


@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['tmp_level'])
class TestToolsMenu(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project, level):
        def teardown():
            workspace = editor.workspace
            # delete temp level
            file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)
            process_utils.kill_processes_named('LuaIDE', True)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)
    
    expected_lines = [
            "Animation Editor (PREVIEW) Action triggered",
            "Asset Browser Action triggered",
            "Asset Editor Action triggered",
            "Console Action triggered",
            "Entity Inspector Action triggered",
            "ImGui Editor Action triggered",
            "Landscape Canvas Action triggered",
            "Level Inspector Action triggered",
            "Lua Editor Action triggered",
            "Material Editor Action triggered",
            "Particle Editor Action triggered",
            "PhysX Configuration (PREVIEW) Action triggered",
            "Script Canvas Action triggered",
            "Slice Relationship View (PREVIEW) Action triggered",
            "Track View Action triggered",
            "UI Editor Action triggered",
            "Vegetation Editor Action triggered",
            "Audio Controls Editor Action triggered",
            "Console Variables Action triggered",
            "Lens Flare Editor Action triggered",
            "Measurement System Tool Action triggered",
            "Python Console Action triggered",
            "Python Scripts Action triggered",
            "Slice Favorites Action triggered",
            "Sun Trajectory Tool Action triggered",
            "Terrain Texture Layers Action triggered",
            "Time Of Day Action triggered",
            "Substance Editor Action triggered",
            "Viewport Camera Selector Action triggered",
        ]
    
    @pytest.mark.test_case_id("C1564077")
    def test_tools_menu_options(self, request, editor, level):
        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "tools_menu_after_interaction_model_toggle.py",
            self.expected_lines,
            cfg_args=[level],
            auto_test_mode=False,
            run_python="--runpython",
            timeout=log_monitor_timeout,
        )

   