"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test Case ID    : C4982595
# Test Case Title : Verify that when the Trigger Checkbox is ticked, the object no longer collides with another object
#                   but simply passes through it



# fmt: off
class Tests:
    enter_game_mode                          = ("Entered game mode",                        "Failed to enter game mode")
    sphere_found_valid                       = ("Sphere found and validated",               "Failed to find and validate Sphere")
    trigger_box_found_valid                  = ("Trigger Box found and validated",          "Failed to find and validate Trigger Box")
    physical_box_found_valid                 = ("Physical Box found and validated",         "Failed to find and validate Physical Box")
    sphere_gravity_disabled                  = ("Gravity is disabled on Sphere",            "Gravity is enabled on Sphere")
    sphere_positive_initial_x_velocity       = ("Sphere has positive initial x-velocity",   "Sphere does not have positive initial x-velocity")
    sphere_entered_trigger_box               = ("Sphere entered Trigger Box",               "Sphere did not enter Trigger Box")
    sphere_exited_trigger_box                = ("Sphere exited Trigger Box",                "Sphere did not exit Trigger Box")
    sphere_passed_through_trigger_box        = ("Sphere passed through Trigger Box",        "Sphere did not pass through Trigger Box")
    sphere_collided_with_physical_box        = ("Sphere collided with Physical Box",        "Sphere did not collide with Physical Box")
    sphere_bounced_back                      = ("Sphere bounced back from Physical Box",    "Sphere did not bounce back from Physical Box")
    sphere_did_not_pass_through_physical_box = ("Sphere did not pass through Physical Box", "Sphere passed through Physical Box")
    exit_game_mode                           = ("Exited game mode",                         "Failed to exit game mode")
# fmt: on


