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

    # Add Entity Tests

    class test_AddEntity_UnderAnotherEntity(EditorSharedTest):
        from .tests.add_entity import AddEntity_UnderAnotherEntity as test_module

    class test_AddEntity_UnderChildEntityOfPrefab(EditorSharedTest):
        from .tests.add_entity import AddEntity_UnderChildEntityOfPrefab as test_module

    class test_AddEntity_UnderContainerEntityOfPrefab(EditorSharedTest):
        from .tests.add_entity import AddEntity_UnderContainerEntityOfPrefab as test_module

    class test_AddEntity_UnderLevelPrefab(EditorSharedTest):
        from .tests.add_entity import AddEntity_UnderLevelPrefab as test_module

    # Create Prefab Tests

    class test_CreatePrefab_ComponentConfigurationRetained(EditorSharedTest):
        from .tests.create_prefab import CreatePrefab_ComponentConfigurationRetained as test_module

    class test_CreatePrefab_UnderAnEntity(EditorSharedTest):
        from .tests.create_prefab import CreatePrefab_UnderAnEntity as test_module

    class test_CreatePrefab_UnderAnotherPrefab(EditorSharedTest):
        from .tests.create_prefab import CreatePrefab_UnderAnotherPrefab as test_module

    class test_CreatePrefab_UnderChildEntityOfAnotherPrefab(EditorSharedTest):
        from .tests.create_prefab import CreatePrefab_UnderChildEntityOfAnotherPrefab as test_module

    class test_CreatePrefab_WithNestedEntities(EditorSharedTest):
        from .tests.create_prefab import CreatePrefab_WithNestedEntities as test_module
    
    class test_CreatePrefab_WithNestedEntitiesAndNestedPrefabs(EditorSharedTest):
        from .tests.create_prefab import CreatePrefab_WithNestedEntitiesAndNestedPrefabs as test_module

    class test_CreatePrefab_WithSingleEntity(EditorSharedTest):
        from .tests.create_prefab import CreatePrefab_WithSingleEntity as test_module

    # Delete Entity Tests

    class test_DeleteEntity_UnderAnotherPrefab(EditorSharedTest):
        from .tests.delete_entity import DeleteEntity_UnderAnotherPrefab as test_module

    class test_DeleteEntity_UnderLevelPrefab(EditorSharedTest):
        from .tests.delete_entity import DeleteEntity_UnderLevelPrefab as test_module

    # Delete Prefab Tests

    class test_DeletePrefab_ContainingASingleEntity(EditorSharedTest):
        from .tests.delete_prefab import DeletePrefab_ContainingASingleEntity as test_module

    class test_DeletePrefab_ContainingNestedEntitiesAndNestedPrefabs(EditorSharedTest):
        from .tests.delete_prefab import DeletePrefab_ContainingNestedEntitiesAndNestedPrefabs as test_module

    class test_DeletePrefab_DuplicatedPrefabInstance(EditorSharedTest):
        from .tests.delete_prefab import DeletePrefab_DuplicatedPrefabInstance as test_module

    # Detach Prefab Tests

    class test_DetachPrefab_UnderAnotherPrefab(EditorSharedTest):
        from .tests.detach_prefab import DetachPrefab_UnderAnotherPrefab as test_module

    class test_DetachPrefab_WithNestedEntities(EditorSharedTest):
        from .tests.detach_prefab import DetachPrefab_WithNestedEntities as test_module

    class test_DetachPrefab_WithSingleEntity(EditorSharedTest):
        from .tests.detach_prefab import DetachPrefab_WithSingleEntity as test_module

    # Duplicate Prefab Tests

    class test_DuplicatePrefab_ContainingASingleEntity(EditorSharedTest):
        from .tests.duplicate_prefab import DuplicatePrefab_ContainingASingleEntity as test_module

    class test_DuplicatePrefab_ContainingNestedEntitiesAndNestedPrefabs(EditorSharedTest):
        from .tests.duplicate_prefab import DuplicatePrefab_ContainingNestedEntitiesAndNestedPrefabs as test_module

    # Instantiate Prefab Tests

    class test_InstantiatePrefab_ContainingASingleEntity(EditorSharedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_ContainingASingleEntity as test_module

    class test_InstantiatePrefab_FromCreatedPrefabWithSingleEntity(EditorSharedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_FromCreatedPrefabWithSingleEntity as test_module

    # Open Level Tests

    class test_OpenLevel_ContainingTwoEntities(EditorSharedTest):
        from .tests.open_level import OpenLevel_ContainingTwoEntities as test_module

    # Overrides Tests

    class test_EditEntity_UnderImmediateInstance(EditorSharedTest):
        from .tests.overrides import EditEntity_UnderImmediateInstance as test_module

    class test_EditEntity_UnderNestedInstance(EditorSharedTest):
        from .tests.overrides import EditEntity_UnderNestedInstance as test_module
        
    # Prefab Notifications Tests

    class test_PrefabNotifications_PropagationNotificationsReceived(EditorSharedTest):
        from .tests.prefab_notifications import PrefabNotifications_PropagationNotificationsReceived as test_module

    class test_PrefabNotifications_RootPrefabLoadedNotificationsReceived(EditorSharedTest):
        from .tests.prefab_notifications import PrefabNotifications_RootPrefabLoadedNotificationsReceived as test_module

    # Reparent Prefab Tests

    class test_ReparentEntity_UnderEntityHierarchies(EditorSharedTest):
        from .tests.reparent_prefab import ReparentEntity_UnderEntityHierarchies as test_module
