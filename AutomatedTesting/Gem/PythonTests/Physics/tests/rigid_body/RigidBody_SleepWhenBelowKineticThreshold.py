"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
Test case ID : C4976202
Test Case Title : Verify that if the object is moving with Kinetic energy less than 
  the sleep threshold value, then physX will put it to stop after 0.4 secs (once the 
  wake counter goes to zero) if the KE is still below the threshold

"""

# fmt: off
class Tests:
    enter_game_mode_1         = ("Entered game mode_1",                           "Failed to enter game mode_1"                                     )
    Trigger1_exists_1         = ("Trigger1 entity was found_1",                   "Trigger1 entity was not found_1"                                 )
    Trigger2_exists_1         = ("Trigger2 entity was found_1",                   "Trigger2 entity was not found_1"                                 )
    Trigger3_exists_1         = ("Trigger3 entity was found_1",                   "Trigger3 entity was not found_1"                                 )
    Pushing_Cube_exists_1     = ("Pushing_Cube entity was found_1",               "Pushing_Cube entity was not found_1"                             )
    Target_Ball_exists_1      = ("Target_Ball entity was found_1",                "Target_Ball entity was not found_1"                              )
    threshold_setup_1         = ("The sleep threshold was setup properly_1",      "The sleep threshold was not setup properly_1"                    )
    cube_collided_with_ball_1 = ("The cube hit the ball the_1",                   "The cube did not hit the ball the_1"                             )
    ball_stopped_moving_1     = ("The ball has stopped moving before timeout_1",  "The ball did not stop moving before timeout was reached_1"       )
    trigger_patterns_match_1  = ("The actual trigger pattern matches expected_1", "The actual trigger pattern did not match expected_1"             )
    check_y_z_movement_1      = ("The ball did not move too far in Y or Z_1",     "The ball moved farther than expected in Y or Z after collision_1")
    exit_game_mode_1          = ("Exited game mode_1",                            "Couldn't exit game mode_1"                                       )
    
    enter_game_mode_2         = ("Entered game mode_2",                           "Failed to enter game mode_2"                                     )
    Trigger1_exists_2         = ("Trigger1 entity was found_2",                   "Trigger1 entity was not found_2"                                 )
    Trigger2_exists_2         = ("Trigger2 entity was found_2",                   "Trigger2 entity was not found_2"                                 )
    Trigger3_exists_2         = ("Trigger3 entity was found_2",                   "Trigger3 entity was not found_2"                                 )
    Pushing_Cube_exists_2     = ("Pushing_Cube entity was found_2",               "Pushing_Cube entity was not found_2"                             )
    Target_Ball_exists_2      = ("Target_Ball entity was found_2",                "Target_Ball entity was not found_2"                              )
    threshold_setup_2         = ("The sleep threshold was setup properly_2",      "The sleep threshold was not setup properly_2"                    )
    cube_collided_with_ball_2 = ("The cube hit the ball the_2",                   "The cube did not hit the ball the_2"                             )
    ball_stopped_moving_2     = ("The ball has stopped moving before timeout_2",  "The ball did not stop moving before timeout was reached_2"       )
    trigger_patterns_match_2  = ("The actual trigger pattern matches expected_2", "The actual trigger pattern did not match expected_2"             )
    check_y_z_movement_2      = ("The ball did not move too far in Y or Z_2",     "The ball moved farther than expected in Y or Z after collision_2")
    exit_game_mode_2          = ("Exited game mode_2",                            "Couldn't exit game mode_2"                                       )
    
    enter_game_mode_3         = ("Entered game mode_3",                           "Failed to enter game mode_3"                                     )
    Trigger1_exists_3         = ("Trigger1 entity was found_3",                   "Trigger1 entity was not found_3"                                 )
    Trigger2_exists_3         = ("Trigger2 entity was found_3",                   "Trigger2 entity was not found_3"                                 )
    Trigger3_exists_3         = ("Trigger3 entity was found_3",                   "Trigger3 entity was not found_3"                                 )
    Pushing_Cube_exists_3     = ("Pushing_Cube entity was found_3",               "Pushing_Cube entity was not found_3"                             )
    Target_Ball_exists_3      = ("Target_Ball entity was found_3",                "Target_Ball entity was not found_3"                              )
    threshold_setup_3         = ("The sleep threshold was setup properly_3",      "The sleep threshold was not setup properly_3"                    )
    cube_collided_with_ball_3 = ("The cube hit the ball the_3",                   "The cube did not hit the ball the_3"                             )
    ball_stopped_moving_3     = ("The ball has stopped moving before timeout_3",  "The ball did not stop moving before timeout was reached_3"       )
    trigger_patterns_match_3  = ("The actual trigger pattern matches expected_3", "The actual trigger pattern did not match expected_3"             )
    check_y_z_movement_3      = ("The ball did not move too far in Y or Z_3",     "The ball moved farther than expected in Y or Z after collision_3")
    exit_game_mode_3          = ("Exited game mode_3",                            "Couldn't exit game mode_3"                                       )

    stop_locations_comparison = ("The stop locations are correctly ordered",      "The stop locations are not ordered correctly"                    )
# fmt: on


def RigidBody_SleepWhenBelowKineticThreshold():
    """
    Summary:
        A Pushing Cube and a Sphere are suspended over the PhysX Terrain.
        The cube entity has an initial velocity and on start will move toward the ball
        and collide with it. When the kinetic energy of the ball is below a certain threshold,
        it should stop moving.

    Level Description:
        Terrain (entity):
            PhysX Terrain component: default settings

        PushingCube (entity): Start Inactive
            Mesh component: Box shape
            PhysX Collider component: Box shape; default settings
            PhysX Rigid Body component: Initial linear velocity: 5m/s in positive X direction;
                                        gravity disabled; mass 1.0kg;

        TargetBall (entity):
            Mesh component: Sphere shape
            PhysX Collider component: Sphere shape; default settings
            PhysX Rigid Body component: Linear damping: 0.5; mass 10kg; gravity disabled; sleep threshold: 1.0;

        Trigger1 (entity):
            Box Shape component: Dimensions (0.1, 1.0, 1.0); Visible: Checked; Game View: Checked
            PhysX Collider component: Box shape; Dimensions (0.1, 1.0, 1.0);

        Trigger2 (entity):
            Box Shape component: Dimensions (0.1, 1.0, 1.0); Visible: Checked; Game View: Checked
            PhysX Collider component: Box shape; Dimensions (0.1, 1.0, 1.0);

        Trigger3 (entity):
            Box Shape component: Dimensions (0.1, 1.0, 1.0); Visible: Checked; Game View: Checked
            PhysX Collider component: Box shape; Dimensions (0.1, 1.0, 1.0);

    Expected Behavior:
        The cube entity should push the ball entity into motion.
        When the Kinetic Energy of the ball drops below the sleep threshold, it should stop moving.
        The test steps will run for each sleep threshold value (1.0, 5.0, and 10.0). The ball should stop
        before touching a specific Trigger (1.0 & Trigger3, 5.0 & Trigger 2, 10.0 & Trigger 1)

    Test Steps:
     1) Open the level
     # Steps 2-9 are repeated for each threshold value
        2) Enter game mode
        3) Find entities and setup handlers or values
        4) Activate the PushingCube entity to start movement
        5) Wait for the ball to be hit by the pushing block
        6) Wait for the ball to come to a stop in X direction
        7) Check that the triggers match the expected trigger results
        8) Check that the ball didn't move too far in Y or Z directions
        9) Exit game mode
    10) Run test_steps_per_threshold for remaining threshold values
    11) Compare the stop locations to each other
    12) Close the editor

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

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    helper.init_idle()

    TRIGGER_TIMEOUT = 5.0
    TIMEOUT_SECONDS = 2.0
    VELOCITY_TOLERANCE = 0.05
    Y_Z_BUFFER = 0.01

    class TestData:
        """
        This is a placeholder to store the values persisting the duration of the test
        """

        threshold_count = 0
        stop_locations = []

    class Entity(object):
        """
        Base class for Entities to add basic common functionality such as fetch_id, validate_existence
        """

        def __init__(self, name, test_steps_run):
            self.name = name
            self.test_steps_run = test_steps_run
            self.id = None
            self.fetch_id()
            self.validate_exist()

        def fetch_id(self):
            self.id = general.find_game_entity(self.name)

        def validate_exist(self):
            Report.critical_result(Tests.__dict__[self.name + "_exists_" + str(self.test_steps_run)], self.id.IsValid())

        def get_location(self):
            return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

    class Trigger(Entity):
        """
        Trigger entities used to tell if the Target_Ball has moved far enough/too far compared to expected distances
        """

        def __init__(self, test_steps_run, name):
            super(Trigger, self).__init__(name, test_steps_run)
            self.handler = None
            self.triggered = False
            self.triggered_by = None
            self.attach_handler()

        def on_trigger_enter(self, args):
            other_id = args[0]
            self.triggered = True
            self.triggered_by = azlmbr.entity.GameEntityContextRequestBus(
                azlmbr.bus.Broadcast, "GetEntityName", other_id
            )
            Report.info("{} was triggered by {}.".format(self.name, self.triggered_by))

        def attach_handler(self):
            self.handler = azlmbr.physics.TriggerNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnTriggerEnter", self.on_trigger_enter)

    class PushingCube(Entity):
        def __init__(self, test_steps_run, name="Pushing_Cube"):
            super(PushingCube, self).__init__(name, test_steps_run)

        def activate(self):
            azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", self.id)

    class TargetBall(Entity):
        def __init__(self, test_steps_run, sleep_threshold, name="Target_Ball"):
            super(TargetBall, self).__init__(name, test_steps_run)
            self.expected_sleep_threshold = sleep_threshold
            self.handler = None
            self.init_velocity = None
            self.grabbed_velocity = None
            self.collided_with_box = False
            self.stopped_moving_x = False
            self.trigger_timed_out = True
            self.pushing_cube_id = None
            self.setup_sleep_threshold()
            self.attach_collision_handler()

        def setup_sleep_threshold(self):
            azlmbr.physics.RigidBodyRequestBus(
                azlmbr.bus.Event, "SetSleepThreshold", self.id, self.expected_sleep_threshold
            )
            grabbed_value = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetSleepThreshold", self.id)
            Report.info("Expected sleep threshold value: {}".format(self.expected_sleep_threshold))
            Report.info("Actual value: {}".format(grabbed_value))
            Report.critical_result(
                Tests.__dict__["threshold_setup_" + str(TestData.threshold_count)],
                grabbed_value == self.expected_sleep_threshold,
            )

        def on_collision_begin(self, args):
            other_id = args[0]
            if other_id.Equal(self.pushing_cube_id):
                Report.info("Push_Box collided with ball.")
                self.init_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)
                Report.info_vector3(self.init_velocity, "Initial Velocity of {}".format(self.name))
                self.collided_with_box = True

        def attach_collision_handler(self):
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)

        def has_stopped_moving_in_x(self):
            """
            This method is used as a condition for helper.wait_for_condition()
            """
            velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)
            if abs(velocity.x) < VELOCITY_TOLERANCE:
                Report.info("{} has stopped moving in the X direction.".format(self.name))
                self.stopped_moving_x = True
                self.trigger_timed_out = False
                return True
            return False

        def check_y_z_delta(self):
            """
            Used to check that the entity has not moved too far in either the Y or Z direction
            """

            def is_within_tolerance(velocity_one_direction):
                return abs(velocity_one_direction) < Y_Z_BUFFER

            Report.info_vector3(self.init_velocity, "Initial Velocity: ")
            Report.result(
                Tests.__dict__["check_y_z_movement_" + str(TestData.threshold_count)],
                is_within_tolerance(self.init_velocity.y) and is_within_tolerance(self.init_velocity.z),
            )

    # 1) Open the level
    helper.open_level("Physics", "RigidBody_SleepWhenBelowKineticThreshold")

    def test_steps(sleep_threshold_value, trigger_pattern):
        TestData.threshold_count += 1

        # 2) Enter game mode
        helper.enter_game_mode(Tests.__dict__["enter_game_mode_" + str(TestData.threshold_count)])

        # 3) Find entities and setup handlers or values
        target_ball = TargetBall(TestData.threshold_count, sleep_threshold_value)
        pushing_cube = PushingCube(TestData.threshold_count)
        target_ball.pushing_cube_id = pushing_cube.id

        triggers = []
        for i in range(1, 4):
            triggers.append(Trigger(TestData.threshold_count, "Trigger" + str(i)))

        # 4) Activate the Pushing_Box entity to start movement
        pushing_cube.activate()

        # 5) Wait for the ball to be hit by the pushing block
        helper.wait_for_condition(lambda: target_ball.collided_with_box, TIMEOUT_SECONDS)
        Report.result(
            Tests.__dict__["cube_collided_with_ball_" + str(TestData.threshold_count)], target_ball.collided_with_box
        )

        # 6) Wait for the ball to come to a stop in X direction
        helper.wait_for_condition(target_ball.has_stopped_moving_in_x, TRIGGER_TIMEOUT)
        Report.result(
            Tests.__dict__["ball_stopped_moving_" + str(TestData.threshold_count)], target_ball.stopped_moving_x
        )

        # record the location of the stopping point
        TestData.stop_locations.append(target_ball.get_location())

        # 7) Check that the triggers match the expected trigger results
        trigger_result = (triggers[0].triggered, triggers[1].triggered, triggers[2].triggered)
        triggers_matched = trigger_result == trigger_pattern
        Report.result(Tests.__dict__["trigger_patterns_match_" + str(TestData.threshold_count)], triggers_matched)

        # 8) Check that the ball didn't move too far in Y or Z directions
        target_ball.check_y_z_delta()

        # 9) Exit game mode
        helper.exit_game_mode(Tests.__dict__["exit_game_mode_" + str(TestData.threshold_count)])

    # 10) Run test_steps_per_threshold for each threshold value
    test_steps(1.0, (True, True, False))
    test_steps(5.0, (True, False, False))
    test_steps(10.0, (False, False, False))

    # 11) Compare the stop locations to each other
    order = TestData.stop_locations[0].x > TestData.stop_locations[1].x > TestData.stop_locations[2].x
    Report.result(Tests.stop_locations_comparison, order)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_SleepWhenBelowKineticThreshold)
