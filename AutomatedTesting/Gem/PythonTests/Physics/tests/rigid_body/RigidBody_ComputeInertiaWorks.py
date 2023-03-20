"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# Test case ID : C4976218
# Test Case Title: Verify that when compute inertia is checked, the physX engine does compute the inertia of the objects

# fmt: off

class Tests():
    enter_game_mode         = ("Entered game mode",      "Failed to enter game mode")
    find_upper_boxes        = ("Upper Boxes found",      "Upper Boxes not found")
    find_lower_boxes        = ("Lower Boxes found",      "Lower Boxes not found")
    boxes_collided          = ("All Boxes Collided",     "Not all Boxes Collided")
    upper_box_x_did_topple  = ("Upper Box X did topple", "Upper Box X did not toppled")
    upper_box_y_did_topple  = ("Upper Box Y did topple", "Upper Box Y did not toppled")
    upper_box_z_did_topple  = ("Upper Box Z did topple", "Upper Box Z did not toppled")
    exit_game_mode          = ("Exited game mode",       "Failed to exit game mode")
# fmt: on

def RigidBody_ComputeInertiaWorks():
    """
    Summary:
    Level Description:
    A box (entity: Upper Box) set above and askew to another box (entity: Lower Box)
    Lower box and Upper box has computed inertia.
    Test Steps:
    --> Open level
    --> Enter game mode
    --> Retrieve entities
    --> Check for collision between the top box and lower box for the 3 pairs (6 boxes total)
    --> Check that the upper box's x, y, or z angular velocity decreases past -1.0
    --> Exit game mode
    --> Close editor
    :return: None
    """
    # Setup path
    import os
    import sys
    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.components
    import azlmbr.physics

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    class UpperBox:
        def __init__(self, name):
            self.name = name
            self.id = general.find_game_entity(name)
            self.collided_with_lower_box = False
            self.handler = None
            self.lower_box_list = None

        def on_boxes_collision_begin(self, args):
            other_id = args[0]
            for boxes in self.lower_box_list:
                if other_id.Equal(boxes.id):
                    Report.info("{} hit lower box".format(self.name))
                    self.collided_with_lower_box = True

        def create_handler(self):
            self.handler = bus.NotificationHandler("CollisionNotificationBus")
            self.handler.connect(self.id)
            self.handler.add_callback("OnCollisionBegin", self.on_boxes_collision_begin)

        @property
        def angular_velocity(self):
            return azlmbr.physics.RigidBodyRequestBus(bus.Event, "GetAngularVelocity", self.id)

    class LowerBox:
        def __init__(self, name):
            self.id = general.find_game_entity(name)

    # Amount of time before test times out
    TIME_OUT = 3.0
    MINIMUM_ANGUlAR_VELOCITY = -1.0

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "RigidBody_ComputeInertiaWorks")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve entities
    upper_box_x = UpperBox("Upper Box X")
    upper_box_y = UpperBox("Upper Box Y")
    upper_box_z = UpperBox("Upper Box Z")

    lower_box_x = LowerBox("Lower Box X")
    lower_box_y = LowerBox("Lower Box Y")
    lower_box_z = LowerBox("Lower Box Z")

    # Store boxes in two lists
    upper_box_list = [upper_box_x, upper_box_y, upper_box_z]
    lower_box_list = [lower_box_x, lower_box_y, lower_box_z]


    upper_boxes_found = True
    lower_boxes_found = True

    for upper_boxes in upper_box_list:
        upper_boxes.lower_box_list = lower_box_list
        if upper_boxes.id is None:
            upper_boxes_found = False

    for lower_boxes in lower_box_list:
        if lower_boxes.id is None:
            lower_boxes_found = False

    Report.critical_result(Tests.find_upper_boxes, upper_boxes_found)
    Report.critical_result(Tests.find_lower_boxes, lower_boxes_found)

    for box in upper_box_list:
        box.create_handler()

    def all_boxes_collided():
        for boxes in upper_box_list:
            if not boxes.collided_with_lower_box:
                return False
        return True

    # 4) Wait for the boxes to collide
    boxes_collided = helper.wait_for_condition(all_boxes_collided, TIME_OUT)
    Report.result(Tests.boxes_collided, boxes_collided)

    # 5) Check that the upper box's corresponding angular velocity is lower than minimum (higher negative number)
    def validate_angular_velocities():
        velocities_set = True
        if upper_box_x.angular_velocity.x > MINIMUM_ANGUlAR_VELOCITY:
            velocities_set = False
        if upper_box_y.angular_velocity.y > MINIMUM_ANGUlAR_VELOCITY:
            velocities_set = False
        if upper_box_z.angular_velocity.z > MINIMUM_ANGUlAR_VELOCITY:
            velocities_set = False
        return velocities_set

    helper.wait_for_condition(validate_angular_velocities, TIME_OUT)

    # Checking to see if the X angular has a greater negative number (higher negative value) than MINIMUM
    Report.info("Angular Velocity for X Box = {}".format(upper_box_x.angular_velocity.x))
    Report.info("Angular Velocity for Y Box = {}".format(upper_box_y.angular_velocity.y))
    Report.info("Angular Velocity for Z Box = {}".format(upper_box_z.angular_velocity.z))

    Report.result(Tests.upper_box_x_did_topple, upper_box_x.angular_velocity.x < MINIMUM_ANGUlAR_VELOCITY)
    Report.result(Tests.upper_box_y_did_topple, upper_box_y.angular_velocity.y < MINIMUM_ANGUlAR_VELOCITY)
    Report.result(Tests.upper_box_z_did_topple, upper_box_z.angular_velocity.z < MINIMUM_ANGUlAR_VELOCITY)

    # 7) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_ComputeInertiaWorks)
