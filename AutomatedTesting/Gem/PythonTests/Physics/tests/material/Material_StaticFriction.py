"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C4044460
# Test Case Title : Verify the functionality of static friction



# fmt: off
class Tests():
    enter_game_mode       = ("Entered game mode",                                            "Failed to enter game mode")
    find_ramp             = ("Ramp entity found",                                            "Ramp entity not found")
    find_box_zero         = ("Box entity 'zero' found",                                      "Box entity 'zero' not found")
    find_box_low          = ("Box entity 'low' found",                                       "Box entity 'low' not found")
    find_box_mid          = ("Box entity 'mid' found",                                       "Box entity 'mid' not found")
    find_box_high         = ("Box entity 'high' found",                                      "Box entity 'high' not found")
    box_at_rest_zero      = ("Box 'zero' began test motionless",                             "Box 'zero' did not begin test motionless")
    box_at_rest_low       = ("Box 'low' began test motionless",                              "Box 'low' did not begin test motionless")
    box_at_rest_mid       = ("Box 'mid' began test motionless",                              "Box 'mid' did not begin test motionless")
    box_at_rest_high      = ("Box 'high' began test motionless",                             "Box 'high' did not begin test motionless")
    box_was_pushed_zero   = ("Box 'zero' moved",                                             "Box 'zero' did not move before timeout")
    box_was_pushed_low    = ("Box 'low' moved",                                              "Box 'low' did not move before timeout")
    box_was_pushed_mid    = ("Box 'mid' moved",                                              "Box 'mid' did not move before timeout")
    box_was_pushed_high   = ("Box 'high' moved",                                             "Box 'high' did not move before timeout")
    force_impulse_ordered = ("Boxes with greater static friction required greater impulses", "Boxes with greater static friction did not require greater impulses")
    exit_game_mode        = ("Exited game mode",                                             "Couldn't exit game mode")
# fmt: on


def Material_StaticFriction():
    """
    Summary:
    Runs an automated test to ensure that greater static friction coefficient settings on a physX material results in
    rigidbodys (with that material) requiring a greater force in order to be set into motion

    Level Description:
    Four boxes sit on a horizontal 'ramp'. The boxes are identical, save for their physX material.

    A new material library was created with 4 materials and their static friction coefficient:
        zero_static_friction: 0.00
        low_static_friction:  0.50
        mid_static_friction:  1.00
        high_static_friction: 1.50
    Each material is identical otherwise
    Each box is assigned its corresponding friction material, the ramp is assigned low_static_friction
    The COM of the boxes is placed on the plane (0, 0, -0.5), so as to remove any torque moments and resulting rotations

    Expected Behavior:
    For each box, this script will apply a force impulse in the world X direction (starting at magnitude 0.0).
    Every frame, it checks if the box moved:
        If it didn't, we increase the magnitude slightly and try again
        If it did, the box retains the magnitude required to move it, and we move to the next box.

    Boxes with greater static friction coefficients should require greater forces in order to set them in motion.

    Test Steps:
    1) Open level
    2) Enter game mode
    3) Find the ramp

    For each box:
        4) Find the box
        5) Ensure the box is stationary
        6) Push the box until it moves

    7) Assert that greater coefficients result in greater required force impulses
    8) Exit game mode
    9) Close the editor

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

    import azlmbr
    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.math as lymath

    FORCE_IMPULSE_INCREMENT = 0.005  # How much we increase the force every frame
    MIN_MOVE_DISTANCE = 0.02  # Distance magnitude that a box must travel in order to be considered moved
    STATIONARY_TOLERANCE = 0.0001  # Boxes must have velocities under this magnitude in order to be stationary
    TIMEOUT = 10

    class Box:
        def __init__(self, name, valid_test, stationary_test, moved_test):
            self.name = name
            self.id = general.find_game_entity(name)
            self.start_position = self.get_position()
            self.force_impulse = 0.0
            self.valid_test = valid_test
            self.stationary_test = stationary_test
            self.moved_test = moved_test

        def is_stationary(self):
            velocity = azlmbr.physics.RigidBodyRequestBus(bus.Event, "GetLinearVelocity", self.id)
            return vector_close_to_zero(velocity, STATIONARY_TOLERANCE)

        def get_position(self):
            return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

    def vector_close_to_zero(vector, tolerance):
        return abs(vector.x) <= tolerance and abs(vector.y) <= tolerance and abs(vector.z) <= tolerance

    def push(box):
        delta = box.start_position.Subtract(box.get_position())
        if vector_close_to_zero(delta, MIN_MOVE_DISTANCE):
            box.force_impulse += FORCE_IMPULSE_INCREMENT
            impulse_vector = lymath.Vector3(box.force_impulse, 0.0, 0.0)
            azlmbr.physics.RigidBodyRequestBus(bus.Event, "ApplyLinearImpulse", box.id, impulse_vector)
            return False
        else:
            Report.info("Box {} required force was {:.3f}".format(box.name, box.force_impulse))
            return True

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "Material_StaticFriction")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # fmt: off
    # Set up our boxes
    box_zero = Box(
        name            = "Zero",
        valid_test      = Tests.find_box_zero,
        stationary_test = Tests.box_at_rest_zero,
        moved_test      = Tests.box_was_pushed_zero
    )
    box_low = Box(
        name            = "Low",
        valid_test      = Tests.find_box_low,
        stationary_test = Tests.box_at_rest_low,
        moved_test      = Tests.box_was_pushed_low
    )
    box_mid = Box(
        name            = "Mid",
        valid_test      = Tests.find_box_mid,
        stationary_test = Tests.box_at_rest_mid,
        moved_test      = Tests.box_was_pushed_mid
    )
    box_high = Box(
        name            = "High",
        valid_test      = Tests.find_box_high,
        stationary_test = Tests.box_at_rest_high,
        moved_test      = Tests.box_was_pushed_high
    )
    all_boxes = (box_zero, box_low, box_mid, box_high)
    # fmt: on

    # 3) Find the ramp
    ramp_id = general.find_game_entity("Ramp")
    Report.critical_result(Tests.find_ramp, ramp_id.IsValid())

    for box in all_boxes:
        Report.info("********Pushing Box {}********".format(box.name))
        # 4) Find the box
        Report.critical_result(box.valid_test, box.id.IsValid())
        # 5) Ensure the box is stationary
        Report.result(box.stationary_test, box.is_stationary())
        # 6) Push the box until it moves
        Report.critical_result(box.moved_test, helper.wait_for_condition(lambda: push(box), TIMEOUT))

    # 7) Assert that greater coefficients result in greater required force impulses
    ordered_impulses = box_high.force_impulse > box_mid.force_impulse > box_low.force_impulse > box_zero.force_impulse
    Report.result(Tests.force_impulse_ordered, ordered_impulses)

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_StaticFriction)
