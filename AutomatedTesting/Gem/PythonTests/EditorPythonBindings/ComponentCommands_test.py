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


@pytest.mark.skip  # SPEC-4102
@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize('launcher_platform', ['windows_editor'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['Simple'])
class TestComponentCommands(object):

    # It needs a new test level in prefab format to make it testable again.
    def test_MeshComponentBasics(self, request, editor, level, launcher_platform):

        unexpected_lines=[]
        expected_lines = [
            "Type Ids List returned correctly",
            "Type Names List returned correctly",
            "New entity with no parent created",
            "Entity does not have a Mesh component",
            "Mesh component added to entity",
            "EntityId on the meshComponent EntityComponentIdPair matches",
            "EntityComponentIdPair to_string works",
            "Entity has a Mesh component",
            "Mesh component is active",
            "Mesh component is not active",
            "Mesh component is valid",
            "Comment components added to entity",
            "Got both Comment components",
            "GetComponent works",
            "Entity has two Comment components",
            "Disabled both Comment components",
            "Enabled both Comment components",
            "Mesh Component removed",
            "Mesh component is no longer valid",
            "Single comment component added to entity",
            "Entity has three Comment components",
            "Mesh Collider component added to entity",
            "Mesh Collider component retrieved from entity",
            "Mesh Collider component removed from entity"
            ]
        
        test_case_file = os.path.join(os.path.dirname(__file__), 'ComponentCommands_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)

