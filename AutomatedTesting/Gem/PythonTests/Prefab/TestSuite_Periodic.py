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
class TestAutomationNoOverrides(EditorTestSuite):

    # These tests will execute with prefab outliner overrides disabled
    EditorTestSuite.global_extra_cmdline_args.append("--regset=O3DE/Preferences/Prefabs/EnableOutlinerOverrideManagement=false")

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
