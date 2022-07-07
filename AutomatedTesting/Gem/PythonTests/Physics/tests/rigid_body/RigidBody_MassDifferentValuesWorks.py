"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4976201
# Test Case Title : Verify that the value assigned to the Mass of the object, gets actually assigned to the object


# fmt: off
class Tests:
    # test iteration 1
    enter_game_mode_1         = ("Entered game mode first time",                             "Failed to enter game mode first time")
    ProjectileSphere_exists_1 = ("ProjectileSphere entity found first time",                 "ProjectileSphere entity not found first time")
    TargetSphere_exists_1     = ("TargetSphere entity found first time",                     "TargetSphere entity not found first time")
    Trigger1_exists_1         = ("Trigger1 entity found first time",                         "Trigger1 entity not found first time")
    Trigger2_exists_1         = ("Trigger2 entity found first time",                         "Trigger2 entity not found first time")
    Trigger3_exists_1         = ("Trigger3 entity found first time",                         "Trigger3 entity not found first time")
    TargetSphere_mass_1       = ("Mass of TargetSphere was set to 1.0",                      "Mass of TargetSphere was not set to 1.0")
    spheres_collided_1        = ("ProjectileSphere and TargetSphere collided first time",    "Timed out before ProjectileSphere & 2 collided first time")
    stopped_correctly_1       = ("TargetSphere hit Trigger1 & Trigger2 but not Trigger_3",   "TargetSphere did not stop correctly")
    check_y_1                 = ("sphere did not move far from expected in Y direction _1",  "TargetSphere moved an unexpected distance in Y direction _1")
    check_z_1                 = ("sphere did not move far from expected in Z direction _1",  "TargetSphere moved an unexpected distance in Z direction _1")
    exit_game_mode_1          = ("Exited game mode first time",                              "Couldn't exit game mode first time")

    # test iteration 2
    enter_game_mode_2         = ("Entered game mode second time",                            "Failed to enter game mode second time")
    ProjectileSphere_exists_2 = ("ProjectileSphere entity found second time",                "ProjectileSphere entity not found second time")
    TargetSphere_exists_2     = ("TargetSphere entity found second time",                    "TargetSphere entity not found second time")
    Trigger1_exists_2         = ("Trigger1 entity found second time",                        "Trigger1 entity not found second time")
    Trigger2_exists_2         = ("Trigger2 entity found second time",                        "Trigger2 entity not found second time")
    Trigger3_exists_2         = ("Trigger3 entity found second time",                        "Trigger3 entity not found second time")
    TargetSphere_mass_2       = ("Mass of TargetSphere was set to 10.0",                     "Mass of TargetSphere was not set to 10.0")
    spheres_collided_2        = ("ProjectileSphere and TargetSphere collided second time",   "Timed out before ProjectileSphere & 2 collided second time")
    stopped_correctly_2       = ("TargetSphere hit Trigger1 but not Trigger2 or Trigger3",   "TargetSphere did not stop correctly")
    check_y_2                 = ("sphere did not move far from expected in Y direction _2",  "TargetSphere moved an unexpected distance in Y direction _2")
    check_z_2                 = ("sphere did not move far from expected in Z direction _2",  "TargetSphere moved an unexpected distance in Z direction _2")
    exit_game_mode_2          = ("Exited game mode second time",                             "Couldn't exit game mode second time")

    # test iteration 3
    enter_game_mode_3         = ("Entered game mode third time",                             "Failed to enter game mode third time")
    ProjectileSphere_exists_3 = ("ProjectileSphere entity found third time",                 "ProjectileSphere entity not found third time")
    TargetSphere_exists_3     = ("TargetSphere entity found third time",                     "TargetSphere entity not found third time")
    Trigger1_exists_3         = ("Trigger1 entity found third time",                         "Trigger1 entity not found third time")
    Trigger2_exists_3         = ("Trigger2 entity found third time",                         "Trigger2 entity not found third time")
    Trigger3_exists_3         = ("Trigger3 entity found third time",                         "Trigger3 entity not found third time")
    TargetSphere_mass_3       = ("Mass of TargetSphere was set to 100.0",                    "Mass of TargetSphere was not set to 100.0")
    spheres_collided_3        = ("ProjectileSphere and TargetSphere collided third time",    "Timed out before ProjectileSphere & 2 collided third time")
    stopped_correctly_3       = ("TargetSphere did not hit Trigger1, Trigger2, or Trigger3", "TargetSphere hit one or more triggers before stopping")
    check_y_3                 = ("sphere did not move far from expected in Y direction _3",  "TargetSphere moved an unexpected distance in Y direction _3")
    check_z_3                 = ("sphere did not move far from expected in Z direction _3",  "TargetSphere moved an unexpected distance in Z direction _3")
    exit_game_mode_3          = ("Exited game mode third time",                              "Couldn't exit game mode third time")

    # general
    velocity_sizing           = ("The velocities are in the correct order of magnitude",     "The velocities are not correctly ordered in magnitude")
