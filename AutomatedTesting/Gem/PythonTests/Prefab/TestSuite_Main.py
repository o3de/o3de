"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

 """

# This suite consists of all test cases that are passing and have been verified.

import pytest
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase

@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def _run_prefab_test(self, request, workspace, editor, test_module, batch_mode=True, autotest_mode=True):
        self._run_test(request, workspace, editor, test_module, 
            batch_mode=batch_mode,
            autotest_mode=autotest_mode)

    def test_OpenLevel_ContainingTwoEntities(self, request, workspace, editor, launcher_platform):
        from Prefab.tests.open_level import OpenLevel_ContainingTwoEntities as test_module
        self._run_prefab_test(request, workspace, editor, test_module)

    def test_CreatePrefab_WithSingleEntity(self, request, workspace, editor, launcher_platform):
        from Prefab.tests.create_prefab import CreatePrefab_WithSingleEntity as test_module
        self._run_prefab_test(request, workspace, editor, test_module)
        
    def test_InstantiatePrefab_ContainingASingleEntity(self, request, workspace, editor, launcher_platform):
        from Prefab.tests.instantiate_prefab import InstantiatePrefab_ContainingASingleEntity as test_module
        self._run_prefab_test(request, workspace, editor, test_module)

    def test_DeletePrefab_ContainingASingleEntity(self, request, workspace, editor, launcher_platform):
        from Prefab.tests.delete_prefab import DeletePrefab_ContainingASingleEntity as test_module
        self._run_prefab_test(request, workspace, editor, test_module)

    def test_ReparentPrefab_UnderAnotherPrefab(self, request, workspace, editor, launcher_platform):
        from Prefab.tests.reparent_prefab import ReparentPrefab_UnderAnotherPrefab as test_module
        self._run_prefab_test(request, workspace, editor, test_module, autotest_mode=False)

    def test_DetachPrefab_UnderAnotherPrefab(self, request, workspace, editor, launcher_platform):
        from Prefab.tests.detach_prefab import DetachPrefab_UnderAnotherPrefab as test_module
        self._run_prefab_test(request, workspace, editor, test_module, autotest_mode=False)

    def test_DuplicatePrefab_ContainingASingleEntity(self, request, workspace, editor, launcher_platform):
        from Prefab.tests.duplicate_prefab import DuplicatePrefab_ContainingASingleEntity as test_module
        self._run_prefab_test(request, workspace, editor, test_module)

    def test_CreatePrefab_UnderAnEntity(self, request, workspace, editor, launcher_platform):
        from Prefab.tests.create_prefab import CreatePrefab_UnderAnEntity as test_module
        self._run_prefab_test(request, workspace, editor, test_module, autotest_mode=False)

    def test_CreatePrefab_UnderAnotherPrefab(self, request, workspace, editor, launcher_platform):
        from Prefab.tests.create_prefab import CreatePrefab_UnderAnotherPrefab as test_module
        self._run_prefab_test(request, workspace, editor, test_module, autotest_mode=False)

    def test_DeleteEntity_UnderAnotherPrefab(self, request, workspace, editor, launcher_platform):
        from Prefab.tests.delete_entity import DeleteEntity_UnderAnotherPrefab as test_module
        self._run_prefab_test(request, workspace, editor, test_module, autotest_mode=False)

    def test_DeleteEntity_UnderLevelPrefab(self, request, workspace, editor, launcher_platform):
        from Prefab.tests.delete_entity import DeleteEntity_UnderLevelPrefab as test_module
        self._run_prefab_test(request, workspace, editor, test_module, autotest_mode=False)
