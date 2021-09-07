"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C5932045
# Test Case Title : Check that force region exerts spline follow force on rigid bodies



# fmt: off
class Tests():
    enter_game_mode             = ("Entered game mode",                   "Failed to enter game mode")
    find_sphere                 = ("Sphere entity found",                 "Sphere entity not found")
    find_force_region           = ("Force region found",                  "Force region not found")
    find_trigger_0              = ("Trigger0 entity found",               "Trigger0 entity not found")
    find_trigger_1              = ("Trigger1 entity found",               "Trigger1 entity not found")
    find_trigger_2              = ("Trigger2 entity found",               "Trigger2 entity not found")
    find_trigger_3              = ("Trigger3 entity found",               "Trigger3 entity not found")
    triggers_positioned_apart   = ("All triggers were positioned apart",  "All triggers were not positioned apart")
    sphere_fell                 = ("The sphere fell",                     "The sphere did not fall")
    sphere_entered_force_region = ("The sphere entered the force region", "The sphere did not enter the force region before timeout")
    sphere_reached_trigger0     = ("The sphere reached Trigger0",         "The sphere did not reach Trigger0 before timeout")
    sphere_reached_trigger1     = ("The sphere reached Trigger1",         "The sphere did not reach Trigger1 before timeout")
    sphere_reached_trigger2     = ("The sphere reached Trigger2",         "The sphere did not reach Trigger2 before timeout")
    sphere_reached_trigger3     = ("The sphere reached Trigger3",         "The sphere did not reach Trigger3 before timeout")
    exit_game_mode              = ("Exited game mode",                    "Couldn't exit game mode")
# fmt: on


def ForceRegion_SplineForceOnRigidBodies():
    """
    Summary:
    Runs an automated test to ensure that a PhysX force region exerts spline follow force on rigid bodies

    Level Description:
    A sphere entity is positioned above a force region entity
    The sphere has a PhysX collider (sphere) and rigidbody component with default values

    The force region entity has a PhysX collider (Box) and force region with a 'spline follow' force.
    It also has a spline component with 4 nodes. Each node is connected linearly in the following pattern:
        [0]___
          ___[1]
        [2]___
             [3]

    There are 4 trigger entities, each with a PhysX collider (sphere). They are positioned at each node.

    Expected Behavior:
    The sphere will fall into the force region and begin to follow the spline. It will visit each node in order.

    Test Steps:
    1) Open level
    2) Enter game mode
    3) Find entities
    4) Verify triggers are apart
    5) Drop the sphere
    6) Wait for sphere to complete path
    7) Exit game mode
    8) Close the editor

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

    import itertools
    import azlmbr
    import azlmbr.legacy.general as general
    import azlmbr.bus as bus

    TIMEOUT = 5
    MIN_TRIGGER_DISTANCE = 2

    class Sphere:
        def __init__(self, name):
            self.name = name
            self.id = general.find_game_entity(name)

        def get_velocity(self):
            return azlmbr.physics.RigidBodyRequestBus(bus.Event, "GetLinearVelocity", self.id)

        def is_falling(self):
            return self.get_velocity().z < 0.0

    class Trigger:
        def __init__(self, name, valid_test, triggered_test):
            self.name = name
            self.id = general.find_game_entity(name)
            self.valid_test = valid_test
            self.triggered_test = triggered_test
            self.triggered = False
            self.create_handler()

        def get_position(self):
            return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

        def on_trigger_enter(self, args):
            self.triggered = True

        def create_handler(self):
            self.handler = azlmbr.physics.TriggerNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnTriggerEnter", self.on_trigger_enter)

    def are_apart(position1, position2, distance):
        return position1.GetDistance(position2) >= distance

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "ForceRegion_SplineForceOnRigidBodies")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Find entities
    sphere = Sphere("Sphere")
    Report.critical_result(Tests.find_sphere, sphere.id.IsValid())

    force_region = Trigger("ForceRegion", Tests.find_force_region, Tests.sphere_entered_force_region)
    trigger0 = Trigger("Trigger0", Tests.find_trigger_0, Tests.sphere_reached_trigger0)
    trigger1 = Trigger("Trigger1", Tests.find_trigger_1, Tests.sphere_reached_trigger1)
    trigger2 = Trigger("Trigger2", Tests.find_trigger_2, Tests.sphere_reached_trigger2)
    trigger3 = Trigger("Trigger3", Tests.find_trigger_3, Tests.sphere_reached_trigger3)
    all_triggers = (force_region, trigger0, trigger1, trigger2, trigger3)

    for trigger in all_triggers:
        Report.critical_result(trigger.valid_test, trigger.id.IsValid())

    # 4) Verify triggers are apart
    all_triggers_apart = True
    for combination in itertools.combinations(all_triggers, 2):
        if not are_apart(combination[0].get_position(), combination[1].get_position(), MIN_TRIGGER_DISTANCE):
            all_triggers_apart = False

    Report.critical_result(Tests.triggers_positioned_apart, all_triggers_apart)

    # 5) Drop the sphere
    Report.result(Tests.sphere_fell, helper.wait_for_condition(sphere.is_falling, TIMEOUT))

    # 6) Wait for sphere to complete path
    for trigger in all_triggers:
        Report.result(trigger.triggered_test, helper.wait_for_condition(lambda: trigger.triggered, TIMEOUT))

    # 7) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_SplineForceOnRigidBodies)
