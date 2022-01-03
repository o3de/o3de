"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C4044456
# Test Case Title : Verify that when two objects with different materials collide, the friction combine works



# fmt: off
class Tests():
    enter_game_mode            = ("Entered game mode",                                     "Failed to enter game mode")
    find_ramp                  = ("Ramp entity found",                                     "Ramp entity not found")
    find_box_minimum           = ("Box entity 'minimum' found",                            "Box entity 'minimum' not found")
    find_box_multiply          = ("Box entity 'multiply' found",                           "Box entity 'multiply' not found")
    find_box_average           = ("Box entity 'average' found",                            "Box entity 'average' not found")
    find_box_maximum           = ("Box entity 'maximum' found",                            "Box entity 'maximum' not found")
    box_at_rest_start_minimum  = ("Box 'minimum ' began test motionless",                  "Box 'minimum' did not begin test motionless")
    box_at_rest_start_multiply = ("Box 'multiply' began test motionless",                  "Box 'multiply' did not begin test motionless")
    box_at_rest_start_average  = ("Box 'average' began test motionless",                   "Box 'average' did not begin test motionless")
    box_at_rest_start_maximum  = ("Box 'maximum' began test motionless",                   "Box 'maximum' did not begin test motionless")
    box_was_pushed_minimum     = ("Box 'minimum' moved",                                   "Box 'minimum' did not move before timeout")
    box_was_pushed_multiply    = ("Box 'multiply' moved",                                  "Box 'multiply' did not move before timeout")
    box_was_pushed_average     = ("Box 'average' moved",                                   "Box 'average' did not move before timeout")
    box_was_pushed_maximum     = ("Box 'maximum' moved",                                   "Box 'maximum' did not move before timeout")
    box_at_rest_end_minimum    = ("Box 'minimum' came to rest",                            "Box 'minimum' did not come to rest before timeout")
    box_at_rest_end_multiply   = ("Box 'multiply' came to rest",                           "Box 'multiply' did not come to rest before timeout")
    box_at_rest_end_average    = ("Box 'average' came to rest",                            "Box 'average' did not come to rest before timeout")
    box_at_rest_end_maximum    = ("Box 'maximum' came to rest",                            "Box 'maximum' did not come to rest before timeout")
    minimum_equals_multiply    = ("Box 'minimum' and 'multiply' traveled equal distances", "Box 'minimum' and 'multiply' did not travel equal distances")
    distance_ordered           = ("Box travel distance was ordered as expected",           "Box travel distance was not ordered as expected")
    exit_game_mode             = ("Exited game mode",                                      "Couldn't exit game mode")
# fmt: on


def Material_FrictionCombine():
    """
    Summary:

    Level Description:
    Four boxes sit on a horizontal 'ramp'. The boxes are identical, save for their physX material:

    A new material library was created with 4 materials, minimum, multiply, average, and maximum.
    Each material has its 'friction combine' mode assigned as named; as well as the following properties:
        dynamic friction: 0.1
        static friction:  0.1
        restitution:      0.1

    An additional material was created for the ramp entity. It has the following properties:
        dynamic friction: 1.0
        static friction:  1.0
        restitution:      1.0
        friction combine: Average

    Each box is assigned its corresponding friction material
    Each box also has a PhysX box collider with default settings
    The COM of the boxes is placed on the plane (0, 0, -0.5), so as to remove any torque moments and resulting rotations

    Expected Behavior:
    For each box, this script will apply a force impulse in the world X direction
    Boxes with greater friction combine mode results should travel a shorter distance.
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
        5) Ensure the box is stationary
        6) Push the box and wait for it to come to rest

    7) Special case: assert that minimum and multiply travel the same distance
    8) Assert that greater friction combine modes travel a shorter distance
    9) Exit game mode
    10) Close the editor

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

    FORCE_IMPULSE = lymath.Vector3(5.0, 0.0, 0.0)
    VECTOR_TOLERANCE = 0.001
    DISTANCE_TOLERANCE = 0.002
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
        return (
            abs(vector.x) <= VECTOR_TOLERANCE
            and abs(vector.y) <= VECTOR_TOLERANCE
            and abs(vector.z) <= VECTOR_TOLERANCE
        )

    def push(box):
        azlmbr.physics.RigidBodyRequestBus(bus.Event, "ApplyLinearImpulse", box.id, FORCE_IMPULSE)

    def float_is_close(value, target, tolerance):
        return abs(value - target) <= tolerance

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "Material_FrictionCombine")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # fmt: off
    # Set up our boxes
    box_minimum = Box(
        name                  = "Minimum",
        valid_test            = Tests.find_box_minimum,
        stationary_start_test = Tests.box_at_rest_start_minimum,
        moved_test            = Tests.box_was_pushed_minimum,
        stationary_end_test   = Tests.box_at_rest_end_minimum
    )
    box_multiply = Box(
        name                  = "Multiply",
        valid_test            = Tests.find_box_multiply,
        stationary_start_test = Tests.box_at_rest_start_multiply,
        moved_test            = Tests.box_was_pushed_multiply,
        stationary_end_test   = Tests.box_at_rest_end_multiply
    )
    box_average = Box(
        name                  = "Average",
        valid_test            = Tests.find_box_average,
        stationary_start_test = Tests.box_at_rest_start_average,
        moved_test            = Tests.box_was_pushed_average,
        stationary_end_test   = Tests.box_at_rest_end_average
    )
    box_maximum = Box(
        name                  = "Maximum",
        valid_test            = Tests.find_box_maximum,
        stationary_start_test = Tests.box_at_rest_start_maximum,
        moved_test            = Tests.box_was_pushed_maximum,
        stationary_end_test   = Tests.box_at_rest_end_maximum
    )
    all_boxes = (box_minimum, box_multiply, box_average, box_maximum)
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

    # 7) Special case: assert that minimum and multiply travel the same distance
    boxes_are_close = float_is_close(box_minimum.distance, box_multiply.distance, DISTANCE_TOLERANCE)
    Report.result(Tests.minimum_equals_multiply, boxes_are_close)

    # 8) Assert that greater coefficients result in shorter travelled distance
    distance_ordered = boxes_are_close and box_minimum.distance > box_average.distance > box_maximum.distance
    Report.result(Tests.distance_ordered, distance_ordered)

    # 9) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_FrictionCombine)
