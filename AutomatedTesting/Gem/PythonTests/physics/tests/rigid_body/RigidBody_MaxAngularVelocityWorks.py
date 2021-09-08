"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C13352089
# Test Case Title : Verify that maximum angular velocity interacts correctly with initial angular velocity



# fmt: off
class Tests:

    # level
    enter_game_mode       = ("Entered game mode",                "Failed to enter game mode")
    exit_game_mode        = ("Exited game mode",                 "Couldn't exit game mode")
    trigger_touch_times   = ("Bar5 rotated more than bar6",      "Bar5 did not stop more slowly than bar6")
    rotation_duration     = ("Bar5 stopped faster than bar6",    "Bar5 did not stop faster than bar6")
 
    # bar 1 
    bar1_gravity_disabled = ("Bar1 : Gravity is disabled",       "Bar1 : Gravity is not disabled")
    bar1_found            = ("Bar1 : Found entity",              "Bar1 : Entity not found")
    bar1_rotation         = ("Bar1 : Rotated on X axis",         "Bar1 : Unexpected rotation")
    bar1_angular_velocity = ("Bar1 : Expected angular velocity", "Bar1 : Unexpected angular velocity")
 
    # bar 2 
    bar2_gravity_disabled = ("Bar2 : Gravity is disabled",       "Bar2 : Gravity is not disabled")
    bar2_found            = ("Bar2 : Found entity",              "Bar2 : Entity not found")
    bar2_rotation         = ("Bar2 : Rotated on X axis",         "Bar2 : Unexpected rotation")
    bar2_angular_velocity = ("Bar2 : Expected angular velocity", "Bar2 : Unexpected angular velocity")
     
    # bar 3 
    bar3_gravity_disabled = ("Bar3 : Gravity is disabled",       "Bar3 : Gravity is not disabled")
    bar3_found            = ("Bar3 : Found entity",              "Bar3 : Entity not found")
    bar3_rotation         = ("Bar3 : Rotated on X axis",         "Bar3 : Unexpected rotation")
    bar3_angular_velocity = ("Bar3 : Expected angular velocity", "Bar3 : Unexpected angular velocity")
     
    # bar 4 
    bar4_gravity_disabled = ("Bar4 : Gravity is disabled",       "Bar4 : Gravity is not disabled")
    bar4_found            = ("Bar4 : Found entity",              "Bar4 : Entity not found")
    bar4_rotation         = ("Bar4 : Did not rotate",            "Bar4 : Unexpected rotation")
    bar4_angular_velocity = ("Bar4 : Expected angular velocity", "Bar4 : Unexpected angular velocity")
     
    # bar 5 
    bar5_gravity_disabled = ("Bar5 : Gravity is disabled",       "Bar5 : Gravity is not disabled")
    bar5_found            = ("Bar5 : Found entity",              "Bar5 : Entity not found")
    bar5_rotation         = ("Bar5 : Rotated on X axis",         "Bar5 : Unexpected rotation")
    bar5_angular_velocity = ("Bar5 : Expected angular velocity", "Bar5 : Unexpected angular velocity")
     
    # bar 6 
    bar6_gravity_disabled = ("Bar6 : Gravity is disabled",       "Bar6 : Gravity is not disabled")
    bar6_found            = ("Bar6 : Found entity",              "Bar6 : Entity not found")
    bar6_rotation         = ("Bar6 : Rotated on X axis",         "Bar6 : Unexpected rotation")
    bar6_angular_velocity = ("Bar6 : Expected angular velocity", "Bar6 : Unexpected angular velocity")
     
    # trigger for bar 5 
    bar5_trigger_found    = ("Trigger for bar5 : Found entity",   "Trigger for bar5 : Entity not found")
 
    # trigger for bar 6 
    bar6_trigger_found    = ("Trigger for bar6 : Found entity",   "Trigger for bar6 : Entity not found")

    did_not_timeout       = ("Should_wait did not time out",      "Should wait timed out")
# fmt: on


