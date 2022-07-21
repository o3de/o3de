"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorParallelTest, EditorTestSuite


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationAutoTestMode(EditorTestSuite):

    # Enable only -autotest_mode for these tests. Tests cannot run in -BatchMode due to UI interactions
    global_extra_cmdline_args = ["-autotest_mode"]

    class test_SC_Spawnables_SimpleSpawnAndDespawn(EditorSharedTest):
        from .tests.spawnables import SC_Spawnables_SimpleSpawnAndDespawn as test_module

    class test_SC_Spawnables_EntityClearedOnGameModeExit(EditorSharedTest):
        from .tests.spawnables import SC_Spawnables_EntityClearedOnGameModeExit as test_module

    class test_SC_Spawnables_MultipleSpawnsFromSingleTicket(EditorSharedTest):
        from .tests.spawnables import SC_Spawnables_MultipleSpawnsFromSingleTicket as test_module

    class test_SC_Spawnables_NestedSpawn(EditorSharedTest):
        from .tests.spawnables import SC_Spawnables_NestedSpawn as test_module

    class test_SC_Spawnables_DespawnOnEntityDeactivate(EditorSharedTest):
        from .tests.spawnables import SC_Spawnables_DespawnOnEntityDeactivate as test_module

    class test_Lua_Spawnables_SimpleSpawnAndDespawn(EditorSharedTest):
        from .tests.spawnables import Lua_Spawnables_SimpleSpawnAndDespawn as test_module

    class test_Lua_Spawnables_EntityClearedOnGameModeExit(EditorSharedTest):
        from .tests.spawnables import Lua_Spawnables_EntityClearedOnGameModeExit as test_module

    class test_Lua_Spawnables_MultipleSpawnsFromSingleTicket(EditorSharedTest):
        from .tests.spawnables import Lua_Spawnables_MultipleSpawnsFromSingleTicket as test_module

    class test_Lua_Spawnables_NestedSpawn(EditorSharedTest):
        from .tests.spawnables import Lua_Spawnables_NestedSpawn as test_module

    class test_Lua_Spawnables_DespawnOnEntityDeactivate(EditorSharedTest):
        from .tests.spawnables import Lua_Spawnables_DespawnOnEntityDeactivate as test_module
