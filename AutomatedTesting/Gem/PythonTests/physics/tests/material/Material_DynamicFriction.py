"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C4044459
# Test Case Title : Verify the functionality of dynamic friction



# fmt: off
class Tests():
    enter_game_mode        = ("Entered game mode",                                    "Failed to enter game mode")
    find_ramp              = ("Ramp entity found",                                    "Ramp entity not found")
    find_box_zero          = ("Box entity 'zero' found",                              "Box entity 'zero' not found")
    find_box_low           = ("Box entity 'low' found",                               "Box entity 'low' not found")
    find_box_mid           = ("Box entity 'mid' found",                               "Box entity 'mid' not found")
    find_box_high          = ("Box entity 'high' found",                              "Box entity 'high' not found")
    box_at_rest_start_zero = ("Box 'zero' began test motionless",                     "Box 'zero' did not begin test motionless")
    box_at_rest_start_low  = ("Box 'low' began test motionless",                      "Box 'low' did not begin test motionless")
    box_at_rest_start_mid  = ("Box 'mid' began test motionless",                      "Box 'mid' did not begin test motionless")
    box_at_rest_start_high = ("Box 'high' began test motionless",                     "Box 'high' did not begin test motionless")
    box_was_pushed_zero    = ("Box 'zero' moved",                                     "Box 'zero' did not move before timeout")
    box_was_pushed_low     = ("Box 'low' moved",                                      "Box 'low' did not move before timeout")
    box_was_pushed_mid     = ("Box 'mid' moved",                                      "Box 'mid' did not move before timeout")
    box_was_pushed_high    = ("Box 'high' moved",                                     "Box 'high' did not move before timeout")
    box_at_rest_end_zero   = ("Box 'zero' came to rest",                              "Box 'zero' did not come to rest before timeout")
    box_at_rest_end_low    = ("Box 'low' came to rest",                               "Box 'low' did not come to rest before timeout")
    box_at_rest_end_mid    = ("Box 'mid' came to rest",                               "Box 'mid' did not come to rest before timeout")
    box_at_rest_end_high   = ("Box 'high' came to rest",                              "Box 'high' did not come to rest before timeout")
    distance_ordered       = ("Boxes with greater dynamic friction traveled shorter", "Boxes with greater dynamic friction traveled further")
    exit_game_mode         = ("Exited game mode",                                     "Couldn't exit game mode")
# fmt: on


def Material_DynamicFriction():
    """
    Summary:
    Runs an automated test to ensure that greater dynamic friction coefficient settings on a physX material results in
    rigidbody entities (with that material) that require a greater force in order to remain in motion

    Level Description:
    Four boxes sit on a horizontal 'ramp'. The boxes are identical, save for their physX material:

    A new material library was created with 4 materials and their dynamic friction coefficient:
        zero_dynamic_friction: 0.00
        low_dynamic_friction:  0.50
        mid_dynamic_friction:  1.00
        high_dynamic_friction: 1.50
    Each material is identical otherwise.

    Each box is assigned its corresponding friction material
    Each box also has a PhysX box collider with default settings
    The COM of the boxes is placed on the plane (0, 0, -0.5), so as to remove any torque moments and resulting rotations

    Expected Behavior:
    For each box, this script will apply a force impulse in the world X direction
    Boxes with greater dynamic friction coefficients should travel a shorter distance along the ramp.

    Test Steps:
    1) Open level
    2) Enter game mode
    3) Find the ramp

    For each box:
        4) Find the box
        5) Ensure the box is stationary
        6) Push the box and wait for it to come to rest

    7) Assert that greater coefficients result in a shorter distance travelled
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

    FORCE_IMPULSE = lymath.Vector3(10.0, 0.0, 0.0)
    TIMEOUT = 5

    class Box:
        def __init__(self, name, valid_test, stationary_start_test, moved_test, stationary_end_test):
            self.name = name
            self.id = general.find_game_entity(name)
            self.start_position = self.get_position()
            self.distance = 0.0
            self.valid_test = valid_test
            self.stationary_start_test = stationary_start_test
            self.moved_test = moved_test
            self.stationary_end_test = stationary_end_test

        def is_stationary(self):
            velocity = azlmbr.physics.RigidBodyRequestBus(bus.Event, "GetLinearVelocity", self.id)
            return vector_is_close_to_zero(velocity)

        def get_position(self):
            return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

    def vector_is_close_to_zero(vector):
        return abs(vector.x) <= 0.001 and abs(vector.y) <= 0.001 and abs(vector.z) <= 0.001

    def push(box):
        azlmbr.physics.RigidBodyRequestBus(bus.Event, "ApplyLinearImpulse", box.id, FORCE_IMPULSE)

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "Material_DynamicFriction")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # fmt: off
    # Set up our boxes
    box_zero = Box(
        name                  = "Zero",
        valid_test            = Tests.find_box_zero,
        stationary_start_test = Tests.box_at_rest_start_zero,
        moved_test            = Tests.box_was_pushed_zero,
        stationary_end_test   = Tests.box_at_rest_end_zero
    )
    box_low = Box(
        name                  = "Low",
        valid_test            = Tests.find_box_low,
        stationary_start_test = Tests.box_at_rest_start_low,
        moved_test            = Tests.box_was_pushed_low,
        stationary_end_test   = Tests.box_at_rest_end_low
    )
    box_mid = Box(
        name                  = "Mid",
        valid_test            = Tests.find_box_mid,
        stationary_start_test = Tests.box_at_rest_start_mid,
        moved_test            = Tests.box_was_pushed_mid,
        stationary_end_test   = Tests.box_at_rest_end_mid
    )
    box_high = Box(
        name                  = "High",
        valid_test            = Tests.find_box_high,
        stationary_start_test = Tests.box_at_rest_start_high,
        moved_test            = Tests.box_was_pushed_high,
        stationary_end_test   = Tests.box_at_rest_end_high
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
        Report.result(box.stationary_start_test, box.is_stationary())
        # 6) Push the box
        push(box)
        Report.result(box.moved_test, helper.wait_for_condition(lambda: not box.is_stationary(), TIMEOUT))
        Report.result(box.stationary_end_test, helper.wait_for_condition(lambda: box.is_stationary(), TIMEOUT))

        end_position = box.get_position()
        box.distance = end_position.GetDistance(box.start_position)
        Report.info("Box {} travelled {:.3f} meters".format(box.name, box.distance))

    # 7) Assert that greater coefficients result in shorter travelled distance
    distance_ordered = box_high.distance < box_mid.distance < box_low.distance < box_zero.distance
    Report.result(Tests.distance_ordered, distance_ordered)

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_DynamicFriction)
