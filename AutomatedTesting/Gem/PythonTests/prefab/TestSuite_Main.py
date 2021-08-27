"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

 """

# This suite consists of all test cases that are passing and have been verified.

import pytest
import os
import sys

import editor_python_test_tools.hydra_test_utils as hydra

from prefab.Prefab_Test_Results import Results
from prefab.Prefab_Test_Results import UnexpectedResults

from ly_test_tools import LAUNCHERS
sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def _run_prefab_test(self, request, workspace, editor, test_module):
        self._run_test(request, workspace, editor, test_module, ["--regset=/Amazon/Preferences/EnablePrefabSystem=true"])

    def test_PrefabLevel_OpensLevelWithEntities(self, request, workspace, editor, launcher_platform):
        from . import PrefabLevel_OpensLevelWithEntities as test_module
        self._run_prefab_test(request, workspace, editor, test_module)


TEST_DIRECTORY = os.path.dirname(__file__)
LOG_MONITOR_TIMEOUT = 180
PREFAB_TEST_LEVEL_FOLDER = "Prefab"
PREFAB_TEST_BASE_LEVEL_NAME = "Base"
PREFAB_TEST_BASE_LEVEL_FILE_PATH = os.path.join(PREFAB_TEST_LEVEL_FOLDER, PREFAB_TEST_BASE_LEVEL_NAME)


def test_Prefab_BasicWorkflow(request, editor, level, 
                              test_file_name, expected_lines, timeout=LOG_MONITOR_TIMEOUT):
    hydra.launch_and_validate_results(
        request, TEST_DIRECTORY, editor, 
        test_file_name, expected_lines, 
        unexpected_lines=UnexpectedResults, cfg_args=[level], auto_test_mode=False, enable_prefab_system=True,
        timeout=timeout,
    )


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", [PREFAB_TEST_BASE_LEVEL_FILE_PATH])
class TestPrefabBasicWorkflows(object):

    def test_Prefab_BasicWorkflow_CreatePrefab(self, request, editor, launcher_platform, level):
        expected_lines = [
            Results.create_entity_succeeded,
            Results.create_prefab_succeeded,
        ]

        test_Prefab_BasicWorkflow(request, editor, level, 
            "Prefab_BasicWorkflow_CreatePrefab.py", expected_lines)
        

    def test_Prefab_BasicWorkflow_InstantiatePrefab(self, request, editor, launcher_platform, level):
        expected_lines = [
            Results.instantiate_prefab_succeeded,
            Results.instantiated_prefab_at_expected_position,
        ]

        test_Prefab_BasicWorkflow(request, editor, level, 
            "Prefab_BasicWorkflow_InstantiatePrefab.py", expected_lines)


    def test_Prefab_BasicWorkflow_CreateAndDeletePrefab(self, request, editor, launcher_platform, level):
        expected_lines = [
            Results.create_entity_succeeded,
            Results.create_prefab_succeeded,
            Results.delete_prefab_succeeded,
            Results.delete_prefab_entities_and_descendants_succeeded, 
        ]

        test_Prefab_BasicWorkflow(request, editor, level, 
            "Prefab_BasicWorkflow_CreateAndDeletePrefab.py", expected_lines)


    def test_Prefab_BasicWorkflow_CreateAndReparentPrefab(self, request, editor, launcher_platform, level):
        expected_lines = [
            Results.create_entity_succeeded,
            Results.create_prefab_succeeded,
            Results.reparent_prefab_succeeded,
        ]

        test_Prefab_BasicWorkflow(request, editor, level, 
            "Prefab_BasicWorkflow_CreateAndReparentPrefab.py", expected_lines)
