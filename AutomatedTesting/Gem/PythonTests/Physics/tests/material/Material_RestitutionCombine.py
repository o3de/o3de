"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C4044457
# Test Case Title : Verify that when two objects with different materials collide, the restitution combine works



# fmt: off
class Tests():
    enter_game_mode           = ("Entered game mode",                                    "Failed to enter game mode")
    find_ramp                 = ("Ramp entity found",                                    "Ramp entity not found")
    find_box_minimum          = ("Box entity 'minimum' found",                           "Box entity 'minimum' not found")
    find_box_multiply         = ("Box entity 'multiply' found",                          "Box entity 'multiply' not found")
    find_box_average          = ("Box entity 'average' found",                           "Box entity 'average' not found")
    find_box_maximum          = ("Box entity 'maximum' found",                           "Box entity 'maximum' not found")
    box_fell_minimum          = ("Box 'minimum' fell",                                   "Box 'minimum' did not fall")
    box_fell_multiply         = ("Box 'multiply' fell",                                  "Box 'multiply' did not fall")
    box_fell_average          = ("Box 'average' fell",                                   "Box 'average' did not fall")
    box_fell_maximum          = ("Box 'maximum' fell",                                   "Box 'maximum' did not fall")
    box_hit_ramp_minimum      = ("Box 'minimum' hit the ramp",                           "Box 'minimum' did not hit the ramp before timeout")
    box_hit_ramp_multiply     = ("Box 'multiply' hit the ramp",                          "Box 'multiply' did not hit the ramp before timeout")
    box_hit_ramp_average      = ("Box 'average' hit the ramp",                           "Box 'average' did not hit the ramp before timeout")
    box_hit_ramp_maximum      = ("Box 'maximum' hit the ramp",                           "Box 'maximum' did not hit the ramp before timeout")
    box_peaked_minimum        = ("Box 'minimum' reached its max height",                 "Box 'minimum' did not reach its' max height before timeout")
    box_peaked_multiply       = ("Box 'multiply' reached its max height",                "Box 'multiply' did not reach its' max height before timeout")
    box_peaked_average        = ("Box 'average' reached its max height",                 "Box 'average' did not reach its' max height before timeout")
    box_peaked_maximum        = ("Box 'maximum' reached its max height",                 "Box 'maximum' did not reach its' max height before timeout")
    minimum_equals_multiply   = ("Box 'minimum' and 'multiply' bounced equal heights",   "Box 'minimum' and 'multiply' did not bounce equal heights")
    distance_ordered          = ("Boxes with greater restitution values bounced higher", "Boxes with greater restitution values did not bounce higher")
    exit_game_mode            = ("Exited game mode",                                     "Couldn't exit game mode")
# fmt: on


def Material_RestitutionCombine():
    """
    Summary:

    Level Description:
    Four boxes sit above a horizontal 'ramp'. Gravity on each rigidbody component is set to disabled.
    The boxes are identical, save for their physX material.

    A new material library was created with 4 materials, minimum, multiply, average, and maximum.
    Each material has its 'restitution combine' mode assigned as named; as well as the following properties:
        dynamic friction: 0.1
        static friction:  0.1
        restitution:      0.1

    An additional material was created for the ramp entity. It has the following properties:
        dynamic friction: 1.0
        static friction:  1.0
        restitution:      1.0
        friction combine: Average

    Each box is assigned its corresponding material
    Each box also has a PhysX box collider with default settings

    Expected Behavior:
    For each box, this script will enable gravity, then wait for the box to collide with the ramp.
    It then measures the height of the bounce relative to when it first came in contact with the ramp.
    When the z component of the box's velocity reaches zero (or below zero), the box latches its bounce height.
    The box is then frozen in place and the steps run for the next box in the list.

    Boxes with greater restitution combine mode retain more energy between collisions, therefore bouncing higher.
        minimum:   0.1 vs 1      -> 0.1
        multiply:  0.1 *  1      -> 0.1
        average:  (0.1 +  1) / 2 -> 0.55
        maximum:   0.1 vs 1      -> 1

    Test Steps:
    1) Open level
    2) Enter game mode
    3) Find the ramp

    For each box:
        4) Find the box
        5) Drop the box
        6) Ensure the box collides with the ramp
        7) Ensure the box reaches its peak height

    8) Special case: assert that minimum and multiply bounce the same height
    9) Assert that greater restitution combine modes bounce higher
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

    DISTANCE_TOLERANCE = 0.005
    TIMEOUT = 5

    class Box:
        def __init__(self, name, valid_test, fell_test, hit_ramp_test, peaked_test):
            self.name = name
            self.id = general.find_game_entity(name)
            self.start_position = self.get_position()
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

    def float_is_close(value, target, tolerance):
        return abs(value - target) <= tolerance

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "Material_RestitutionCombine")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # fmt: off
    # Set up our boxes
    box_minimum = Box(
        name          = "Minimum",
        valid_test    = Tests.find_box_minimum,
        fell_test     = Tests.box_fell_minimum,
        hit_ramp_test = Tests.box_hit_ramp_minimum,
        peaked_test   = Tests.box_peaked_minimum,
    )
    box_multiply = Box(
        name          = "Multiply",
        valid_test    = Tests.find_box_multiply,
        fell_test     = Tests.box_fell_multiply,
        hit_ramp_test = Tests.box_hit_ramp_multiply,
        peaked_test   = Tests.box_peaked_multiply,
    )
    box_average = Box(
        name          = "Average",
        valid_test    = Tests.find_box_average,
        fell_test     = Tests.box_fell_average,
        hit_ramp_test = Tests.box_hit_ramp_average,
        peaked_test   = Tests.box_peaked_average,
    )
    box_maximum = Box(
        name          = "Maximum",
        valid_test    = Tests.find_box_maximum,
        fell_test     = Tests.box_fell_maximum,
        hit_ramp_test = Tests.box_hit_ramp_maximum,
        peaked_test   = Tests.box_peaked_maximum,
    )
    all_boxes = (box_minimum, box_multiply, box_average, box_maximum)
    # fmt: on

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
        Report.critical_result(box.fell_test, helper.wait_for_condition(lambda: is_falling(box), TIMEOUT))

        # 6) Wait for the box to hit the ground
        Report.result(box.hit_ramp_test, helper.wait_for_condition(lambda: box.hit_ramp, TIMEOUT))

        # 7) Measure the bounce height
        Report.result(box.peaked_test, helper.wait_for_condition(lambda: reached_max_height(box), TIMEOUT))

        # Freeze the box so it does not interfere with the other boxes
        box.set_velocity(lymath.Vector3(0.0, 0.0, 0.0))
        box.set_gravity_enabled(False)

    # 8) Special case: assert that minimum and multiply bounce the same height
    boxes_are_close = float_is_close(box_minimum.bounce_height, box_multiply.bounce_height, DISTANCE_TOLERANCE)
    Report.result(Tests.minimum_equals_multiply, boxes_are_close)

    # 9) Assert that greater coefficients result in higher bounces
    distance_ordered = (
        boxes_are_close and box_minimum.bounce_height < box_average.bounce_height < box_maximum.bounce_height
    )
    Report.result(Tests.distance_ordered, distance_ordered)

    # 10) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_RestitutionCombine)
