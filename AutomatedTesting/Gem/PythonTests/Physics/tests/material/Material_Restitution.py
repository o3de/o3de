"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C4044461
# Test Case Title : Verify the functionality of restitution



# fmt: off
class Tests():
    enter_game_mode         = ("Entered game mode",                                    "Failed to enter game mode")
    find_ramp               = ("Ramp entity found",                                    "Ramp entity not found")
    find_box_zero           = ("Box entity 'zero' found",                              "Box entity 'zero' not found")
    find_box_low            = ("Box entity 'low' found",                               "Box entity 'low' not found")
    find_box_mid            = ("Box entity 'mid' found",                               "Box entity 'mid' not found")
    find_box_high           = ("Box entity 'high' found",                              "Box entity 'high' not found")
    box_fell_zero           = ("Box 'zero' fell",                                      "Box 'zero' did not fall")
    box_fell_low            = ("Box 'low' fell",                                       "Box 'low' did not fall")
    box_fell_mid            = ("Box 'mid' fell",                                       "Box 'mid' did not fall")
    box_fell_high           = ("Box 'high' fell",                                      "Box 'high' did not fall")
    box_hit_ramp_zero       = ("Box 'zero' hit the ramp",                              "Box 'zero' did not hit the ramp before timeout")
    box_hit_ramp_low        = ("Box 'low' hit the ramp",                               "Box 'low' did not hit the ramp before timeout")
    box_hit_ramp_mid        = ("Box 'mid' hit the ramp",                               "Box 'mid' did not hit the ramp before timeout")
    box_hit_ramp_high       = ("Box 'high' hit the ramp",                              "Box 'high' did not hit the ramp before timeout")
    box_peaked_zero         = ("Box 'zero' reached its max height",                    "Box 'zero' did not reach max height before timeout")
    box_peaked_low          = ("Box 'low' reached its max height",                     "Box 'low' did not reach max height before timeout")
    box_peaked_mid          = ("Box 'mid' reached its max height",                     "Box 'mid' did not reach max height before timeout")
    box_peaked_high         = ("Box 'high' reached its max height",                    "Box 'high' did not reach max height before timeout")
    box_zero_did_not_bounce = ("Box 'zero' did not bounce",                            "Box 'zero' bounced - this should not happen")
    bounce_height_ordered   = ("Boxes with greater restitution values bounced higher", "Boxes with greater restitution values did not bounce higher")
    exit_game_mode          = ("Exited game mode",                                     "Couldn't exit game mode")
# fmt: on


