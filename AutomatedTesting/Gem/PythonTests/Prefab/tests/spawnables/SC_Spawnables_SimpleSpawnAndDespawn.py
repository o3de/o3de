"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def SC_Spawnables_SimpleSpawnAndDespawn():

    import os

    import azlmbr.entity as entity
    import azlmbr.bus as bus
    import azlmbr.math as math
    import azlmbr.legacy.general as general

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.prefab_utils import Prefab

    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    helper.init_idle()
    helper.open_level("Prefab", "SC_Spawnables_SimpleSpawnDespawn")

    # Search for expected entities in edit mode
    search_filter = entity.SearchFilter()
    search_filter.names = ["SC_Spawner"]
    spawner_entity = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)[0]
    search_filter.names = ["PinkFlower"]
    flower_entity = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
    assert spawner_entity is not None, "Failed to find Spawner entity"
    assert len(flower_entity) == 0, "Unexpectedly found PinkFlower entity prior to spawn command"

    # Enter Game Mode and search for expected entities
    general.enter_game_mode()
    general.idle_wait(1.0)
    spawner_entity = general.find_game_entity("SC_Spawner")
    flower_entity = general.find_game_entity("PinkFlower")
    assert spawner_entity is not None, "Failed to find Spawner entity at runtime"
    assert flower_entity is not None, "Failed to find PinkFlower entity at runtime"

    # Wait for SC graph to exit Game Mode on despawn of PinkFlower.spawnable and search for expected entities
    game_mode_exited = helper.wait_for_condition(lambda: not general.is_in_game_mode(), 3.0)
    assert game_mode_exited, "SC graph failed to exit Game Mode"
    search_filter.names = ["SC_Spawner"]
    spawner_entity = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)[0]
    search_filter.names = ["PinkFlower"]
    flower_entity = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
    assert spawner_entity is not None, "Failed to find Spawner entity"
    assert len(flower_entity) == 0, "Unexpectedly found PinkFlower entity after exited Game Mode"


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(SC_Spawnables_SimpleSpawnAndDespawn)
