"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C3510644
# Test Case Title : Check that the collision layer and collision group of the terrain can be changed
#     and the collision behavior of the terrain changes accordingly


# fmt: off
class Tests:
    enter_game_mode                       = ("Entered game mode",                     "Failed to enter game mode")
    box_1_a_valid                         = ("Box 1 A has been validated",            "Box 1 A COULD NOT be validated")
    box_2_a_valid                         = ("Box 2 A has been validated",            "Box 2 A COULD NOT be validated")
    terrain_a_valid                       = ("Terrain A has been validated",          "Terrain A COULD NOT be validated")
    box_1_b_valid                         = ("Box 1 B has been validated",            "Box 1 B COULD NOT be validated")
    box_2_b_valid                         = ("Box 2 B has been validated",            "Box 2 B COULD NOT be validated")
    terrain_b_valid                       = ("Terrain B has been validated",          "Terrain B COULD NOT be validated")
    box_1_a_pos_found                     = ("Box 1 A position found",                "Box 1 A position NOT found")
    box_2_a_pos_found                     = ("Box 2 A position found",                "Box 2 A position NOT found")
    terrain_a_pos_found                   = ("Terrain A position found",              "Terrain A position NOT found")
    box_1_b_pos_found                     = ("Box 1 B position found",                "Box 1 B position NOT found")
    box_2_b_pos_found                     = ("Box 2 B position found",                "Box 2 B position NOT found")
    terrain_b_pos_found                   = ("Terrain B position found",              "Terrain B position NOT found")
    box_1_a_did_collide_with_terrain      = ("Box 1 A did collide with terrain",      "Box 1 A DID NOT collide with terrain")
    box_1_a_did_not_pass_through_terrain  = ("Box 1 A did not fall past the terrain", "Box 1 A DID fall past the terrain")
    box_2_a_did_not_collide_with_terrain  = ("Box 2 A did not collide with terrain", "Box 2 A DID collide with terrain")
    box_2_a_did_pass_through_terrain      = ("Box 2 A did fall past the terrain",     "Box 2 A DID NOT fall past the terrain")
    box_1_b_did_not_collide_with_terrain  = ("Box 1 B did not collide with terrain",  "Box 1 B DID collide with terrain")
    box_1_b_did_pass_through_terrain      = ("Box 1 B did fall past the terrain",     "Box 1 B DID NOT fall past the terrain")
    box_2_b_did_collide_with_terrain      = ("Box 2 B did collide with terrain",      "Box 2 B DID NOT collide with terrain")
    box_2_b_did_not_pass_through_terrain  = ("Box 2 B did not fall past the terrain", "Box 2 B DID fall past the terrain")
    exit_game_mode                        = ("Exited game mode",                      "Couldn't exit game mode")
# fmt: on


