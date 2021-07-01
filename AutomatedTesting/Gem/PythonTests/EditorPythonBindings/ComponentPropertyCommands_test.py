"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

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
class TestGradientRequiresShape(object):

    @pytest.mark.skip  # SPEC-4102
    def test_ComponentProperty(self, request, editor, level, launcher_platform):

        unexpected_lines=[]
        expected_lines = [
            "New entity with no parent created",
            "Environment Probe component added to entity",
            "Entity has an Environment Probe component",
            "get_paths_list works",
            "GetSetCompareTest Settings|General Settings|Visible: SUCCESS",
            "GetSetCompareTest Settings|Animation|Style: SUCCESS",
            "GetSetCompareTest Settings|Environment Probe Settings|Box height: SUCCESS",
            "GetSetCompareTest Settings|General Settings|Color: SUCCESS",
            "GetSetCompareTest Settings|Environment Probe Settings|Area dimensions: SUCCESS",
            "PteTest Settings|General Settings|Visible: SUCCESS",
            "PteTest Settings|Animation|Style: SUCCESS",
            "PteTest Settings|Environment Probe Settings|Box height: SUCCESS",
            "PteTest Settings|General Settings|Color: SUCCESS",
            "PteTest Settings|Environment Probe Settings|Area dimensions: SUCCESS",
            ]
        
        test_case_file = os.path.join(os.path.dirname(__file__), 'ComponentPropertyCommands_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)

    @pytest.mark.skip  # SPEC-4102
    def test_SetDistance_Between_FilterBound_Mode(self, request, editor, level, launcher_platform):

        unexpected_lines = ['FAILURE', 'script failure']
        expected_lines = [
            "New entity with no parent created: SUCCESS",
            "Components added to entity: SUCCESS",
            "Found Vegetation Distance Between Filter: SUCCESS",
            "CompareComponentProperty - Configuration|Bound Mode: SUCCESS",
            "GetSetCompareTest - Configuration|Bound Mode: SUCCESS"
        ]

        test_case_file = os.path.join(os.path.dirname(__file__), 'ComponentPropertyCommands_test_enum.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)

    @pytest.mark.skip  # LYN-1951
    def test_PropertyTreeVisibility(self, request, editor, level, launcher_platform):

        unexpected_lines = ['FAILURE', 'script failure']
        expected_lines = [
            "oceanEntityId was found: SUCCESS",
            "Found Infinite Ocean component ID: SUCCESS",
            "Created a PropertyTreeEditor for the infiniteOceanId: SUCCESS",
            "Found proprety hidden node in path: SUCCESS",
            "Proprety node is now a hidden path: SUCCESS",
            "Property path enforcement of visibility: SUCCESS"
        ]

        test_case_file = os.path.join(os.path.dirname(__file__), 'ComponentPropertyCommands_test_case_visibility.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)

    @pytest.mark.skip  # SPEC-4102
    def test_PropertyContainerOpeartions(self, request, editor, level, launcher_platform):
        unexpected_lines = ['FAILURE', 'script failure']
        expected_lines = [
            "New entity with no parent created: SUCCESS",
            "GradientSurfaceDataComponent added to entity :SUCCESS",
            "Has zero items: SUCCESS",
            "Add an item 0: SUCCESS",
            "Has one item 0: SUCCESS",
            "Add an item 1: SUCCESS",
            "Add an item 2: SUCCESS",
            "Add an item 3: SUCCESS",
            "Has four items: SUCCESS",
            "Updated an item: SUCCESS",
            "itemTag equals tagFour: SUCCESS",
            "Removed one item 0: SUCCESS",
            "Removed one item 1: SUCCESS",
            "Has two items: SUCCESS",
            "Reset items: SUCCESS",
            "Has cleared the items: SUCCESS"
        ]

        test_case_file = os.path.join(os.path.dirname(__file__), 'ComponentPropertyCommands_test_containers.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)

    @pytest.mark.skip  # LYN-1951
    def test_PropertyContainerOpeartionWithNone(self, request, editor, level, launcher_platform):
        unexpected_lines = ['FAILURE', 'script failure']
        expected_lines = [
            "material current is valid - True: SUCCESS",
            "material set to None: SUCCESS",
            "material has been set to None: SUCCESS"
        ]

        test_case_file = os.path.join(os.path.dirname(__file__), 'ComponentPropertyCommands_test_case_set_none.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)
