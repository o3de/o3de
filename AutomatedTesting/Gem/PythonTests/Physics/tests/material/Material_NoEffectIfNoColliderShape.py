"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5296614
# Test Case Title : Check that unless you assign a shape to a physX collider component,
# the material assigned to it does not take affect

# fmt: off
class Tests:
    # level
    enter_game_mode  = ("Entered game mode",                "Failed to enter game mode")
    exit_game_mode   = ("Exited game mode",                 "Couldn't exit game mode")
    collider_1_found = ("collider_1 was found",             "collider_1 was not found")
    collider_2_found = ("collider_2 was found",             "collider_2 was not found")
    ball_1_found     = ("ball_1 was found",                 "ball_1 was not found")
    ball_1_gravity   = ("ball_1 gravity is disabled",       "ball_1 gravity is enabled")
    ball_1_collision = ("ball_1 collided with collider_1",  "ball_1 passed through collider_1")
    ball_2_found     = ("ball_2 was found",                 "ball_2 was not found")
    ball_2_gravity   = ("ball_2 gravity is disabled",       "ball_2 gravity is enabled")
    ball_2_collision = ("ball_2 passed through collider_2", "ball_2 collided with collider_2")
    trigger_1_found  = ("trigger_1 was found",              "trigger_1 was not found")
    trigger_2_found  = ("trigger_2 was found",              "trigger_2 was not found")
# fmt: on


def Material_NoEffectIfNoColliderShape():
    """
    Summary:
    Runs an automated test to verify that unless you assign a shape to a PhysX collider component,
    the material assigned to it does not take affect

    Level Description:
    4 colliders named "collider_1", "collider_2", "ball_1" and "ball_2", all with PhysX Collider component.
    collider_1 has no shape assigned to it, but collider_2 has box shape.
    ball_1 and ball_2 have sphere shape, PhysX Rigid Body component, gravity disabled and initial linear velocity
    of 20 m/s on Y axis.
    Each ball is positioned in front of its respective collider.

    Expected Behavior:
    The balls are supposed to move towards the colliders.
    Ball_1 should pass through collider_1 WITHOUT collision, and enter trigger_1.
    Ball_2 should collide with collider_2 and NOT enter trigger_2.

    Test Steps:
    1) Load the level
    2) Enter game mode
    3) Setup entities
    4) Wait for balls to collide with colliders and/or triggers
    5) Report results
    6) Exit game mode
    7) Close editor
    """

    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import azlmbr.legacy.general as general
    import azlmbr.bus

    # Constants
    TIME_OUT = 2.0

    def get_test(entity_name, suffix):
        return Tests.__dict__[entity_name + suffix]

    class Entity:
        def __init__(self, name):
            self.name = name
            self.validate_ID()

        def validate_ID(self):
            self.id = general.find_game_entity(self.name)
            found_tuple = get_test(self.name, "_found")
            Report.critical_result(found_tuple, self.id.IsValid())

    class Ball(Entity):
        def __init__(self, name, collider, trigger):
            Entity.__init__(self, name)
            self.collider = collider
            self.trigger = trigger
            self.collided_with_collider = False
            self.collided_with_trigger = False
            self.validate_gravity()
            self.setup_collision_handler()

        def validate_gravity(self):
            gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id)
            gravity_tuple = get_test(self.name, "_gravity")
            Report.critical_result(gravity_tuple, not gravity_enabled)

        def collider_hit(self, args):
            colliding_entity_id = args[0]
            if colliding_entity_id.Equal(self.id):
                Report.info(self.name + " collided with " + self.collider.name)
                self.collided_with_collider = True

        def trigger_hit(self, args):
            colliding_entity_id = args[0]
            if colliding_entity_id.Equal(self.id):
                Report.info(self.name + " collided with " + self.trigger.name)
                self.collided_with_trigger = True

        def setup_collision_handler(self):
            self.collider.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.collider.handler.connect(self.collider.id)
            self.collider.handler.add_callback("OnCollisionBegin", self.collider_hit)
            self.trigger.handler = azlmbr.physics.TriggerNotificationBusHandler()
            self.trigger.handler.connect(self.trigger.id)
            self.trigger.handler.add_callback("OnTriggerEnter", self.trigger_hit)

    def both_balls_have_moved():
        return ball_1.collided_with_trigger and ball_2.collided_with_collider

    helper.init_idle()

    # 1) Load the level
    helper.open_level("Physics", "Material_NoEffectIfNoColliderShape")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Setup entities
    collider_1 = Entity("collider_1")
    collider_2 = Entity("collider_2")
    trigger_1 = Entity("trigger_1")
    trigger_2 = Entity("trigger_2")
    ball_1 = Ball("ball_1", collider_1, trigger_1)
    ball_2 = Ball("ball_2", collider_2, trigger_2)

    # 4) Wait for balls to collide
    helper.wait_for_condition(both_balls_have_moved, TIME_OUT)

    # 5) Report results
    Report.result(Tests.ball_1_collision, not ball_1.collided_with_collider and ball_1.collided_with_trigger)
    Report.result(Tests.ball_2_collision, ball_2.collided_with_collider and not ball_2.collided_with_trigger)

    # 6) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_NoEffectIfNoColliderShape)