def RigidBody_MaxAngularVelocityWorks():
    """
    Summary:
    Runs an automated test to ensure maximum angular velocity and angular damping interact correctly with
        initial angular velocity

    Level Description:
        bars:
        6 PhysX Rigid Bodies with PhysX Shape Collider (Box Shape, Dimensions: X=1, Y=1, Z=5), gravity disabled,
        start asleep, positioned above terrain
            bar 1: Initial Angular Velocity = 10 rad/s, Max Angular Velocity = 5 rad/s,  Angular Damping = 0.0
            bar 2: Initial Angular Velocity = 10 rad/s, Max Angular Velocity = 10 rad/s, Angular Damping = 0.0
            bar 3: Initial Angular Velocity = 20 rad/s, Max Angular Velocity = 20 rad/s, Angular Damping = 0.0
            bar 4: Initial Angular Velocity = 20 rad/s, Max Angular Velocity = 0 rad/s,  Angular Damping = 0.0
            bar 5: Initial Angular Velocity = 20 rad/s, Max Angular Velocity = 20 rad/s, Angular Damping = 2.0
            bar 6: Initial Angular Velocity = 20 rad/s, Max Angular Velocity = 20 rad/s, Angular Damping = 5.0
        triggers:
        2 PhysX Shape Collider (Box Shape), gravity disabled, trigger enabled, start asleep
            trigger for bar 5: positioned above terrain, in front of bar 5
            trigger for bar 6: positioned above terrain, in front of bar 6

    Expected Behavior:
        bar 1: Should rotate at 5 rad/s on X axis
        bar 2: Should rotate at 10 rad/s on X axis
        bar 3: Should rotate at 20 rad/s on X axis
        bar 4: Should not rotate at all
        bar 5: Should start rotating at 20 rad/s on X axis and stop slowly (touching its trigger several times)
        bar 6: Should start rotating at 20 rad/s on X axis but stop quickly (touching its trigger 0 or very few times)

    Test Steps:
    1) Loads the level
    2) Enters game mode
    3) Set up bars values
    4) Loop for each bar
        4.1) Find the bar
        4.2) Validate ID
        4.3) Activate the bar
        4.4) Validate and log its position
        4.5) Confirm gravity is disabled for the bar
        4.6) Log bar's initial rotation
        4.7) Log bar's angular damping
        4.8) Log bar's angular velocity
        4.9) Set up trigger if the bar has one
        4.10) Wait for each bar to rotate or stop rotating
        4.11) Log current location
        4.12) Validate rotation based on expected behavior
        4.13) Destroy bar (and its trigger)
    5) Compare trigger touch times for bar5 and bar6
    6) Exit game mode
    7) Close editor
    """

    import os
    import sys
    import math
    import time



    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import azlmbr.math as lymath
    import azlmbr.legacy.general as general
    import azlmbr.bus

    # Constants
    ANGULAR_VELOCITY_TOLERANCE = 0.05
    TIME_OUT = 15.0
    ROTATION_TOLERANCE = 0.001

    def is_close(x, y, tolerance=ANGULAR_VELOCITY_TOLERANCE):
        return abs(x - y) < tolerance

    def is_angle_close(x, y):
        r = (math.sin(x) - math.sin(y)) * (math.sin(x) - math.sin(y)) + (math.cos(x) - math.cos(y)) * (
            math.cos(x) - math.cos(y)
        )
        diff = math.acos((2.0 - r) / 2.0)
        return abs(diff) <= ROTATION_TOLERANCE

    class Entity:  # Parent class for bars and triggers
        def __init__(self, name):
            self.name = name
            self.validate_ID()
            self.activate_entity()

        def validate_ID(self):
            self.id = general.find_game_entity(self.name)
            found_tuple = Tests.__dict__[self.name + "_found"]
            Report.critical_result(found_tuple, self.id.IsValid())

        def activate_entity(self):
            Report.info("Activating Entity : " + self.name)
            azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", self.id)
            general.idle_wait_frames(1)
            self.rotation_start_time = time.time()

    class Bar(Entity):
        def __init__(self, name, init_ang_velocity_on_X, max_ang_velocity, has_trigger=False):
            # 1) Validate ID, then activate Bar
            Entity.__init__(self, name)
            self.rotation_duration = 999999.9
            self.init_angular_velocity = lymath.Vector3(init_ang_velocity_on_X, 0.0, 0.0)
            self.max_angular_velocity = max_ang_velocity
            self.max_valid_velocity = min(self.max_angular_velocity, self.init_angular_velocity.GetLength())
            self.angular_damping = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetAngularDamping", self.id)
            # ) Log initial rotation
            self.init_rotation = self.get_rotation()
            # ) Verify gravity is disabled for bar
            self.validate_gravity_is_disabled()
            # ) Validate angular velocity according to max angular velocity
            self.validate_angular_velocity()
            # ) Setup trigger
            if has_trigger:
                self.touched_trigger = 0
                self.setup_trigger()

        def validate_gravity_is_disabled(self):
            gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id)
            gravity_tuple = Tests.__dict__[self.name + "_gravity_disabled"]
            Report.result(gravity_tuple, not gravity_enabled)

        def setup_trigger(self):
            self.trigger = Entity(self.name + "_trigger")
            self.trigger.handler = azlmbr.physics.TriggerNotificationBusHandler()
            self.trigger.handler.connect(self.trigger.id)
            self.trigger.handler.add_callback("OnTriggerEnter", self.count_touch_times)

        def get_angular_velocity(self):
            return azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetAngularVelocity", self.id)

        def get_rotation(self):
            return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldRotation", self.id)

        def validate_angular_velocity(self):
            general.idle_wait_frames(10)
            self.angular_velocity = self.get_angular_velocity()
            angular_velocity_tuple = Tests.__dict__[self.name + "_angular_velocity"]
            if self.angular_damping == 0:
                velocity_is_valid = is_close(self.angular_velocity.GetLength(), self.max_valid_velocity)
            else:
                velocity_is_valid = self.max_valid_velocity >= self.angular_velocity.GetLength() >= 0.0

            Report.result(angular_velocity_tuple, velocity_is_valid)

        def validate_rotation(self):
            self.rotation = self.get_rotation()
            rotated_on_x = not is_angle_close(self.init_rotation.x, self.rotation.x)
            rotated_on_y = not is_angle_close(self.init_rotation.y, self.rotation.y)
            rotated_on_z = not is_angle_close(self.init_rotation.z, self.rotation.z)
            if self.max_valid_velocity != 0:
                return rotated_on_x and not rotated_on_y and not rotated_on_z
            else:
                return not rotated_on_x and not rotated_on_y and not rotated_on_z

        def has_waited_enough(self):
            if self.angular_damping > 0:
                # Bar5 or bar6 should stop rotating to consider their movement as done
                if self.get_angular_velocity().IsZero():
                    self.rotation_duration = time.time() - self.rotation_start_time
                    return True
            elif self.angular_damping == 0:
                # Bar1, bar2, bar3 or bar4
                return True
            return False

        def count_touch_times(self, args):
            entering_entity_id = args[0]
            if entering_entity_id.Equal(self.id):
                Report.info(self.name + " touched " + self.trigger.name)
                self.touched_trigger += 1

    helper.init_idle()

    # 1) Load the level
    helper.open_level("Physics", "RigidBody_MaxAngularVelocityWorks")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Init bars values
    bars = [
        Bar(name="bar1", init_ang_velocity_on_X=10.0, max_ang_velocity=5),
        Bar(name="bar2", init_ang_velocity_on_X=10.0, max_ang_velocity=20),
        Bar(name="bar3", init_ang_velocity_on_X=20.0, max_ang_velocity=20),
        Bar(name="bar4", init_ang_velocity_on_X=20.0, max_ang_velocity=0,),
        Bar(name="bar5", init_ang_velocity_on_X=20.0, max_ang_velocity=20, has_trigger=True,),
        Bar(name="bar6", init_ang_velocity_on_X=20.0, max_ang_velocity=20, has_trigger=True,),
    ]

    # 4.10) Wait
    Report.critical_result(
        Tests.did_not_timeout,
        helper.wait_for_condition(lambda: all([bar.has_waited_enough() for bar in bars]), TIME_OUT),
    )

    # 4.12) Validate rotation
    for bar in bars:
        Report.result(Tests.__dict__["{}_rotation".format(bar.name)], bar.validate_rotation())

    # 5) Compare number of times bar5 and bar6 touched their trigger
    Report.result(Tests.trigger_touch_times, bars[4].touched_trigger > bars[5].touched_trigger)

    Report.info(bars[4].rotation_duration)
    Report.info(bars[5].rotation_duration)

    Report.result(Tests.rotation_duration, bars[4].rotation_duration > bars[5].rotation_duration)

    # 6) Exit Game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_MaxAngularVelocityWorks)
