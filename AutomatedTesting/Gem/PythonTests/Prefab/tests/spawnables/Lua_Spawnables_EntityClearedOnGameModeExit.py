"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def Lua_Spawnables_EntityClearedOnGameModeExit():

    import math

    import azlmbr.entity as entity
    import azlmbr.bus as bus
    import azlmbr.math as azmath
    import azlmbr.legacy.general as general

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import TestHelper as helper
    from Prefab.tests.PrefabTestUtils import validate_spawned_entity_transform

    def validate_entities_in_edit_mode(condition):
        search_filter = entity.SearchFilter()
        search_filter.names = ["Lua_Spawner"]
        spawner_entity = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)[0]
        search_filter.names = ["PinkFlower"]
        flower_entity = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
        assert spawner_entity is not None, f"Failed to find Spawner entity {condition}"
        assert len(flower_entity) == 0, f"Unexpectedly found PinkFlower entity {condition}"

    helper.init_idle()
    helper.open_level("Prefab", "Lua_Spawnables_EntityClearedOnGameModeExit")

    # Search for expected entities in edit mode
    validate_entities_in_edit_mode("before entering Game Mode")

    # Enter Game Mode and search for expected entities
    general.enter_game_mode()
    helper.wait_for_condition(lambda: EditorEntity(general.find_game_entity("Lua_Spawner")) is not None, 3.0)
    spawner_entity = EditorEntity(general.find_game_entity("Lua_Spawner"))
    helper.wait_for_condition(lambda: EditorEntity(general.find_game_entity("PinkFlower")) is not None, 3.0)
    flower_entity = EditorEntity(general.find_game_entity("PinkFlower"))
    assert spawner_entity, "Failed to find Spawner entity at runtime"
    assert flower_entity, "Failed to find PinkFlower entity at runtime"

    # Verify spawned entity is spawned with the correct position, rotation, and scale values
    expected_spawned_entity_position = azmath.Vector3(0.0, 0.0, 0.0)
    expected_spawned_entity_rotation = azmath.Vector3(0.0, 0.0, math.radians(0.0))
    expected_spawned_entity_scale = 1.0
    validate_spawned_entity_transform(flower_entity, expected_spawned_entity_position, expected_spawned_entity_rotation,
                                      expected_spawned_entity_scale)

    # Exit Game Mode and verify despawn of PinkFlower.spawnable
    general.exit_game_mode()
    game_mode_exited = helper.wait_for_condition(lambda: not general.is_in_game_mode(), 5.0)
    assert game_mode_exited, "Failed to exit Game Mode"
    validate_entities_in_edit_mode("after exiting Game Mode")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Lua_Spawnables_EntityClearedOnGameModeExit)