def Collider_TriggerPassThrough():
    """
    Summary:
    This script runs an automated test to verify that when an entity's PhysX collider is set as a trigger, the entity no
    longer collides with another entity.

    Level Description:
    Entity: Sphere:       PhysX Rigid Body, PhysX Collider with sphere shape, and Mesh with sphere asset
                          Gravity disabled, Radius 1.0, Initial linear x-velocity +30 m/s, Position (36.0, 36.0, 36.0)
    Entity: Trigger Box:  PhysX Collider with box shape, and Mesh with cube asset
                          Trigger enabled, Dimensions (2.0, 2.0, 2.0), Z-offset 1.0, Position (42.0, 36.0, 35.0)
    Entity: Physical Box: PhysX Collider with box shape, and Mesh with cube asset
                          Dimensions (2.0, 2.0, 2.0), Z-offset 1.0, Position (48.0, 36.0, 35.0)
    The entities are aligned along the x-axis as follows:
                      _              _
       O   --->      !_!            |_|
    Sphere       Trigger Box    Physical Box

    Expected behavior:
    The sphere will pass through the trigger box, collide with the physical box, and bounce back without passing through
    the physical box.

    Test Steps:
     1) Open level and enter game mode
     2) Retrieve and validate entities
     3) Check that gravity is disabled on the sphere
     4) Check that the sphere has positive initial x-velocity
     5) Wait for the sphere to enter the trigger box
     6) Wait for the sphere to exit the trigger box
     7) Check that the sphere passed through the trigger box
     8) Wait for the sphere to collide with the physical box
     9) Wait for the sphere to stop colliding with the physical box and check that the sphere bounced back
    10) Check that the sphere did not pass through the physical box
    11) Exit game mode and close editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    # Setup path
    import os
    import sys
    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr.components
    import azlmbr.physics
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    TIME_OUT_SECONDS = 3.0
    SPHERE_RADIUS = 1.0
    BOX_X_DIMENSION = 2.0
    BOX_X_RADIUS = BOX_X_DIMENSION / 2
    CLOSE_ENOUGH_BUFFER = 0.1

    class Entity:
        def __init__(self, name, found_valid_test):
            self.name = name
            self.id = general.find_game_entity(name)
            self.found_valid_test = found_valid_test

        def get_x_position(self):
            position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)
            Report.info_vector3(position, "{}'s position:".format(self.name))
            return position.x

    class Sphere(Entity):
        def __init__(self, name, found_valid_test):
            Entity.__init__(self, name, found_valid_test)
            self.initial_x_velocity = self.get_x_velocity()

            # Trigger state values
            self.entered_trigger_target = False
            self.trigger_enter_position = None
            self.exited_trigger_target = False
            self.trigger_exit_position = None
            self.passed_through_trigger_target = False

            # Collision state values
            self.began_collision_with_collision_target = False
            self.ended_collision_with_collision_target = False
            self.bounced_back = False
            self.did_not_pass_through_collision_target = None

        def is_gravity_disabled(self):
            return not azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id)

        def get_x_velocity(self):
            velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)
            Report.info_vector3(velocity, "{}'s velocity:".format(self.name))
            return velocity.x

    class TriggerBox(Entity):
        def __init__(self, name, found_valid_test, trigger_target):
            Entity.__init__(self, name, found_valid_test)
            self.trigger_target = trigger_target

            # Set up trigger notification handler
            self.handler = azlmbr.physics.TriggerNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnTriggerEnter", self.on_trigger_enter)
            self.handler.add_callback("OnTriggerExit", self.on_trigger_exit)

        def on_trigger_enter(self, args):
            other_id = args[0]
            if other_id.Equal(self.trigger_target.id) and not self.trigger_target.entered_trigger_target:
                self.trigger_target.trigger_enter_position = self.trigger_target.get_x_position()
                self.trigger_target.entered_trigger_target = True

        def on_trigger_exit(self, args):
            other_id = args[0]
            if other_id.Equal(self.trigger_target.id) and not self.trigger_target.exited_trigger_target:
                self.trigger_target.trigger_exit_position = self.trigger_target.get_x_position()
                self.trigger_target.exited_trigger_target = True
                # Check that the sphere's position traveled at least the approximate x-length of the box measured from
                # the point on the sphere where it entered the trigger to the point on the sphere where it exited
                if (
                    self.trigger_target.trigger_exit_position - SPHERE_RADIUS + CLOSE_ENOUGH_BUFFER
                    >= self.trigger_target.trigger_enter_position + BOX_X_DIMENSION + SPHERE_RADIUS
                ):
                    self.trigger_target.passed_through_trigger_target = True

    class PhysicalBox(Entity):
        def __init__(self, name, found_valid_test, collision_target):
            Entity.__init__(self, name, found_valid_test)
            self.collision_target = collision_target

            # Set up collision notification handler
            self.collision_handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.collision_handler.connect(self.id)
            self.collision_handler.add_callback("OnCollisionBegin", self.on_collision_begin)
            self.collision_handler.add_callback("OnCollisionEnd", self.on_collision_end)

        def on_collision_begin(self, args):
            other_id = args[0]
            if other_id.Equal(self.collision_target.id):
                self.collision_target.began_collision_with_collision_target = True

        def on_collision_end(self, args):
            other_id = args[0]
            if other_id.Equal(self.collision_target.id):
                self.collision_target.ended_collision_with_collision_target = True
                # Check that the sphere reversed its x-direction
                if self.collision_target.get_x_velocity() < 0:
                    self.collision_target.bounced_back = True
                # Check that the whole sphere is to the negative x-direction of the box
                if sphere.get_x_position() + SPHERE_RADIUS <= physical_box.get_x_position() - BOX_X_RADIUS:
                    self.collision_target.did_not_pass_through_collision_target = True

    # 1) Open level and enter game mode
    helper.init_idle()
    helper.open_level("Physics", "Collider_TriggerPassThrough")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve and validate entities
    sphere = Sphere("Sphere", Tests.sphere_found_valid)
    trigger_box = TriggerBox("Trigger Box", Tests.trigger_box_found_valid, trigger_target=sphere)
    physical_box = PhysicalBox("Physical Box", Tests.physical_box_found_valid, collision_target=sphere)

    entities = (sphere, trigger_box, physical_box)
    for entity in entities:
        Report.critical_result(entity.found_valid_test, entity.id.IsValid())

    # 3) Check that gravity is disabled on the sphere
    Report.critical_result(Tests.sphere_gravity_disabled, sphere.is_gravity_disabled())

    # 4) Check that the sphere has positive initial x-velocity
    Report.critical_result(Tests.sphere_positive_initial_x_velocity, sphere.initial_x_velocity > 0)

    # 5) Wait for the sphere to enter the trigger box
    helper.wait_for_condition(lambda: sphere.entered_trigger_target, TIME_OUT_SECONDS)
    Report.critical_result(Tests.sphere_entered_trigger_box, sphere.entered_trigger_target)

    # 6) Wait for the sphere to exit the trigger box
    helper.wait_for_condition(lambda: sphere.exited_trigger_target, TIME_OUT_SECONDS)
    Report.critical_result(Tests.sphere_exited_trigger_box, sphere.exited_trigger_target)

    # 7) Check that the sphere passed through the trigger box
    Report.critical_result(Tests.sphere_passed_through_trigger_box, sphere.passed_through_trigger_target)

    # 8) Wait for the sphere to collide with the physical box
    helper.wait_for_condition(lambda: sphere.began_collision_with_collision_target, TIME_OUT_SECONDS)
    Report.critical_result(Tests.sphere_collided_with_physical_box, sphere.began_collision_with_collision_target)

    # 9) Wait for the sphere to stop colliding with the physical box and check that the sphere bounced back
    helper.wait_for_condition(lambda: sphere.ended_collision_with_collision_target, TIME_OUT_SECONDS)
    Report.result(Tests.sphere_bounced_back, sphere.bounced_back)

    # 10) Check that the sphere did not pass through the physical box
    Report.result(Tests.sphere_did_not_pass_through_physical_box, sphere.did_not_pass_through_collision_target)

    # 11) Exit game mode and close editor
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_TriggerPassThrough)
