"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorBatchedTest, EditorTestSuite


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationOverridesDisabled(EditorTestSuite):

    # These tests will execute with Outliner Overrides/Inspector DPE/Inspector Overrides disabled
    EditorTestSuite.global_extra_cmdline_args.extend(
        [f"--regset=/O3DE/Autoexec/ConsoleCommands/ed_enableOutlinerOverrideManagement=false",
         f"--regset=/O3DE/Autoexec/ConsoleCommands/ed_enableInspectorOverrideManagement=false",
         f"--regset=/O3DE/Autoexec/ConsoleCommands/ed_enableDPEInspector=false"])

    # Add Entity Tests
    class test_AddEntity_UnderAnotherEntity(EditorBatchedTest):
        from .tests.add_entity import AddEntity_UnderAnotherEntity as test_module

    class test_AddEntity_UnderChildEntityOfPrefab(EditorBatchedTest):
        from .tests.add_entity import AddEntity_UnderChildEntityOfPrefab as test_module

    class test_AddEntity_UnderContainerEntityOfPrefab(EditorBatchedTest):
        from .tests.add_entity import AddEntity_UnderContainerEntityOfPrefab as test_module

    class test_AddEntity_UnderLevelPrefab(EditorBatchedTest):
        from .tests.add_entity import AddEntity_UnderLevelPrefab as test_module

    # Create Prefab Tests

    class test_CreatePrefab_ComponentConfigurationRetained(EditorBatchedTest):
        from .tests.create_prefab import CreatePrefab_ComponentConfigurationRetained as test_module

    class test_CreatePrefab_CreationFailsWithDifferentRootEntities(EditorBatchedTest):
        from .tests.create_prefab import CreatePrefab_CreationFailsWithDifferentRootEntities as test_module

    class test_CreatePrefab_UnderAnEntity(EditorBatchedTest):
        from .tests.create_prefab import CreatePrefab_UnderAnEntity as test_module

    class test_CreatePrefab_UnderAnotherPrefab(EditorBatchedTest):
        from .tests.create_prefab import CreatePrefab_UnderAnotherPrefab as test_module

    class test_CreatePrefab_UnderChildEntityOfAnotherPrefab(EditorBatchedTest):
        from .tests.create_prefab import CreatePrefab_UnderChildEntityOfAnotherPrefab as test_module

    class test_CreatePrefab_WithNestedEntities(EditorBatchedTest):
        from .tests.create_prefab import CreatePrefab_WithNestedEntities as test_module
    
    class test_CreatePrefab_WithNestedEntitiesAndNestedPrefabs(EditorBatchedTest):
        from .tests.create_prefab import CreatePrefab_WithNestedEntitiesAndNestedPrefabs as test_module

    class test_CreatePrefab_WithSingleEntity(EditorBatchedTest):
        from .tests.create_prefab import CreatePrefab_WithSingleEntity as test_module

    # Delete Entity Tests

    class test_DeleteEntity_UnderAnotherPrefab(EditorBatchedTest):
        from .tests.delete_entity import DeleteEntity_UnderAnotherPrefab as test_module

    class test_DeleteEntity_UnderLevelPrefab(EditorBatchedTest):
        from .tests.delete_entity import DeleteEntity_UnderLevelPrefab as test_module

    class test_DeleteEntity_UnderNestedEntityHierarchy(EditorBatchedTest):
        from .tests.delete_entity import DeleteEntity_UnderNestedEntityHierarchy as test_module

    # Delete Prefab Tests

    class test_DeletePrefab_ContainingASingleEntity(EditorBatchedTest):
        from .tests.delete_prefab import DeletePrefab_ContainingASingleEntity as test_module

    class test_DeletePrefab_ContainingNestedEntitiesAndNestedPrefabs(EditorBatchedTest):
        from .tests.delete_prefab import DeletePrefab_ContainingNestedEntitiesAndNestedPrefabs as test_module

    class test_DeletePrefab_DuplicatedPrefabInstance(EditorBatchedTest):
        from .tests.delete_prefab import DeletePrefab_DuplicatedPrefabInstance as test_module

    # Detach Prefab Tests

    class test_DetachPrefab_UnderAnotherPrefab(EditorBatchedTest):
        from .tests.detach_prefab import DetachPrefab_UnderAnotherPrefab as test_module

    class test_DetachPrefab_WithNestedEntities(EditorBatchedTest):
        from .tests.detach_prefab import DetachPrefab_WithNestedEntities as test_module

    class test_DetachPrefab_WithSingleEntity(EditorBatchedTest):
        from .tests.detach_prefab import DetachPrefab_WithSingleEntity as test_module

    # Duplicate Prefab Tests

    class test_DuplicateEntity_WithNestedEntities(EditorBatchedTest):
        from .tests.duplicate_prefab import DuplicateEntity_WithNestedEntities as test_module

    class test_DuplicateEntity_WithNestedEntitiesAndNestedPrefabs(EditorBatchedTest):
        from .tests.duplicate_prefab import DuplicateEntity_WithNestedEntitiesAndNestedPrefabs as test_module

    class test_DuplicatePrefab_ContainingASingleEntity(EditorBatchedTest):
        from .tests.duplicate_prefab import DuplicatePrefab_ContainingASingleEntity as test_module

    class test_DuplicatePrefab_ContainingNestedEntitiesAndNestedPrefabs(EditorBatchedTest):
        from .tests.duplicate_prefab import DuplicatePrefab_ContainingNestedEntitiesAndNestedPrefabs as test_module

    # Instantiate Prefab Tests

    class test_InstantiatePrefab_ContainingASingleEntity(EditorBatchedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_ContainingASingleEntity as test_module

    class test_InstantiatePrefab_FromCreatedPrefabWithSingleEntity(EditorBatchedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_FromCreatedPrefabWithSingleEntity as test_module

    class test_InstantiatePrefab_LevelPrefab(EditorBatchedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_LevelPrefab as test_module

    class test_InstantiatePrefab_WithNestedEntities(EditorBatchedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_WithNestedEntities as test_module

    class test_InstantiatePrefab_WithNestedEntitiesAndNestedPrefabs(EditorBatchedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_WithNestedEntitiesandNestedPrefabs as test_module

    # Open Level Tests

    class test_OpenLevel_ContainingTwoEntities(EditorBatchedTest):
        from .tests.open_level import OpenLevel_ContainingTwoEntities as test_module
        
    # Prefab Notifications Tests

    class test_PrefabNotifications_PropagationNotificationsReceived(EditorBatchedTest):
        from .tests.prefab_notifications import PrefabNotifications_PropagationNotificationsReceived as test_module

    class test_PrefabNotifications_RootPrefabLoadedNotificationsReceived(EditorBatchedTest):
        from .tests.prefab_notifications import PrefabNotifications_RootPrefabLoadedNotificationsReceived as test_module

    # Reparent Prefab Tests

    class test_ReparentEntity_UnderEntityHierarchies(EditorBatchedTest):
        from .tests.reparent_prefab import ReparentEntity_UnderEntityHierarchies as test_module

    # Spawnables Tests

    class test_SC_Spawnables_DespawnOnEntityDeactivate(EditorBatchedTest):
        from .tests.spawnables import SC_Spawnables_DespawnOnEntityDeactivate as test_module

    class test_SC_Spawnables_EntityClearedOnGameModeExit(EditorBatchedTest):
        from .tests.spawnables import SC_Spawnables_EntityClearedOnGameModeExit as test_module

    class test_SC_Spawnables_MultipleSpawnsFromSingleTicket(EditorBatchedTest):
        from .tests.spawnables import SC_Spawnables_MultipleSpawnsFromSingleTicket as test_module

    class test_SC_Spawnables_NestedSpawn(EditorBatchedTest):
        from .tests.spawnables import SC_Spawnables_NestedSpawn as test_module

    class test_SC_Spawnables_SimpleSpawnAndDespawn(EditorBatchedTest):
        from .tests.spawnables import SC_Spawnables_SimpleSpawnAndDespawn as test_module

    class test_Lua_Spawnables_DespawnOnEntityDeactivate(EditorBatchedTest):
        from .tests.spawnables import Lua_Spawnables_DespawnOnEntityDeactivate as test_module

    class test_Lua_Spawnables_EntityClearedOnGameModeExit(EditorBatchedTest):
        from .tests.spawnables import Lua_Spawnables_EntityClearedOnGameModeExit as test_module

    class test_Lua_Spawnables_MultipleSpawnsFromSingleTicket(EditorBatchedTest):
        from .tests.spawnables import Lua_Spawnables_MultipleSpawnsFromSingleTicket as test_module

    class test_Lua_Spawnables_NestedSpawn(EditorBatchedTest):
        from .tests.spawnables import Lua_Spawnables_NestedSpawn as test_module

    class test_Lua_Spawnables_SimpleSpawnAndDespawn(EditorBatchedTest):
        from .tests.spawnables import Lua_Spawnables_SimpleSpawnAndDespawn as test_module


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationOverrides(EditorTestSuite):

    # These tests will execute with Outliner Overrides/Inspector DPE/Inspector Overrides enabled
    EditorTestSuite.global_extra_cmdline_args.extend(
        [f"--regset=/O3DE/Autoexec/ConsoleCommands/ed_enableOutlinerOverrideManagement=true",
         f"--regset=/O3DE/Autoexec/ConsoleCommands/ed_enableInspectorOverrideManagement=true",
         f"--regset=/O3DE/Autoexec/ConsoleCommands/ed_enableDPEInspector=true"])

    # Overrides Tests

    class test_AddEntity_UnderUnfocusedInstanceAsOverride(EditorBatchedTest):
        from .tests.overrides import AddEntity_UnderUnfocusedInstanceAsOverride as test_module

    class test_DeleteEntity_UnderImmediateInstance(EditorBatchedTest):
        from .tests.overrides import DeleteEntity_UnderImmediateInstance as test_module

    class test_DeleteEntity_UnderNestedInstance(EditorBatchedTest):
        from .tests.overrides import DeleteEntity_UnderNestedInstance as test_module

    class test_DeletePrefab_UnderImmediateInstance(EditorBatchedTest):
        from .tests.overrides import DeletePrefab_UnderImmediateInstance as test_module

    class test_DeletePrefab_UnderNestedInstance(EditorBatchedTest):
        from .tests.overrides import DeletePrefab_UnderNestedInstance as test_module

    class test_EditEntity_UnderImmediateInstance(EditorBatchedTest):
        from .tests.overrides import EditEntity_UnderImmediateInstance as test_module

    class test_EditEntity_UnderNestedInstance(EditorBatchedTest):
        from .tests.overrides import EditEntity_UnderNestedInstance as test_module
