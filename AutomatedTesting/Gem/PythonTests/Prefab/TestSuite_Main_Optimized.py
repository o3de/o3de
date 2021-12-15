"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorParallelTest, EditorTestSuite


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationNoAutoTestMode(EditorTestSuite):

    # Enable only -BatchMode for these tests. Some tests cannot run in -autotest_mode due to UI interactions
    global_extra_cmdline_args = ["-BatchMode"]

    class test_CreatePrefab_UnderAnEntity(EditorSharedTest):
        from .tests.create_prefab import CreatePrefab_UnderAnEntity as test_module

    class test_CreatePrefab_UnderAnotherPrefab(EditorSharedTest):
        from .tests.create_prefab import CreatePrefab_UnderAnotherPrefab as test_module

    class test_DeleteEntity_UnderAnotherPrefab(EditorSharedTest):
        from .tests.delete_entity import DeleteEntity_UnderAnotherPrefab as test_module

    class test_DeleteEntity_UnderLevelPrefab(EditorSharedTest):
        from .tests.delete_entity import DeleteEntity_UnderLevelPrefab as test_module

    class test_ReparentPrefab_UnderAnotherPrefab(EditorSharedTest):
        from .tests.reparent_prefab import ReparentPrefab_UnderAnotherPrefab as test_module

    class test_DetachPrefab_UnderAnotherPrefab(EditorSharedTest):
        from .tests.detach_prefab import DetachPrefab_UnderAnotherPrefab as test_module

    class test_OpenLevel_ContainingTwoEntities(EditorSharedTest):
        from .tests.open_level import OpenLevel_ContainingTwoEntities as test_module

    class test_CreatePrefab_WithSingleEntity(EditorSharedTest):
        from .tests.create_prefab import CreatePrefab_WithSingleEntity as test_module

    class test_InstantiatePrefab_ContainingASingleEntity(EditorSharedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_ContainingASingleEntity as test_module

    class test_DeletePrefab_ContainingASingleEntity(EditorSharedTest):
        from .tests.delete_prefab import DeletePrefab_ContainingASingleEntity as test_module

    class test_DuplicatePrefab_ContainingASingleEntity(EditorSharedTest):
        from .tests.duplicate_prefab import DuplicatePrefab_ContainingASingleEntity as test_module