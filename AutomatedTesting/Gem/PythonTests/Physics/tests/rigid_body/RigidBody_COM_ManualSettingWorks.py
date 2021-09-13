"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4976210
# Test Case Title : Verify that when Compute COM is disabled, the user gets an option to add the co-ordinates of
#                       the COM and the COM gets implemented at those co-ordinates.



import os
import sys


# fmt: off
class Tests:
    enter_game_mode         = ("Entered game mode",                                       "Failed to enter game mode")
    boxes_validated         = ("All box entities were found and validated",               "Couldn't find or validate at least one box")
    test_objects_validated  = ("All test entities were found and validated",              "Not all test entities could be found and validated")
    sphere_com_set          = ("The Test Sphere's COM was successfully set",              "The Test Sphere's COM WAS NOT successfully set")
    capsule_com_set         = ("The Test Capsule's COM was successfully set",             "The Test Capsule's COM WAS NOT successfully set")
    box_com_set             = ("The Test Box's COM was successfully set",                 "The Test Box's COM WAS NOT successfully set")
    sphere_confirm          = ("The Test Sphere rolled uphill due to COM offset",         "The Test Sphere rolled downhill-- COM did not work as expected")
    capsule_confirm         = ("The Test Capsule fell against gravity due to COM offset", "The Test Capsule fell with gravity-- COM did not work as expected")
    box_confirm             = ("The Test Box fell against gravity due to COM offset",     "The Test Box fell with gravity-- COM did not work as expected")
    exit_game_mode          = ("Exited game mode",                                        "Couldn't exit game mode")
# fmt: on


