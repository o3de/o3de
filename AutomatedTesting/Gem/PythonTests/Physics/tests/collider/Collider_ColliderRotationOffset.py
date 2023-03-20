"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4982798
# Test Case Title : Verify that when the x,y,z values are defined in the offset, the collider frame
#                   rotates from its original orientation in the direction defined by the x,y,z units



import os
import sys


# fmt: off
class Tests:
    enter_game_mode       = ("Entered game mode",                                     "Failed to enter game mode")
    boxes_found           = ("All boxes were found",                                  "Couldn't find at least one box")
    spheres_found         = ("All spheres were found",                                "Not all the spheres could be found")
    test_completed        = ("The test completed",                                    "The test timed out")
    target_spheres_passed = ("All spheres intended to collide with Target Boxes DID", "At least one sphere intended to collide with a Target Box DID NOT")
    pass_thru_spheres_passed   = ("All spheres intended to pass through Target Boxes DID", "At least one sphere intended to pass through a Target Box DID NOT")
    exit_game_mode        = ("Exited game mode",                                      "Couldn't exit game mode")

# fmt: on


def Collider_ColliderRotationOffset():
    # type: () -> None
    """
    Summary:
    Runs an automated test to ensure that PhysXCollider rotational offsets work properly. Collisions should be
    calculated based on the location of the colliders, including their rotational offsets in x, y, and/or z

    Level Description:
    There are five classes of entities in the level: Target Spheres, Pass Spheres, Target Boxes, Pass Boxes
    and Fail Boxes. All entities have gravity disabled, are positioned above the terrain, and have the same
    collision group/layer (Default/All).

    Target Boxes: These are the actual boxes that have their collider offsets rotated by 90 degrees.
        There are three of these boxes, one for each rotation axis (rightly labeled Box_[X, Y, or Z]_Target).

    Target Spheres: These spheres are positioned near the Target Boxes' rotated collision areas. These entities are
        initialized with a velocity that will send them on course to collide with the collision geometry for the
        Target Boxes. They are appropriately labeled for which Target box they are to collide with
        Sphere_[X, Y, or Z]_Target

    Pass Thru Spheres: These spheres are positioned near the Target Boxes as well, and are also initialized with a
        velocity that will will send them towards their Target Box. The difference here is that they are aligned to
        "collide" with their Target Box's actual transform (not their rotated collider). They too are appropriately
        named Sphere_[X, Y, or Z]_Pass

    Pass Thru Boxes: Pass Boxes are positioned on the other side of the Pass Spheres from their target Box. They serve
        as a trigger to register that the Pass Sphere successfully passed through the respective Target Box's
        transform geometry. They are rightfully named Box_[X, Y, or Z]_Pass

    Fail Boxes: Fail boxes (like Pass Boxes) are positioned on the other side of their respective Target Box,
        but they are arranged opposite the Target Sphere (rather than the Pass Sphere). They act as a fail condition
        if the Target Sphere happens the pass through the Target Box's rotated collider. They are named following the
        same convention: Box_[X, Y, or Z]_Fail

    Note: All boxes are set to "kinematic" so they do not move when a collision happens.

    Expected Behavior:
    Upon entering game mode, the spheres should follow their initial velocities. The Target Spheres should collide and
    bounce off of the Target Boxes' rotated colliders, and the Pass Spheres should pass through the visible transforms
    of their Target Boxes and collide with their respective Pass Boxes.

    Test Steps:
    1) Loads the level
    2) Enters game mode
    3) Retrieve and validate entities and starting locations
    4) Wait for test to complete or time out
    5) Log the results
    6) Exit game mode and editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """




    # Internal editor imports
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr

    # ******** Global Variables ********

    # Entity data organization class
    class EntityData:
        def __init__(self, name):
            self.name = name
            self.id = None
            self.init_pos = None
            self.current_pos = None
            self.result = None

    # fmt: off
    # Establish entity sets
    target_spheres    = [EntityData("Sphere_X_Target"),    EntityData("Sphere_Y_Target"),    EntityData("Sphere_Z_Target")]
    pass_thru_spheres = [EntityData("Sphere_X_Pass_Thru"), EntityData("Sphere_Y_Pass_Thru"), EntityData("Sphere_Z_Pass_Thru")]
    target_boxes      = [EntityData("Box_X_Target"),       EntityData("Box_Y_Target"),       EntityData("Box_Z_Target")]
    pass_thru_boxes   = [EntityData("Box_X_Pass_Thru"),    EntityData("Box_Y_Pass_Thru"),    EntityData("Box_Z_Pass_Thru")]
    fail_boxes        = [EntityData("Box_X_Fail"),         EntityData("Box_Y_Fail"),         EntityData("Box_Z_Fail")]

    all_spheres = target_spheres + pass_thru_spheres
    all_boxes = pass_thru_boxes + fail_boxes + target_boxes
    all_entities = all_spheres + all_boxes

    # Possible results
    PASS_THRU =   "pass thru"
    FAIL =        "fail"
    TARGET =      "target"
    # fmt: on

    # ******** Helper Functions ********

    # Retrieve entities' IDs and initial positions.
    #   Fast Fails if there are any problems retrieving vital information
    def retrieve_entities(entity_list, test_tuple):
        # type: ([EntityData], (str, str)) -> None
        passed = True
        for entity in entity_list:
            valid = True
            entity.id = general.find_game_entity(entity.name)
            if not entity.id.IsValid():
                valid = False
                Report.info("Entity: {} could not be found".format(entity.name))
            entity.init_pos = entity.current_pos = azlmbr.components.TransformBus(
                azlmbr.bus.Event, "GetWorldTranslation", entity.id
            )

            if entity.init_pos is None or entity.init_pos.IsZero():
                valid = False
                Report.info("Entity: {}'s initial position could not be found".format(entity.name))

            if not valid:
                passed = False
                break
        Report.critical_result(test_tuple, passed)

    # Updates entities' current position
    def refresh_positions(entity_list):
        # type: ([EntityData]) -> None
        for entity in entity_list:
            entity.current_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", entity.id)

    # Checks for unexpected movement in stationary objects
    #   Fast Fails and writes to the log if there is a difference between initial position and current position
    def check_for_unexpected_movement(entity_list):
        # type: ([EntityData]) -> None
        for entity in entity_list:
            if not entity.init_pos.IsClose(entity.current_pos, CLOSE_ENOUGH_THRESHOLD):
                helper.fail_fast("{} has unexpectedly moved".format(entity.name))

    # Verifies the results for the entities passed in.
    #   Returns a count of the verified results
    def verify_results(entity_list, expected_result):
        # type: ([EntityData], str) -> int
        results_verified = 0
        for entity in entity_list:
            if entity.result == expected_result:
                results_verified += 1
            else:
                Report.info("{} had unexpected result:".format(entity.name))
                Report.info("  expected: {}".format(expected_result))
                Report.info("  found: {}".format(entity.result))
        return results_verified

    # Batch assign event handlers
    def set_handlers(entity_list, callback, event="OnCollisionBegin"):
        # type: ([EntityData], function, str) -> [handler]
        handlers = []
        for entity in entity_list:
            handler = azlmbr.physics.CollisionNotificationBusHandler()
            handler.connect(entity.id)
            handler.add_callback(event, callback)
            handlers.append(handler)
        return handlers

    # Checks to see if we are done collecting results for the test
    def done_collecting_results(entity_list, num_results):
        # type: ([EntityData], int) -> bool
        result_count = 0
        for entity in entity_list:
            if entity.result is not None:
                result_count += 1

        # When all spheres have a result we are done
        return result_count == num_results

    # Callback function to be passed to wait_for_condition
    # Updates game entities' data, checks for failures and unexpected results,
    # then returns True if we are done observing the test.
    def test_completed():
        refresh_positions(all_entities)
        # check_for_failure(all_spheres)
        check_for_unexpected_movement(all_boxes)
        return done_collecting_results(all_spheres, TOTAL_SPHERES)

    # ******** Event Handlers ********

    # General collision handler
    def on_collision_begin(collider_id, result):
        # type: (EntityId, str) -> None
        for sphere in all_spheres:
            if sphere.id.Equal(collider_id):
                Report.info("Entity: {} collided with a {} box".format(sphere.name, result))
                if result is FAIL or sphere.result is None:
                    # Set result if not set yet OR the result is a failure (failure overrides success)
                    sphere.result = result
                return
        # It wasn't a sphere that collided, something went wrong
        if collider_id.IsValid():
            entity_name = azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "GetEntityName", collider_id)
            Report.info("{} box collided with unexpected entity: {}".format(result, entity_name))

    # Fail Box collision event handler
    def on_collision_begin_fail_box(args):
        # type: ([EntityId, ...]) -> None
        on_collision_begin(args[0], FAIL)

    # Success Box Collision Event Handler
    def on_collision_begin_pass_thru_box(args):
        # type: ([EntityId, ...]) -> None
        on_collision_begin(args[0], PASS_THRU)

    # Target box collision event handler
    def on_collision_begin_target_box(args):
        # type: ([EntityId, ...]) -> None
        on_collision_begin(args[0], TARGET)

    # ******** Execution Code *********

    # Local Constants
    TIME_OUT = 2.0
    CLOSE_ENOUGH_THRESHOLD = 0.0001

    TOTAL_TARGET_SPHERES = 3
    TOTAL_PASS_THRU_SPHERES = 3
    TOTAL_SPHERES = TOTAL_TARGET_SPHERES + TOTAL_PASS_THRU_SPHERES

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "Collider_ColliderRotationOffset")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate entities
    retrieve_entities(all_boxes, Tests.boxes_found)
    retrieve_entities(all_spheres, Tests.spheres_found)

    # Assign handlers
    handlers = []
    handlers.extend(set_handlers(fail_boxes, on_collision_begin_fail_box))
    handlers.extend(set_handlers(pass_thru_boxes, on_collision_begin_pass_thru_box))
    handlers.extend(set_handlers(target_boxes, on_collision_begin_target_box))

    # 4) Wait for either time out or for the test to complete
    Report.result(Tests.test_completed, helper.wait_for_condition(test_completed, TIME_OUT))

    # 5) Log results
    # Verify entities results
    pass_thru_spheres_passed = verify_results(pass_thru_spheres, PASS_THRU)
    target_spheres_passed = verify_results(target_spheres, TARGET)

    # Report results
    Report.result(Tests.target_spheres_passed, target_spheres_passed == TOTAL_TARGET_SPHERES)
    Report.result(Tests.pass_thru_spheres_passed, pass_thru_spheres_passed == TOTAL_PASS_THRU_SPHERES)

    # Data dump at bottom of log
    Report.info("******** Collected Data *********")
    for entity in all_entities:
        Report.info("Entity: {}".format(entity.name))
        Report.info_vector3(entity.init_pos, " Initial position:")
        Report.info_vector3(entity.current_pos, " Final position:")
        Report.info(" Result: {}".format(entity.result))
        Report.info("********************************")

    # 6) Exit Game mode
    helper.exit_game_mode(Tests.exit_game_mode)

    Report.info("*** FINISHED TEST ***")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_ColliderRotationOffset)
