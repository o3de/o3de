"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorBatchedTest, EditorTestSuite


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    # These tests will execute without enabling prefab overrides
    EditorTestSuite.global_extra_cmdline_args.append("--regset=O3DE/Preferences/Prefabs/EnableOverridesUx=false")

    # Delete Tests
    class test_DeleteEntity_UnderNestedEntityHierarchy(EditorBatchedTest):
        from .tests.delete_entity import DeleteEntity_UnderNestedEntityHierarchy as test_module

    # Duplicate Tests
    class test_DuplicateEntity_WithNestedEntities(EditorBatchedTest):
        from .tests.duplicate_prefab import DuplicateEntity_WithNestedEntities as test_module

    class test_DuplicateEntity_WithNestedEntitiesAndNestedPrefabs(EditorBatchedTest):
        from .tests.duplicate_prefab import DuplicateEntity_WithNestedEntitiesAndNestedPrefabs as test_module

    # Instantiate Tests

    class test_InstantiatePrefab_LevelPrefab(EditorBatchedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_LevelPrefab as test_module

    class test_InstantiatePrefab_WithNestedEntities(EditorBatchedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_WithNestedEntities as test_module

    class test_InstantiatePrefab_WithNestedEntitiesAndNestedPrefabs(EditorBatchedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_WithNestedEntitiesandNestedPrefabs as test_module

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


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationOverrides(EditorTestSuite):

    # These tests will execute with prefab overrides enabled
    EditorTestSuite.global_extra_cmdline_args.append("--regset=O3DE/Preferences/Prefabs/EnableOverridesUx=true")

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