def Material_Restitution():
    """
    Summary:
    Runs an automated test to ensure that greater restitution coefficient settings on a physX material results in
    rigid bodies (with that material) that bounce higher

    Level Description:
    Four boxes sit above a horizontal 'ramp'. Gravity on each rigid body component is set to disabled.
    The boxes are identical, save for their physX material.

    A new material library was created with 4 materials and their restitution coefficient:
        zero_restitution: 0.00
        low_restitution:  0.30
        mid_restitution:  0.60
        high_restitution: 1.00
    Each material is identical otherwise
    Each box is assigned its corresponding physX material

    Expected Behavior:
    For each box, this script will enable gravity, then wait for the box to collide with the ramp.
    It then measures the height of the bounce relative to when it first came in contact with the ramp.
    When the z component of the box's velocity reaches zero (or below zero), the box latches its bounce height.
    The box is then frozen in place and the steps run for the next box in the list.

    Boxes with greater restitution values should retain more energy between collisions, therefore bouncing higher

    Test Steps:
    1) Open level
    2) Enter game mode
    3) Find the ramp

    For each box:
        4) Find the box
        5) Drop the box
        6) Ensure the box collides with the ramp
        7) Ensure the box reaches its peak height

    8) Special case: assert that a box with zero restitution does not bounce
    9) Assert that greater restitution coefficients result in higher bounces
    10) Exit game mode
    11) Close the editor

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

    ZERO_RESTITUTION_BOUNCE_TOLERANCE = 0.001
    TIMEOUT = 5
    FALLING_TIMEOUT = 0.1

    class Box:
        def __init__(self, name, valid_test, fell_test, hit_ramp_test, peaked_test):
            self.name = name
            self.id = general.find_game_entity(name)
            self.hit_ramp = False
            self.hit_ramp_position = None
            self.bounce_height = 0.0
            self.valid_test = valid_test
            self.fell_test = fell_test
            self.hit_ramp_test = hit_ramp_test
            self.peaked_test = peaked_test
            self.set_gravity_enabled(False)

        def get_position(self):
            return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

        def get_velocity(self):
            return azlmbr.physics.RigidBodyRequestBus(bus.Event, "GetLinearVelocity", self.id)

        def set_velocity(self, value):
            return azlmbr.physics.RigidBodyRequestBus(bus.Event, "SetLinearVelocity", self.id, value)

        def set_gravity_enabled(self, value):
            azlmbr.physics.RigidBodyRequestBus(bus.Event, "SetGravityEnabled", self.id, value)

    def on_collision_begin(args):
        other_id = args[0]
        for box in all_boxes:
            if box.id.Equal(other_id):
                box.hit_ramp_position = box.get_position()
                box.hit_ramp = True

    def reached_max_height(box):
        current_position = box.get_position()
        current_height = current_position.z - box.hit_ramp_position.z
        current_linear_velocity = box.get_velocity()
        if current_linear_velocity.z > 0.0:
            box.bounce_height = current_height
            return False
        else:
            Report.info("Box {} reached {:.3f}M high".format(box.name, box.bounce_height))
            return True

    def is_falling(box):
        return box.get_velocity().z < 0.0

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "Material_Restitution")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # fmt: off
    # Set up our boxes
    box_zero = Box(
        name          = "Zero",
        valid_test    = Tests.find_box_zero,
        fell_test     = Tests.box_fell_zero,
        hit_ramp_test = Tests.box_hit_ramp_zero,
        peaked_test   = Tests.box_peaked_zero
    )
    box_low = Box(
        name          = "Low",
        valid_test    = Tests.find_box_low,
        fell_test     = Tests.box_fell_low,
        hit_ramp_test = Tests.box_hit_ramp_low,
        peaked_test   = Tests.box_peaked_low
    )
    box_mid = Box(
        name          = "Mid",
        valid_test    = Tests.find_box_mid,
        fell_test     = Tests.box_fell_mid,
        hit_ramp_test = Tests.box_hit_ramp_mid,
        peaked_test   = Tests.box_peaked_mid
    )
    box_high = Box(
        name          = "High",
        valid_test    = Tests.find_box_high,
        fell_test     = Tests.box_fell_high,
        hit_ramp_test = Tests.box_hit_ramp_high,
        peaked_test   = Tests.box_peaked_high
    )
    all_boxes = (box_zero, box_low, box_mid, box_high)
    # fmt:on

    # 3) Find the ramp
    ramp_id = general.find_game_entity("Ramp")
    Report.critical_result(Tests.find_ramp, ramp_id.IsValid())

    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(ramp_id)
    handler.add_callback("OnCollisionBegin", on_collision_begin)

    for box in all_boxes:
        Report.info("********Dropping Box {}********".format(box.name))
        # 4) Find the box
        Report.critical_result(box.valid_test, box.id.IsValid())

        # 5) Drop the box
        box.set_gravity_enabled(True)
        Report.critical_result(box.fell_test, helper.wait_for_condition(lambda: is_falling(box), FALLING_TIMEOUT))

        # 6) Wait for the box to hit the ramp
        Report.result(box.hit_ramp_test, helper.wait_for_condition(lambda: box.hit_ramp, TIMEOUT))

        # 7) Measure the bounce height
        Report.result(box.peaked_test, helper.wait_for_condition(lambda: reached_max_height(box), TIMEOUT))

        # Freeze the box so it does not interfere with the other boxes
        box.set_velocity(lymath.Vector3(0.0, 0.0, 0.0))
        box.set_gravity_enabled(False)

    # 8) Special case: Assert the a box with zero restitution did not bounce
    Report.result(Tests.box_zero_did_not_bounce, box_zero.bounce_height < ZERO_RESTITUTION_BOUNCE_TOLERANCE)

    # 9) Assert that greater restitution coefficients result in higher bounces
    ordered_bounces = box_high.bounce_height > box_mid.bounce_height > box_low.bounce_height > box_zero.bounce_height
    Report.result(Tests.bounce_height_ordered, ordered_bounces)

    # 10) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_Restitution)
