"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C4976200
# Test Case Title : Verify that with higher angular damping, the object in rotation comes to rest faster


# fmt: off
class Tests:
    enter_game_mode              = ("Entered game mode",                                     "Failed to enter game mode")
    low_find_bar                 = ("Find bar low",                                          "Failed to find bar low")
    medium_find_bar              = ("Find bar medium",                                       "Failed to find bar medium")
    high_find_bar                = ("Find bar high",                                         "Failed to find bar high")
    low_no_translation_change    = ("The low bar had no tanslation change",                  "The low bar did have a translation change")
    medium_no_translation_change = ("The medium bar had no tanslation change",               "The medium bar did have a translation change")
    high_no_translation_change   = ("The high bar had no tanslation change",                 "The high bar did have a translation change")
    low_trigger_a                = ("LowBarTriggerA triggered by the low bar",               "LowBarTriggerA not triggered by the low bar")
    low_trigger_b                = ("LowBarTriggerB not triggered by the low bar",           "LowBarTriggerB triggered by the low bar")
    medium_trigger_a             = ("MediumTriggerA triggered by the medium bar",            "MediumTriggerA not triggered by the medium bar")
    medium_trigger_b             = ("MediumTriggerB not triggered by the medium bar",        "MediumTriggerB triggered by the medium bar")
    high_trigger_a               = ("HighTriggerA not triggered by the high bar",            "HighTriggerA triggered by the high bar")
    high_trigger_b               = ("HighTriggerB not triggered by the high bar",            "HighTriggerB triggered by the high bar")
    low_rotation                 = ("The low bar rotated only on the x axis",                "The low bar did not rotate only on the x axis")
    medium_rotation              = ("The medium bar rotated only on the x axis",             "The medium bar did not only rotate on the x axis")
    high_rotation                = ("The high bar did not rotate",                           "The high bar did rotate")
    comparative_rotation         = ("The rotation decreased with increased angular damping", "The rotation din not decrease with increased angular damping")
    timeout                      = ("All spheres rotation IsClose to 0 before timeout",      "All spheres rotation are not IsClose to 0 before timeout")
    exit_game_mode               = ("Exited game mode",                                      "Couldn't exit game mode")
# fmt: on


