"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#
# This is a pytest module to test the in-Editor Python API from PythonEditorFuncs
#
import pytest
pytest.importorskip('ly_test_tools')

import sys
import os
sys.path.append(os.path.dirname(__file__))
from hydra_utils import launch_test_case

@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize('launcher_platform', ['windows_editor'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['Simple'])
class TestObjectManagerAutomation(object):

    # It needs a new test level in prefab format to make it testable again.
    def test_ViewPane(self, request, editor, level, launcher_platform):

        unexpected_lines=[]
        expected_lines = [
            "get_all_objects works",
            "get_names_of_selected_objects works",
            "select_objects works",
            "sel_position and sel_aabb both work",
            "get_num_selected and unselect_objects both work",
            "clear_selection works",
            "hide_all_objects works",
            "unhide_object works",
            "hide_object works",
            "freeze_object works",
            "position setter/getter works",
            "rotation setter/getter works",
            "scale setter/getter works",
            "delete_selected works",
            "delete_object works",
            "rename_object works"
            ]
        
        test_case_file = os.path.join(os.path.dirname(__file__), 'ObjectManagerCommands_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)

