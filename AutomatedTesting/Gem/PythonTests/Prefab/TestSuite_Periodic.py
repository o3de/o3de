"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorBatchedTest, EditorParallelTest, EditorTestSuite


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationNoAutoTestMode(EditorTestSuite):

    global_extra_cmdline_args = ["--regset=O3DE/Preferences/Prefabs/EnableOverridesUx=true"]

    # Delete tests
    class test_DeleteEntity_UnderNestedEntityHierarchy(EditorBatchedTest):
        from .tests.delete_entity import DeleteEntity_UnderNestedEntityHierarchy as test_module

    # Duplicate tests
    class test_DuplicateEntity_WithNestedEntities(EditorBatchedTest):
        from .tests.duplicate_prefab import DuplicateEntity_WithNestedEntities as test_module

    class test_DuplicateEntity_WithNestedEntitiesAndNestedPrefabs(EditorBatchedTest):
        from .tests.duplicate_prefab import DuplicateEntity_WithNestedEntitiesAndNestedPrefabs as test_module

    # Instantiate tests

    class test_InstantiatePrefab_LevelPrefab(EditorBatchedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_LevelPrefab as test_module

    class test_InstantiatePrefab_WithNestedEntities(EditorBatchedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_WithNestedEntities as test_module

    class test_InstantiatePrefab_WithNestedEntitiesAndNestedPrefabs(EditorBatchedTest):
        from .tests.instantiate_prefab import InstantiatePrefab_WithNestedEntitiesandNestedPrefabs as test_module

    # Overrides tests

    class test_AddEntity_UnderUnfocusedInstanceAsOverride(EditorBatchedTest):
        from .tests.overrides import AddEntity_UnderUnfocusedInstanceAsOverride as test_module

    # Spawnables tests

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
