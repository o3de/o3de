"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def Lua_Spawnables_MultipleSpawnsFromSingleTicket():

    import azlmbr.entity as entity
    import azlmbr.bus as bus
    import azlmbr.legacy.general as general

    from editor_python_test_tools.utils import TestHelper as helper

    def validate_entities_in_edit_mode(condition):
        search_filter = entity.SearchFilter()
        search_filter.names = ["Lua_Spawner"]
        spawner_entity = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)[0]
        search_filter.names = ["PinkFlower"]
        flower_entity = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
        assert spawner_entity is not None, f"Failed to find Spawner entity {condition}"
        assert len(flower_entity) == 0, f"Unexpectedly found PinkFlower entity {condition}"

    helper.init_idle()
    helper.open_level("Prefab", "Lua_Spawnables_MultipleSpawnsFromSingleTicket")

    # Search for expected entities in edit mode
    validate_entities_in_edit_mode("before entering Game Mode")

    # Enter Game Mode and allow Lua logic to run. Script will execute the "quit" command and exit Game Mode if all
    # expected entities are found at runtime
    general.enter_game_mode()

    # Wait for Game Mode exit via Lua script and verify despawn of all SpawnablesTestEntity instances
    game_mode_exited = helper.wait_for_condition(lambda: not general.is_in_game_mode(), 5.0)
    assert game_mode_exited, "Lua failed to exit Game Mode"
    validate_entities_in_edit_mode("after exiting Game Mode")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Lua_Spawnables_MultipleSpawnsFromSingleTicket)
