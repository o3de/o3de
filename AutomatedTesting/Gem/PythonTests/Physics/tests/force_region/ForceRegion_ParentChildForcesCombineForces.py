"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C6090547
# Test Case Title : Check that force regions in parent and child entities work together.



# fmt: off
class Tests():
    enter_game_mode               = ("Entered game mode",                                 "Failed to enter game mode")
    find_sphere                   = ("Sphere found",                                      "Sphere not found")
    find_parent_force_region      = ("Parent Force Region found",                         "Parent Force Region not found")
    find_child_force_region       = ("Child Force Region found",                          "Child Force Region not found")
    find_trigger_box              = ("Trigger Box found",                                 "Trigger Box not found")
    sphere_gravity_disabled       = ("Sphere gravity disabled",                           "Sphere gravity not disabled")
    parent_force_region_direction = ("Parent Force Region is in positive x direction",    "Parent Force Region is not in positive x direction")
    child_force_region_direction  = ("Child Force Region is in positive y direction",     "Child Force Region is not in positive y direction")
    parent_force_on_sphere        = ("Parent Force Region applied total force on sphere", "Parent Force Region did not apply total force on sphere")
    child_force_on_sphere         = ("Child Force Region applied total force on sphere",  "Child Force Region did not apply total force on sphere")
    sphere_enters_trigger         = ("Sphere entered Trigger",                            "Sphere did not enter Trigger before Timeout")
    sphere_exits_trigger          = ("Sphere exited Trigger",                             "Sphere did not exit Trigger before Timeout")
    exit_game_mode                = ("Exited game mode",                                  "Couldn't exit game mode")
# fmt: on