def RigidBody_AngularDampingAffectsRotation():
    """
    The level consists of a PhysX Terrain component that is not interacted with (per the test case)
    3 PhysX colliders with shape box, PhysX rigid bodies physics, and mesh with shape box.
    In the test we use the term bar - this means a box shape changed to be a rectangle, think thin plank.
    The bar shape is chosen for debugging, it is easy to see rotation.
    LowBar has 2 associated triggers LowBarTriggerA and LowBarTriggerB
        - LowBarTriggerA should be tripped to measure the rotation of the bar
        - LowBarTriggerB should not be tripped to measure that the rotation does not start increasing unexpectedly
    MediumBar has 2 associated triggers
        - MediumBarTriggerA should be tripped to measure the rotation of the bar
        - MediumBarTriggerB should not be tripped to measure that the rotation does not start increasing unexpectedly
    HighBar has 2 associated triggers
        - HighBarTriggerA is above the bar and should not be tripped
        - HighBarTriggerB is below the bar and should not be tripped

    1) Open level
    2) Enter game mode
    3) Find bars, set bars attributes, and measure initial position
    4) Awaken bars
    5) Measure
    6) Report results
    """
    # Setup path
    import os
    import sys
    import math



    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import AngleHelper

    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr.math as lymath

    class Bar:
        def __init__(
            self, name, initial_angular_velocity, angular_damping, triggers, find_bar_test, no_translation_change_test
        ):
            self.name = name
            self.initial_angular_velocity = initial_angular_velocity
            self.angular_damping = angular_damping
            self.entity = None
            self.initial_position = None
            self.final_position = None
            self.initial_rotation = None
            self.final_rotation = None
            self.timeout = False
            self.triggers = triggers
            self.find_bar_test = find_bar_test
            self.no_translation_change_test = no_translation_change_test

        def find_trigger(self, name):
            return next(t for t in self.triggers if t.name == name)

        def __str__(self):
            return """
            name = {}
            angular_damping = {}
            timeout = {}
            """.format(
                self.name, self.angular_damping, self.timeout
            )

        def report(self):
            Report.info(self)
            Report.info(
                "Initial angular velocity {}  for {}".format(self.initial_angular_velocity.GetLength(), self.name)
            )
            Report.info_vector3(self.initial_position, "Initial position {}".format(self.name))
            if self.final_position:
                Report.info_vector3(self.final_position, "Final position {}".format(self.name))
            Report.info_vector3(self.initial_rotation, "Initial rotation {}".format(self.name))
            if self.final_rotation:
                Report.info_vector3(self.final_rotation, "Final rotation {}".format(self.name))
            for trigger in self.triggers:
                Report.info(trigger)

    class Trigger:
        def __init__(self, name):
            self.name = name
            self.entity = None
            self.handler = None
            self.triggering_entity = None
            self.triggered = False

        def on_trigger(self, args):
            self.triggered = True
            self.triggering_entity = args[0]
            Report.info("{} was triggered by {}".format(self.name, self.triggering_entity_name()))

        def __str__(self):
            return """
            name = {}
            triggering entity name = {}
            triggered = {}
            """.format(
                self.name, self.triggering_entity_name(), self.triggered
            )

        def triggering_entity_name(self):
            if self.triggering_entity:
                return azlmbr.entity.GameEntityContextRequestBus(
                    azlmbr.bus.Broadcast, "GetEntityName", self.triggering_entity
                )
            return None

    INITIAL_ANGULAR_VELOCITY = lymath.Vector3(5.0, 0.0, 0.0)
    ZERO_ANGULAR_VELOCITY = lymath.Vector3(0.0, 0.0, 0.0)
    TOLERANCE = 0.01
    TIMEOUT = 2.0
    LOW_TRIGGER_A = "LowBarTriggerA"
    LOW_TRIGGER_B = "LowBarTriggerB"
    MEDIUM_TRIGGER_A = "MediumBarTriggerA"
    MEDIUM_TRIGGER_B = "MediumBarTriggerB"
    HIGH_TRIGGER_A = "HighBarTriggerA"
    HIGH_TRIGGER_B = "HighBarTriggerB"

    # bars
    # fmt: off
    low_damping = Bar(
        "LowBar",
        INITIAL_ANGULAR_VELOCITY,
        5.0,
        [Trigger(LOW_TRIGGER_A), Trigger(LOW_TRIGGER_B)],
        Tests.low_find_bar,
        Tests.low_no_translation_change,
    )
    medium_damping = Bar(
        "MediumBar",
        INITIAL_ANGULAR_VELOCITY,
        10.0,
        [Trigger(MEDIUM_TRIGGER_A), Trigger(MEDIUM_TRIGGER_B)],
        Tests.medium_find_bar,
        Tests.medium_no_translation_change,
    )
    high_damping = Bar(
        "HighBar",
        INITIAL_ANGULAR_VELOCITY,
        100.0,
        [Trigger(HIGH_TRIGGER_A), Trigger(HIGH_TRIGGER_B)],
        Tests.high_find_bar,
        Tests.high_no_translation_change,
    )
    # fmt: on

    bars = [low_damping, medium_damping, high_damping]

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "RigidBody_AngularDampingAffectsRotation")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)
    # 3) Find bars, set bars attributes, and measure initial position
    for bar in bars:
        bar.entity = general.find_game_entity(bar.name)
        Report.critical_result(bar.find_bar_test, bar.entity.IsValid())
        azlmbr.physics.RigidBodyRequestBus(
            azlmbr.bus.Event, "SetAngularVelocity", bar.entity, bar.initial_angular_velocity
        )
        azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetAngularDamping", bar.entity, bar.angular_damping)
        general.idle_wait_frames(1)  # wait one frame for changes to apply
        bar.initial_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", bar.entity)
        bar.initial_rotation = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldRotation", bar.entity)
        # add handler for each trigger
        for trigger in bar.triggers:
            trigger.entity = general.find_game_entity(trigger.name)
            trigger.handler = azlmbr.bus.NotificationHandler("TriggerNotificationBus")
            trigger.handler.connect(trigger.entity)
            trigger.handler.add_callback("OnTriggerEnter", trigger.on_trigger)

    def all_bars_stopped_rotating():
        bar_results = [False for bar in bars]
        for i, bar in enumerate(bars):
            bar_angular_velocity = azlmbr.physics.RigidBodyRequestBus(
                azlmbr.bus.Event, "GetAngularVelocity", bar.entity
            )
            if bar_angular_velocity.IsClose(ZERO_ANGULAR_VELOCITY, TOLERANCE):
                bar.final_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", bar.entity)
                bar.final_rotation = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldRotation", bar.entity)
                bar_results[i] = True
        return all(bar_results)

    # 4) Awaken bars
    for bar in bars:
        azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "ForceAwake", bar.entity)

    # 5) Measure
    if not helper.wait_for_condition(all_bars_stopped_rotating, TIMEOUT):
        bar.timeout = True

    # 6) Report results
    no_timeout = True
    for bar in bars:
        if bar.timeout:
            Report.info(
                "Timeout occurred. bar with damping = {} and initial angular velocity = {} did not stop rotating in the timeout of {}".format(
                    bar.angular_damping, bar.initial_angular_velocity, TIMEOUT
                )
            )
            no_timeout = False

    # fast fail if timeout occurred, comparisons will be meaningless and all info is in log to determine which bar(s) timed out
    Report.critical_result(Tests.timeout, no_timeout)
    # no translation
    for bar in bars:
        Report.result(bar.no_translation_change_test, bar.initial_position.IsClose(bar.final_position, TOLERANCE))
    # rotation (comparisons are safe because our test setup and triggers ensure the bar rotates < 360 degrees)
    # if a bar rotates to exactly the expected position + n(360), the triggers will fail the test
    outcome = (
        low_damping.final_rotation.x > low_damping.initial_rotation.x
        and AngleHelper.is_angle_close_deg(low_damping.initial_rotation.y, low_damping.final_rotation.y, TOLERANCE)
        and AngleHelper.is_angle_close_deg(low_damping.initial_rotation.z, low_damping.final_rotation.z, TOLERANCE)
    )
    Report.result(Tests.low_rotation, outcome)
    outcome = (
        medium_damping.final_rotation.x > medium_damping.initial_rotation.x
        and AngleHelper.is_angle_close_deg(
            medium_damping.initial_rotation.y, medium_damping.final_rotation.y, TOLERANCE
        )
        and AngleHelper.is_angle_close_deg(
            medium_damping.initial_rotation.z, medium_damping.final_rotation.z, TOLERANCE
        )
    )
    Report.result(Tests.medium_rotation, outcome)
    # we do not need worry about is_angle_close here because we expect this to not change at all
    # if it rotates 360, the triggers will fail the test
    Report.result(Tests.high_rotation, high_damping.initial_rotation.IsClose(high_damping.final_rotation, TOLERANCE))
    # comparative rotation (comparisons are safe because our test setup and triggers ensure the bar rotates < 360 degrees)
    # if the bars rotate to exactly the expected position + n(360), the triggers will fail the test
    Report.result(
        Tests.comparative_rotation,
        high_damping.final_rotation.x < medium_damping.final_rotation.x < low_damping.final_rotation.x,
    )
    # triggers (quantitative measure of rotation)
    Report.result(Tests.low_trigger_a, low_damping.find_trigger(LOW_TRIGGER_A).triggered)
    Report.result(Tests.low_trigger_b, not low_damping.find_trigger(LOW_TRIGGER_B).triggered)
    Report.result(Tests.medium_trigger_a, medium_damping.find_trigger(MEDIUM_TRIGGER_A).triggered)
    Report.result(Tests.medium_trigger_b, not medium_damping.find_trigger(MEDIUM_TRIGGER_B).triggered)
    Report.result(Tests.high_trigger_a, not high_damping.find_trigger(HIGH_TRIGGER_A).triggered)
    Report.result(Tests.high_trigger_b, not high_damping.find_trigger(HIGH_TRIGGER_B).triggered)

    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_AngularDampingAffectsRotation)