def Collider_CollisionGroupsWorkflow():
    # type: () -> None
    """
    Summary:
    Runs an automated test to ensure PhysX collision groups dictate whether collisions happen or not.
    The test has two phases (A and B) for testing collision groups under different circumstances. Phase A
    is run first and upon success Phase B starts.

    Level Description:
    Entities can be divided into 2 groups for the two phases, A and B. Each phase has identical entities with exception
    to Terrain, where Terrain_A has a collision group/layer set for demo_group1/demo1 and Terrain_B has a collision
    group/layer set for demo_group2/demo2.

    Each Phase has two boxes, Box_1 and Box_2, where each box has it's collision group/layer set to it's number
    (1 or 2). Each box is positioned just above the Terrain with gravity enabled.

    All entities for Phase B are deactivated by default. If Phase A is setup and executed successfully it's
    entities are deactivated and Phase B's entities are activated and validated before running the Phase B test.

    Expected behavior:
    When Phase A starts, it's two boxes should fall toward the terrain. Once the boxes' behavior is validated the
    entities from Phase A are deactivated and Phase B's entities are activated. Like in Phase A, the boxes in Phase B
    should fall towards the terrain. If all goes as expected Box_1_A and Box_2_B should collide with teh terrain, and
    Box_2A and Box_1_B should fall through the terrain.

    Test Steps:
    0) [Define helper classes and functions]
    1) Load the level
    2) Enter game mode
    3) Retrieve and validate entities
    4) Phase A
        a) set up
        b) execute test
        c) log results (deactivate Phase A entities)
    5) Phase B
        a) set up (activate Phase B entities)
        b) execute test
        c) log results
    6) close editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.
    - The level for this test uses two PhysX Terrains and must be run with cmdline argument "-autotest_mode"
            to suppress the warning for having multiple terrains.

    :return: None
    """

    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr

    # ******* Helper Classes ********

    # Phase A's test results
    class PhaseATestData:
        total_results = 2
        box_1_collided = False
        box_1_fell_through = True
        box_2_collided = False
        box_2_fell_through = False
        box_1 = None
        box_2 = None
        terrain = None
        box_1_pos = None
        box_2_pos = None
        terrain_pos = None

        @staticmethod
        # Quick check for validating results for Phase A
        def valid():
            return (
                PhaseATestData.box_1_collided
                and PhaseATestData.box_2_fell_through
                and not PhaseATestData.box_1_fell_through
                and not PhaseATestData.box_2_collided
            )

    # Phase B's test results
    class PhaseBTestData:
        total_results = 2
        box_1_collided = False
        box_1_fell_through = False
        box_2_collided = False
        box_2_fell_through = True
        box_1 = None
        box_2 = None
        terrain = None
        box_1_pos = None
        box_2_pos = None
        terrain_pos = None

        @staticmethod
        # Quick check for validating results for Phase B
        def valid():
            return (
                not PhaseBTestData.box_1_collided
                and not PhaseBTestData.box_2_fell_through
                and PhaseBTestData.box_1_fell_through
                and PhaseBTestData.box_2_collided
            )

    # **** Helper Functions ****

    # ** Validation helpers **

    # Attempts to validate an entity based on the name parameter
    def validate_entity(entity_name, msg_tuple):
        # type: (str, (str, str)) -> EntityId
        entity_id = general.find_game_entity(entity_name)
        Report.critical_result(msg_tuple, entity_id.IsValid())
        return entity_id

    # Attempts to retrieve an entity's initial position and logs result
    def validate_initial_position(entity_id, msg_tuple):
        # type: (EntityId, (str, str)) -> azlmbr.math.Vector3
        # Attempts to validate and return the entity's initial position.
        # logs the result to Report.result() using the tuple parameter

        pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", entity_id)
        valid = not (pos is None or pos.IsZero())
        entity_name = azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "GetEntityName", entity_id)
        Report.critical_result(msg_tuple, valid)
        Report.info_vector3(pos, "{} initial position:".format(entity_name))
        return pos

    # ** Phase completion checks checks **

    # Checks if we are done collecting data for phase A
    def done_collecting_results_a():
        # type: () -> bool

        # Update positions
        PhaseATestData.box_1_pos = azlmbr.components.TransformBus(
            azlmbr.bus.Event, "GetWorldTranslation", PhaseATestData.box_1
        )
        PhaseATestData.box_2_pos = azlmbr.components.TransformBus(
            azlmbr.bus.Event, "GetWorldTranslation", PhaseATestData.box_2
        )

        # Check for boxes to fall through terrain
        if PhaseATestData.box_1_pos.z < PhaseATestData.terrain_pos.z:
            PhaseATestData.box_1_fell_through = True
        else:
            PhaseATestData.box_1_fell_through = False

        if PhaseATestData.box_2_pos.z < PhaseATestData.terrain_pos.z:
            PhaseATestData.box_2_fell_through = True
        else:
            PhaseATestData.box_2_fell_through = False

        results = 0
        if PhaseATestData.box_1_collided or PhaseATestData.box_1_fell_through:
            results += 1
        if PhaseATestData.box_2_collided or PhaseATestData.box_2_fell_through:
            results += 1
        return results == PhaseATestData.total_results

    # Checks if we are done collecting data for phase B
    def done_collecting_results_b():
        # type: () -> bool

        # Update positions
        PhaseBTestData.box_1_pos = azlmbr.components.TransformBus(
            azlmbr.bus.Event, "GetWorldTranslation", PhaseBTestData.box_1
        )
        PhaseBTestData.box_2_pos = azlmbr.components.TransformBus(
            azlmbr.bus.Event, "GetWorldTranslation", PhaseBTestData.box_2
        )

        # Check for boxes to fall through terrain
        if PhaseBTestData.box_1_pos.z < PhaseBTestData.terrain_pos.z:
            PhaseBTestData.box_1_fell_through = True
        else:
            PhaseBTestData.box_1_fell_through = False

        if PhaseBTestData.box_2_pos.z < PhaseBTestData.terrain_pos.z:
            PhaseBTestData.box_2_fell_through = True
        else:
            PhaseBTestData.box_2_fell_through = False

        results = 0
        if PhaseBTestData.box_1_collided or PhaseBTestData.box_1_fell_through:
            results += 1
        if PhaseBTestData.box_2_collided or PhaseBTestData.box_2_fell_through:
            results += 1
        return results == PhaseBTestData.total_results

    # **** Event Handlers ****

    # Collision even handler for Phase A
    def on_collision_begin_a(args):
        # type: ([EntityId]) -> None
        collider_id = args[0]
        if (not PhaseATestData.box_1_collided) and PhaseATestData.box_1.Equal(collider_id):
            Report.info("Box_1_A / Terrain_A collision detected")
            PhaseATestData.box_1_collided = True
        if (not PhaseATestData.box_2_collided) and PhaseATestData.box_2.Equal(collider_id):
            Report.info("Box_2_A / Terrain_A collision detected")
            PhaseATestData.box_2_collided = True

    # Collision event handler for Phase B
    def on_collision_begin_b(args):
        # type: ([EntityId]) -> None
        collider_id = args[0]
        if (not PhaseBTestData.box_1_collided) and PhaseBTestData.box_1.Equal(collider_id):
            Report.info("Box_1_B / Terrain_B collision detected")
            PhaseBTestData.box_1_collided = True
        if (not PhaseBTestData.box_2_collided) and PhaseBTestData.box_2.Equal(collider_id):
            Report.info("Box_2_B / Terrain_B collision detected")
            PhaseBTestData.box_2_collided = True

    TIME_OUT = 1.5

    # 1) Open level
    helper.init_idle()
    helper.open_level("Physics", "Collider_CollisionGroupsWorkflow")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate entities
    PhaseATestData.box_1 = validate_entity("Box_1_A", Tests.box_1_a_valid)
    PhaseATestData.box_2 = validate_entity("Box_2_A", Tests.box_2_a_valid)
    PhaseATestData.terrain = validate_entity("Terrain_Entity_A", Tests.terrain_a_valid)
    PhaseBTestData.box_1 = validate_entity("Box_1_B", Tests.box_1_b_valid)
    PhaseBTestData.box_2 = validate_entity("Box_2_B", Tests.box_2_b_valid)
    PhaseBTestData.terrain = validate_entity("Terrain_Entity_B", Tests.terrain_b_valid)

    # Make sure Phase B objects are disabled
    azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "DeactivateGameEntity", PhaseBTestData.box_1)
    azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "DeactivateGameEntity", PhaseBTestData.box_2)
    azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "DeactivateGameEntity", PhaseBTestData.terrain)

    # 4) *********** Phase A *****************

    # 4.a) ** Set Up **

    Report.info(" **** Beginning Phase A **** ")

    # Locate Phase A entities
    PhaseATestData.box_1_pos = validate_initial_position(PhaseATestData.box_1, Tests.box_1_a_pos_found)
    PhaseATestData.box_2_pos = validate_initial_position(PhaseATestData.box_2, Tests.box_2_a_pos_found)
    PhaseATestData.terrain_pos = validate_initial_position(PhaseATestData.terrain, Tests.terrain_a_pos_found)

    # Assign Phase A event handler
    handler_a = azlmbr.physics.CollisionNotificationBusHandler()
    handler_a.connect(PhaseATestData.terrain)
    handler_a.add_callback("OnCollisionBegin", on_collision_begin_a)

    # 4.b) Execute Phase A
    if not helper.wait_for_condition(done_collecting_results_a, TIME_OUT):
        Report.info("Phase A timed out: make sure the level is set up properly or adjust time out threshold")

    # 4.c) Log results for Phase A
    Report.result(Tests.box_1_a_did_collide_with_terrain, PhaseATestData.box_1_collided)
    Report.result(Tests.box_1_a_did_not_pass_through_terrain, not PhaseATestData.box_1_fell_through)
    Report.info_vector3(PhaseATestData.box_1_pos, "Box_1_A's final position:")

    Report.result(Tests.box_2_a_did_pass_through_terrain, PhaseATestData.box_2_fell_through)
    Report.result(Tests.box_2_a_did_not_collide_with_terrain, not PhaseATestData.box_2_collided)
    Report.info_vector3(PhaseATestData.box_2_pos, "Box_2_A's final position:")

    if not PhaseATestData.valid():
        Report.info("Phase A failed test")

    # Deactivate entities for Phase A
    azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "DeactivateGameEntity", PhaseATestData.box_1)
    azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "DeactivateGameEntity", PhaseATestData.box_2)
    azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "DeactivateGameEntity", PhaseATestData.terrain)

    # 5) *********** Phase B *****************

    # 5.a) ** Set Up **
    Report.info(" *** Beginning Phase B *** ")
    # Activate entities for Phase B
    azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", PhaseBTestData.box_1)
    azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", PhaseBTestData.box_2)
    azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", PhaseBTestData.terrain)

    # Initialize positions for Phase B
    PhaseBTestData.box_1_pos = validate_initial_position(PhaseBTestData.box_1, Tests.box_1_b_pos_found)
    PhaseBTestData.box_2_pos = validate_initial_position(PhaseBTestData.box_2, Tests.box_2_b_pos_found)
    PhaseBTestData.terrain_pos = validate_initial_position(PhaseBTestData.terrain, Tests.terrain_b_pos_found)

    # Assign Phase B event handler
    handler_b = azlmbr.physics.CollisionNotificationBusHandler()
    handler_b.connect(PhaseBTestData.terrain)
    handler_b.add_callback("OnCollisionBegin", on_collision_begin_b)

    # 5.b) Execute Phase B
    if not helper.wait_for_condition(done_collecting_results_b, TIME_OUT):
        Report.info("Phase B timed out: make sure the level is set up properly or adjust time out threshold")

    # 5.c) Log results for Phase B
    Report.result(Tests.box_1_b_did_not_collide_with_terrain, not PhaseBTestData.box_1_collided)
    Report.result(Tests.box_1_b_did_pass_through_terrain, PhaseBTestData.box_1_fell_through)
    Report.info_vector3(PhaseBTestData.box_1_pos, "Box_1_B's final position:")

    Report.result(Tests.box_2_b_did_not_pass_through_terrain, not PhaseBTestData.box_2_fell_through)
    Report.result(Tests.box_2_b_did_collide_with_terrain, PhaseBTestData.box_2_collided)
    Report.info_vector3(PhaseBTestData.box_2_pos, "Box_2_B's final position:")

    if not PhaseBTestData.valid():
        Report.info("Phase B failed test")

    # 6) Exit Game mode
    helper.exit_game_mode(Tests.exit_game_mode)
    Report.info(" **** TEST FINISHED ****")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_CollisionGroupsWorkflow)