def ForceRegion_ParentChildForcesCombineForces():
    """
    Summary:
    Runs an automated test to ensure that force regions in parent and child entities work together.

    Level Description:
    Parent Force Region (entity) - contains PhysX Force Region with world space force which has positive X force
                                   Magnitude with Direction (1, 0, 0) and PhysX Collider (box shape).
    Child Force Region (entity)  - contains PhysX Force Region with world space force which has positive Y force
                                   Magnitude with Direction (0, 1, 0) and PhysX Collider (box shape).
    Sphere (entity)              - contains a Sphere mesh, PhysX Collider (sphere shape) and PhysX Rigid Body.
                                   Sphere located at the low (x, y) corner of where the force regions overlap.
    Trigger Box (entity)         - contains PhysX Collider (box shape)
                                   trigger box placed in the (1, 1, 0) direction from the sphere at the opposite end of
                                   the force region overlap.
    Parent and Child force regions are placed above the terrain as two overlapping sheets.

    Expected Behavior:
    When game mode is entered, the Sphere should accelerate evenly in the positive (x, y) direction and it should move
    as much in x as it does in y. Sphere should pass through Trigger Box.

    Test Steps:
     1)  Open level
     2)  Enter game mode
     3)  Retrieve and validate entities
     4)  Make sure gravity is off from the start
     5)  Make sure the parent entity is set as the parent of the child entity in level
     6)  Make sure parent and child force regions are in correct directions
     7)  Check parent and child force regions each exert its force on sphere
     8)  Verify sphere passes through trigger box
     9)  Exit game mode
     10) Close the editor

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
    import azlmbr.math as lymath

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Constants
    TIMEOUT = 3.0
    X_DIRECTION = lymath.Vector3(1.0, 0.0, 0.0)
    Y_DIRECTION = lymath.Vector3(0.0, 1.0, 0.0)
    EXPECTED_MAGNITUDE = 100.0
    TOLERANCE = 0.1

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "ForceRegion_ParentChildForcesCombineForces")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate entities
    sphere_id = general.find_game_entity("Sphere")
    Report.critical_result(Tests.find_sphere, sphere_id.IsValid())

    parent_id = general.find_game_entity("Parent Force Region")
    Report.critical_result(Tests.find_parent_force_region, parent_id.IsValid())

    child_id = general.find_game_entity("Child Force Region")
    Report.critical_result(Tests.find_child_force_region, child_id.IsValid())

    trigger_box_id = general.find_game_entity("Trigger Box")
    Report.critical_result(Tests.find_trigger_box, trigger_box_id.IsValid())

    # 4) Make sure gravity is off from the start
    is_gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", sphere_id)
    Report.critical_result(Tests.sphere_gravity_disabled, not is_gravity_enabled)

    # 5) Make sure the parent entity is set as the parent of the child entity in level
    id = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetParentId", child_id)
    if id.Equal(parent_id):
        Report.info("parent and child force regions are in correct position")

    # 6) Make sure parent and child force regions are in correct directions
    dir_parent = azlmbr.physics.ForceWorldSpaceRequestBus(azlmbr.bus.Event, "GetDirection", parent_id)
    dir_child = azlmbr.physics.ForceWorldSpaceRequestBus(azlmbr.bus.Event, "GetDirection", child_id)
    Report.info_vector3(dir_parent, "Parent force region direction : ")
    Report.info_vector3(dir_child, "Child force region direction : ")
    Report.critical_result(Tests.parent_force_region_direction, dir_parent.IsClose(X_DIRECTION, TOLERANCE))
    Report.critical_result(Tests.child_force_region_direction, dir_child.IsClose(Y_DIRECTION, TOLERANCE))

    # 7) Check parent and child force regions each exert its force on sphere
    class NetForceMagnitude:
        parent_force_region_magnitude = 0
        child_force_region_magnitude = 0

    def on_calc_net_force(args):
        """
        args[0] - force region entity
        args[1] - entity entering
        args[2] - vector
        args[3] - magnitude
        """
        force_region_id = args[0]
        entering_entity = args[1]
        if entering_entity.Equal(sphere_id):
            if force_region_id.Equal(parent_id):
                NetForceMagnitude.parent_force_region_magnitude = args[3]
            elif force_region_id.Equal(child_id):
                NetForceMagnitude.child_force_region_magnitude = args[3]

    force_notification_handler = azlmbr.physics.ForceRegionNotificationBusHandler()
    force_notification_handler.connect(None)
    force_notification_handler.add_callback("OnCalculateNetForce", on_calc_net_force)
    helper.wait_for_condition(lambda : NetForceMagnitude.parent_force_region_magnitude != 0 and NetForceMagnitude.child_force_region_magnitude != 0, 1.0)

    Report.info("Parent Force Region Magnitude on Sphere : {}".format(NetForceMagnitude.parent_force_region_magnitude))
    Report.info("Child Force Region Magnitude on Sphere : {}".format(NetForceMagnitude.child_force_region_magnitude))
    Report.critical_result(
        Tests.parent_force_on_sphere,
        abs(EXPECTED_MAGNITUDE - NetForceMagnitude.parent_force_region_magnitude) < TOLERANCE,
    )
    Report.critical_result(
        Tests.child_force_on_sphere,
        abs(EXPECTED_MAGNITUDE - NetForceMagnitude.child_force_region_magnitude) < TOLERANCE,
    )

    # 8) Verify sphere passes through trigger box
    class Trigger:
        on_entered = False
        on_exited = False

    def on_trigger_enter(args):
        other_id = args[0]
        if other_id.Equal(sphere_id):
            Trigger.on_entered = True

    def on_trigger_exit(args):
        other_id = args[0]
        if other_id.Equal(sphere_id):
            Trigger.on_exited = True

    handler = azlmbr.physics.TriggerNotificationBusHandler()
    handler.connect(trigger_box_id)
    handler.add_callback("OnTriggerEnter", on_trigger_enter)
    handler.add_callback("OnTriggerExit", on_trigger_exit)

    # Check sphere enters trigger box
    helper.wait_for_condition(lambda: Trigger.on_entered, TIMEOUT)
    Report.critical_result(Tests.sphere_enters_trigger, Trigger.on_entered)

    # Check sphere exits trigger box
    helper.wait_for_condition(lambda: Trigger.on_exited, TIMEOUT)
    Report.result(Tests.sphere_exits_trigger, Trigger.on_exited)

    # 9) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_ParentChildForcesCombineForces)