# fmt: on


def RigidBody_MassDifferentValuesWorks():
    """
    Summary:
    Checking that the mass set to the object is actually applied via colliding entities

    Level Description:
    ProjectileSphere (entity) - Sphere shaped Mesh; Sphere shaped PhysX Collider;
        PhysX Rigid Body: initial linear velocity in X direction is 5m/s, initial mass 1kg,
            gravity disabled, linear damping default (0.05)
    TargetSphere (entity) - Sphere shaped Mesh; Sphere shaped PhysX Collider;
        PhysX Rigid Body: no initial velocity, initial mass 1kg, gravity disabled, linear damping 1.0

    Expected Behavior:
    The ProjectileSphere entity will float towards TargetSphere entity and then collide with it.
    Because they are the same mass initially, the second sphere will move after collision.
    TargetSphere's mass will be increased and scenario will run again,
    but TargetSphere will have a smaller velocity after collision.
    TargetSphere will then increase mass again and should barely move after the final collision.

    Test Steps:
     1) Open level
     2) Repeat steps 3-9
        3) Enter game mode
        4) Find and setup entities
        5) Set mass of the TargetSphere
        6) Check for collision
        7) Wait for TargetSphere x velocity = 0
        8) Check the triggers
        9) Exit game mode
    10) Verify the velocity of TargetSphere decreased after collision as mass increased
    11) Close the editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    import sys
    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    MOVEMENT_TIMEOUT = 7.0
    COLLISION_TIMEOUT = 2.0
    VELOCITY_ZERO = 0.01
    Y_Z_BUFFER = 0.01
    TARGET_SPHERE_NAME = "TargetSphere"
    PROJECTILE_SPHERE_NAME = "ProjectileSphere"
    TRIGGER_1_NAME = "Trigger1"
    TRIGGER_2_NAME = "Trigger2"
    TRIGGER_3_NAME = "Trigger3"

    class ProjectileSphere:
        def __init__(self, test_iteration):
            self.name = PROJECTILE_SPHERE_NAME
            self.test_iteration = test_iteration
            self.timeout_reached = True
            self.id = general.find_game_entity(self.name)
            Report.critical_result(
                Tests.__dict__["ProjectileSphere_exists_" + str(self.test_iteration)], self.id.IsValid()
            )

        def destroy_me(self):
            azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "DestroyGameEntity", self.id)

    class TargetSphere:
        def __init__(self, mass_to_assign, stop_before_trigger_name, expected_trigger_pattern, test_iteration):
            self.id = None
            self.name = TARGET_SPHERE_NAME
            self.start_mass = None
            self.mass_to_assign = mass_to_assign
            self.collision_begin = False
            self.after_collision_velocity = None
            self.x_movement_timeout = True
            self.stop_before_trigger_name = stop_before_trigger_name
            self.expected_trigger_pattern = expected_trigger_pattern
            self.collision_ended = False
            self.test_iteration = test_iteration
            self.test_set_mass = self.get_test("TargetSphere_mass_")
            self.test_enter_game_mode = self.get_test("enter_game_mode_")
            self.test_ProjectileSphere_exist = self.get_test("ProjectileSphere_exists_")
            self.test_TargetSphere_exist = self.get_test("TargetSphere_exists_")
            self.test_spheres_collided = self.get_test("spheres_collided_")
            self.test_stop_properly = self.get_test("stopped_correctly_")
            self.test_check_y = self.get_test("check_y_")
            self.test_check_z = self.get_test("check_z_")
            self.test_exit_game_mode = self.get_test("exit_game_mode_")

        def get_test(self, test_prefix):
            return Tests.__dict__[test_prefix + str(self.test_iteration)]

        def find(self):
            self.id = general.find_game_entity(self.name)
            Report.critical_result(Tests.__dict__["TargetSphere_exists_" + str(self.test_iteration)], self.id.IsValid())

        def setup_mass(self):
            self.start_mass = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetMass", self.id)
            Report.info("{} starting mass: {}".format(self.name, self.start_mass))
            azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetMass", self.id, self.mass_to_assign)
            general.idle_wait_frames(1)  # wait for mass to apply
            mass_after_set = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetMass", self.id)
            Report.info("{} mass after setting: {}".format(self.name, mass_after_set))
            Report.result(self.test_set_mass, self.mass_to_assign == mass_after_set)

        def current_velocity(self):
            return azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)

        def on_collision_begin(self, args):
            other_id = args[0]
            if other_id.Equal(self.id):
                Report.info("spheres collision begin")
                self.collision_begin = True

        def on_collision_end(self, args):
            other_id = args[0]
            if other_id.Equal(self.id):
                Report.info("spheres collision end")
                self.after_collision_velocity = self.current_velocity()
                self.collision_ended = True

        def add_collision_handlers(self, projectile_sphere_id):
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(projectile_sphere_id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)
            self.handler.add_callback("OnCollisionEnd", self.on_collision_end)

        def x_velocity_zero(self):
            if abs(self.current_velocity().x) < VELOCITY_ZERO:
                Report.info("TargetSphere has stopped moving.")
                self.x_movement_timeout = False
                return True
            return False

        def collision_complete(self):
            return self.collision_begin and self.collision_ended

        def check_y_z_movement_from_collision(self):
            """
            Used to check that the entity has not moved too far in either the Y or Z direction
            """

            def is_within_tolerance(velocity_one_direction):
                return abs(velocity_one_direction) < Y_Z_BUFFER

            Report.info_vector3(self.after_collision_velocity, "Initial Velocity: ")
            Report.result(self.test_check_y, is_within_tolerance(self.after_collision_velocity.y))
            Report.result(self.test_check_z, is_within_tolerance(self.after_collision_velocity.z))

    class Trigger:
        """
        Used in the level to tell if the TargetSphere entity has moved a certain distance.
        There are three triggers set up in the level.
        """

        def __init__(self, name, test_iteration):
            self.name = name
            self.handler = None
            self.triggered = False
            self.test_iteration = test_iteration
            self.id = general.find_game_entity(self.name)
            Report.critical_result(Tests.__dict__[self.name + "_exists_" + str(self.test_iteration)], self.id.IsValid())
            self.setup_handler()

        def on_trigger_enter(self, args):
            """
            This is passed into this object's handler.add_callback().
            """
            other_id = args[0]
            self.triggered = True
            triggered_by_name = azlmbr.entity.GameEntityContextRequestBus(
                azlmbr.bus.Broadcast, "GetEntityName", other_id
            )
            Report.info("{} was triggered by {}.".format(self.name, triggered_by_name))

        def setup_handler(self):
            """
            This is called to setup the handler for this trigger object
            """
            self.handler = azlmbr.physics.TriggerNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnTriggerEnter", self.on_trigger_enter)

    class TriggerResultPattern:
        """
        Used to store and determine which triggers were activated and compare to expected
        """

        def __init__(self, trigger1_activated, trigger2_activated, trigger3_activated):
            self.trigger1_activated = trigger1_activated
            self.trigger2_activated = trigger2_activated
            self.trigger3_activated = trigger3_activated

        def __eq__(self, other_pattern):
            """
            Used to determine if two patterns equal/match each other (i.e. Expected VS Actual)
            """
            if isinstance(other_pattern, self.__class__):
                return (
                    self.trigger1_activated == other_pattern.trigger1_activated
                    and self.trigger2_activated == other_pattern.trigger2_activated
                    and self.trigger3_activated == other_pattern.trigger3_activated
                )
            else:
                return False

        def report(self, expect_actual):
            Report.info(
                """TargetSphere {} Triggers:
            Trigger_1: {}
            Trigger_2: {}
            Trigger_3: {}
            """.format(
                    expect_actual, self.trigger1_activated, self.trigger2_activated, self.trigger3_activated
                )
            )

    target_sphere_1kg = TargetSphere(
        mass_to_assign=1.0,
        stop_before_trigger_name=TRIGGER_3_NAME,
        expected_trigger_pattern=TriggerResultPattern(True, True, False),
        test_iteration=1,
    )

    target_sphere_10kg = TargetSphere(
        mass_to_assign=10.0,
        stop_before_trigger_name=TRIGGER_2_NAME,
        expected_trigger_pattern=TriggerResultPattern(True, False, False),
        test_iteration=2,
    )

    target_sphere_100kg = TargetSphere(
        mass_to_assign=100.0,
        stop_before_trigger_name=TRIGGER_1_NAME,
        expected_trigger_pattern=TriggerResultPattern(False, False, False),
        test_iteration=3,
    )

    target_spheres = [target_sphere_1kg, target_sphere_10kg, target_sphere_100kg]

    target_sphere_velocities = {}

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "RigidBody_MassDifferentValuesWorks")

    # 2) Repeat steps 3-9
    for target_sphere in target_spheres:
        Report.info("***************** Begin Test Iteration {} ******************".format(target_sphere.test_iteration))

        # 3) Enter game mode
        helper.enter_game_mode(target_sphere.test_enter_game_mode)

        # 4) Find and setup entities
        projectile_sphere = ProjectileSphere(target_sphere.test_iteration)
        target_sphere.find()
        target_sphere.add_collision_handlers(projectile_sphere.id)

        trigger_1 = Trigger(TRIGGER_1_NAME, target_sphere.test_iteration)
        trigger_2 = Trigger(TRIGGER_2_NAME, target_sphere.test_iteration)
        trigger_3 = Trigger(TRIGGER_3_NAME, target_sphere.test_iteration)

        # 5) Set mass of the TargetSphere
        target_sphere.setup_mass()

        # 6) Check for collision

        helper.wait_for_condition(target_sphere.collision_complete, COLLISION_TIMEOUT)
        Report.critical_result(target_sphere.test_spheres_collided, target_sphere.collision_complete())
        projectile_sphere.destroy_me()
        Report.info_vector3(
            target_sphere.after_collision_velocity, "Velocity of {} after the collision: ".format(target_sphere.name)
        )

        Report.info("The sphere should stop before touching {}".format(target_sphere.stop_before_trigger_name))

        # 7) Wait for TargetSphere x velocity = 0

        helper.wait_for_condition(target_sphere.x_velocity_zero, MOVEMENT_TIMEOUT)
        if target_sphere.x_movement_timeout is True:
            Report.info("TargetSphere failed to stop moving in the x direction before timeout was reached.")

        # 8) Check the triggers
        actual_trigger_pattern = TriggerResultPattern(trigger_1.triggered, trigger_2.triggered, trigger_3.triggered)

        patterns_match = actual_trigger_pattern == target_sphere.expected_trigger_pattern
        target_sphere.expected_trigger_pattern.report("Expected")
        actual_trigger_pattern.report("Actual")
        Report.result(target_sphere.test_stop_properly, patterns_match)

        target_sphere.check_y_z_movement_from_collision()
        target_sphere_velocities.update({target_sphere.test_iteration: target_sphere.after_collision_velocity.x})
        # 9) Exit game mode
        helper.exit_game_mode(target_sphere.test_exit_game_mode)
        Report.info("~~~~~~~~~~~~~~ Test Iteration {} End ~~~~~~~~~~~~~~~~~~".format(target_sphere.test_iteration))

    # 10) Verify the velocity of TargetSphere decreased after collision as mass increased
    outcome = target_sphere_velocities[1] > target_sphere_velocities[2] > target_sphere_velocities[3]
    Report.result(Tests.velocity_sizing, outcome)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_MassDifferentValuesWorks)