def RigidBody_COM_ManualSettingWorks():
    # type: () -> None
    """
    Summary:
    Tests that when an entity has "Compute COM" disabled, a user can set a custom center of mass and that
    the set center of mass behaves as expected.

    Level Description:
    Three test objects (Box, Capsule and Sphere) are positioned on a platform that is rotated at an acute angle. Two
    plates (Pass and Fail) are perpendicular to the platform so that the Fail Plate is underneath the test objects, and
    the Pass Plate is above. Gravity is enabled for all test objects. Stationary objects (plates, platform... etc)
    have gravity disabled and have masses set for "very large numbers" (10,000 kg) to resist the test objects'
    movements. Test objects have their "compute COM" property disabled, but in the editor have a computed COM of
    (0.0, 0.0, 0.0) and gravity enabled.

    Expected Behavior:
    When game mode starts, the script manually sets each test objects center of mass in a way where that entity
    will "fall against gravity". The box and capsule should tilt against gravity into the Pass Plate, and the sphere
    should "roll up hill" to the Pass Plate.

    Test Steps:
    0) Define data and functions
    1) Open level, enter game mode
    2) Find and validate entities
    3) set and verify center of mass (COM)
    4) Wait for results or timeout
    5) Log results
    6) Close the editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    # internal editor imports


    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr
    import azlmbr.math as azmath

    # 0) Set up data and functions used in the test

    # EntityData class for storing and organizing useful test information/results
    class EntityData:
        def __init__(self, name):
            self.name = name
            self.id = None
            self.init_pos = None
            self.current_pos = None
            self.init_rot = None
            self.current_rot = None
            self.result = None
            self.fail_info = None

    # ***** Global variables ****

    # Constants
    TIME_OUT = 1.5
    CAPSULE_BOX_OFFSET = azmath.Vector3(3.0, 0.0, 0.0)
    SPHERE_OFFSET = azmath.Vector3(0.5, 0.0, 0.0)
    PASS = "pass"
    FAIL = "fail"

    # Test entities
    test_box = EntityData("Test_Box")
    test_capsule = EntityData("Test_Capsule")
    test_sphere = EntityData("Test_Sphere")
    test_entities = [test_box, test_capsule, test_sphere]

    # Plate entities
    pass_plate = EntityData("Pass_Plate")
    fail_plate = EntityData("Fail_Plate")

    # All non-moving entities
    non_movers = [EntityData("Platform"), EntityData("Pass_Buffer"), EntityData("Fail_Buffer"), pass_plate, fail_plate]

    # Full entity list
    all_entities = non_movers + test_entities

    # ******** Helper Functions ********

    # Attempts to set COM, and manually computes expected COM to verify it was applied correctly
    def set_and_validate_COM(entity, com_offset):
        # type: (EntityData, Vector3) -> None
        CLOSE_ENOUGH_THRESHOLD = 0.01
        # Set COM offset
        azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetCenterOfMassOffset", entity.id, com_offset)
        entity_world_com_pos = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetCenterOfMassWorld", entity.id)
        # Get world rotation matrix
        tm_matrix = azmath.Matrix3x3_CreateFromTransform(
            azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTM", entity.id)
        )
        # Calculate expected world COM
        world_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", entity.id)
        calculated_com = tm_matrix.MultiplyVector3(com_offset).Add(world_pos)
        # If expected COM and actual COM differ, log data collected
        if not entity_world_com_pos.IsClose(calculated_com, CLOSE_ENOUGH_THRESHOLD):
            Report.info("COM not applied to entity: {}".format(entity.name))
            Report.info_vector3(world_pos, " Entity position:")
            Report.info_vector3(com_offset, " COM offset:")
            Report.info_vector3(entity_world_com_pos, " Entity world COM:")
            Report.info_vector3(calculated_com, " Expected COM")
            return False
        return True

    # ** Entity batch operation helpers **

    # Attempts to validate entities' IDs and initial positions and initial rotations.
    #   Returns True if all entities were successfully validated.
    #   Print to the log if there are any problems retrieving vital information
    def validate_entities(entity_list):
        # type: ([EntityData]) -> int
        count = 0
        for entity in entity_list:
            valid = True
            entity.id = general.find_game_entity(entity.name)
            if not entity.id.IsValid():
                valid = False
                Report.info("Entity: {} could not be validated".format(entity.name))
            entity.init_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", entity.id)
            entity.current_pos = entity.init_pos
            if entity.init_pos is None or entity.init_pos.IsZero():
                valid = False
                Report.info("Entity: {}'s initial position could not be found".format(entity.name))
            entity.init_rot = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldRotation", entity.id)
            entity.current_rot = entity.init_rot
            if entity.init_rot is None:
                valid = False
                Report.info("Entity: {}'s initial rotation could not be found".format(entity.name))
            if valid:
                count += 1
        return count == len(entity_list)

    # Updates entities' current position
    def update_pos_and_rot(entity_list):
        # type: ([EntityData]) -> None
        for entity in entity_list:
            entity.current_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", entity.id)
            entity.current_rot = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldRotation", entity.id)

    # Checks for unexpected movement in stationary objects
    #   Prints to the log if there is a difference between initial position and current position
    def check_for_unexpected_movement(entity_list):
        # type: ([EntityData]) -> None
        CLOSE_ENOUGH_THRESHOLD = 0.1
        for entity in entity_list:
            if not entity.init_pos.IsClose(entity.current_pos, CLOSE_ENOUGH_THRESHOLD):
                entity.result = FAIL
                entity.fail_info = "Unexpected movement detected"

    # Checks if we have enough information to end the test
    def done_collecting_results(entity_list, num_results):
        # type: ([EntityData], int) -> bool
        result_count = 0
        for entity in entity_list:
            if entity.result is not None:
                result_count += 1

        # when all spheres have a result we are done
        if result_count == num_results:
            return True
        return False

    # ****** single update function to pass ******
    #        to helper.wait_for_condition()

    # Should be called every frame:
    #  Updates all entities positions and rotations
    #  Returns True if exit conditions for test are met
    #    see @done_collecting_results(..) for exit conditions
    def is_done_updating():
        # type: () -> bool
        update_pos_and_rot(all_entities)
        check_for_unexpected_movement(non_movers)
        return done_collecting_results(test_entities, len(test_entities))

    # ******** Event Handlers ********

    # Fail A Box collision event handler
    def on_collision_begin_fail(args):
        # type: ([EntityId, ...]) -> None
        collider_id = args[0]
        for entity in test_entities:
            if entity.id.Equal(collider_id) and entity.result is None:
                entity.result = FAIL
                entity.fail_info = "Collide with fail plate: COM did not apply properly"
                return

    # Pass Box Collision Event Handler
    def on_collision_begin_pass(args):
        # type: ([EntityId, ...]) -> None
        collider_id = args[0]
        for entity in test_entities:
            if entity.id.Equal(collider_id) and entity.result is None:
                Report.info("Entity: {} collided with the Pass Plate".format(entity.name))
                entity.result = PASS
                return
        # it wasn't a test entity that collided, something went wrong
        if collider_id.IsValid():
            entity_name = azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "GetEntityName", collider_id)
            Report.info("Pass Plate collided with unexpected entity: {}".format(entity_name))

    # ******** Execution Code *********

    # 1) Open level and start game mode
    helper.init_idle()
    helper.open_level("Physics", "RigidBody_COM_ManualSettingWorks")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Find and validate test entities
    Report.critical_result(Tests.test_objects_validated, validate_entities(test_entities))
    Report.critical_result(Tests.boxes_validated, validate_entities(non_movers))

    # 3) Set and verify COM offsets
    Report.critical_result(Tests.box_com_set, set_and_validate_COM(test_box, CAPSULE_BOX_OFFSET))
    Report.critical_result(Tests.capsule_com_set, set_and_validate_COM(test_capsule, CAPSULE_BOX_OFFSET))
    Report.critical_result(Tests.sphere_com_set, set_and_validate_COM(test_sphere, SPHERE_OFFSET))

    # Assign handlers
    pass_handler = azlmbr.physics.CollisionNotificationBusHandler()
    pass_handler.connect(pass_plate.id)
    pass_handler.add_callback("OnCollisionBegin", on_collision_begin_pass)

    fail_handler = azlmbr.physics.CollisionNotificationBusHandler()
    fail_handler.connect(fail_plate.id)
    fail_handler.add_callback("OnCollisionBegin", on_collision_begin_fail)

    # 4) wait for either time out or for the test to complete
    if not helper.wait_for_condition(is_done_updating, TIME_OUT):
        Report.info("The test timed out, check log for possible solutions or adjust the time out time")

    # 5) Log results

    # verify entities results
    Report.result(Tests.box_confirm, test_box.result is not None and test_box.result == PASS)
    Report.result(Tests.capsule_confirm, test_capsule.result is not None and test_capsule.result == PASS)
    Report.result(Tests.sphere_confirm, test_sphere.result is not None and test_sphere.result == PASS)

    # Data dump at bottom of log
    Report.info("******** Collected Data *********")
    for entity in all_entities:
        Report.info("Entity: {}".format(entity.name))
        Report.info_vector3(entity.init_pos, " Initial position:")
        Report.info_vector3(entity.current_pos, " Final position:")
        Report.info_vector3(entity.init_rot, " Initial rotation:")
        Report.info_vector3(entity.current_rot, " Final rotation:")
        Report.info(" Result: {}".format(entity.result))
        if entity.fail_info is not None:
            Report.info("   Fail info: {}".format(entity.fail_info))
        Report.info("********************************")

    # 6) Exit Game mode
    helper.exit_game_mode(Tests.exit_game_mode)

    Report.info("*** FINISHED TEST ***")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_COM_ManualSettingWorks)
