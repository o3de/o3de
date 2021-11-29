"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C6131473
# Test Case Title : Verify a static slice is not spawned automatically everytime a dynamic slice with
# PhysX Components is spawned



# fmt: off
class Tests():
    enter_game_mode      = ("Entered game mode",              "Failed to enter game mode")
    find_entity          = ("Entities are found",             "Entities are not found")
    dynamic_slice_entity = ("Dynamic slice entity not found", "Dynamic slice entity found") 
    exit_game_mode       = ("Exited game mode",               "Couldn't exit game mode")
# fmt: on


def Physics_DynamicSliceWithPhysNotSpawnsStaticSlice():

    """
    Summary:
    Verify a static slice is not spawned automatically everytime a dynamic slice with PhysX Components is spawned

    Level Description:
    Spawner (entity) - Entity with a spawner component with a dynamic slice attached to it.
                       "Spawn on activate" is enabled for the spawner component.
                       The Spawner is attached to a dynamic slice "RigidBody" with components
                       PhysX Rigid body,
                       PhysX Collider with shape as Sphere and radius as 1.0,
                       Rendering mesh with mesh as primitive_sphere

    Expected Behavior:
    We are checking if the dynamic slice "RigidBody" is present in the current level.
    The test fails if the dynamic slice is found in the level.

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Retrieve and validate entities
     4) Verify if the dynamic slice entity exists in the current level
     5) Exit game mode
     6) Close the editor


    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr.entity as entity

    # Constants
    DYNAMIC_SLICE_NAME = "RigidBody"

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "Physics_DynamicSliceWithPhysNotSpawnsStaticSlice")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate entities
    spawner_id = general.find_game_entity("SpawnerEntity")
    Report.result(Tests.find_entity, spawner_id.IsValid())

    # 4) Verify if the dynamic slice entity exists in the current level
    # Get all the entity ids in the level
    searchFilter = entity.SearchFilter()
    searchFilter.Names = ["*"]
    entity_ids = entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", searchFilter)
    entity_names = [
        azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "GetEntityName", entity_id)
        for entity_id in entity_ids
    ]
    # check if the dynamic slice ("RigidBody") exists in the list if entities
    Report.result(Tests.dynamic_slice_entity, DYNAMIC_SLICE_NAME not in entity_names)

    # 5) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Physics_DynamicSliceWithPhysNotSpawnsStaticSlice)
