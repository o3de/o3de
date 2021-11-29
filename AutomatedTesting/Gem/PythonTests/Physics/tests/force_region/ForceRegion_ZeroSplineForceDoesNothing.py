"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C6090555
# Test Case Title : Check that force region exerts spline follow force on rigid bodies(negative test)



# fmt: off
class Tests():
    enter_game_mode             = ("Entered game mode",                     "Failed to enter game mode")
    find_sphere                 = ("Sphere entity found",                   "Sphere entity not found")
    find_force_region           = ("Force region found",                    "Force region not found")
    find_triggers               = ("All triggers are found",                "All triggers are not found")
    sphere_fell                 = ("The sphere fell",                       "The sphere did not fall")
    sphere_entered_force_region = ("The sphere entered the force region",   "The sphere did not enter the force region before timeout")
    sphere_exited_force_region  = ("The sphere exited the force region",    "The sphere did not exit the force region before timeout")
    sphere_drops_force_region   = ("Sphere drops through the force region", "Sphere did not drop through the force region due to spline force")
    exit_game_mode              = ("Exited game mode",                      "Couldn't exit game mode")
# fmt: on


def ForceRegion_ZeroSplineForceDoesNothing():
    """
    Summary:
    Runs an automated test to ensure that a PhysX force region exerts spline follow force on rigid bodies(negative test)

    Level Description:
    A sphere entity is positioned above a force region entity
    The sphere has a PhysX collider (sphere) and rigidbody component with default values

    The force region entity has a PhysX collider (Box) and force region with a 'spline follow' force.
    It also has a spline component with 4 nodes. Each node is connected linearly in the following pattern:
            ___[0]
        [1] ___
            ___ [2]
        [3]

    There are 4 trigger entities, each with a PhysX collider (sphere). They are positioned at each node.

    Expected Behavior:
    The Sphere drops through the force region as though spline follow force does not exist.

    Test Steps:
    1) Open level
    2) Enter game mode
    3) Retrieve and Validate entities
    4) Get position of spline and sphere
    5) Wait till the sphere drops
    6) Get z position of sphere when it enters and exits from trigger area
    7) Verify sphere drops through force region without spline force effect
    8) Exit game mode
    9) Close the editor

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
    import azlmbr.bus

    # Constants
    TIMEOUT = 3
    CLOSE_ENOUGH = 0.001

    class Sphere:
        id = None
        start_position_z = None
        fell = False
        entered_force_region = False
        exited_force_region = False

    def on_trigger_enter(args):
        other_id = args[0]
        if other_id.Equal(Sphere.id):
            Sphere.entered_force_region = True

    def on_trigger_exit(args):
        other_id = args[0]
        if other_id.Equal(Sphere.id):
            Sphere.exited_force_region = True

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "ForceRegion_ZeroSplineForceDoesNothing")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and Validate entities
    Sphere.id = general.find_game_entity("Sphere")
    Report.critical_result(Tests.find_sphere, Sphere.id.IsValid())

    force_region_id = general.find_game_entity("Force Region")
    Report.critical_result(Tests.find_force_region, force_region_id.IsValid())

    all_triggers = ("Trigger0", "Trigger1", "Trigger2", "Trigger3")
    all_triggers_found = True
    for trigger in all_triggers:
        trigger_id = general.find_game_entity(trigger)
        if not trigger_id.IsValid():
            all_triggers_found = False
    Report.critical_result(Tests.find_triggers, all_triggers_found)

    # 4) Get z position of spline and sphere
    # All triggers are arranged at each node in spline. Getting z position of Trigger3 is same as z position of spline
    spline_z_position = azlmbr.components.TransformBus(
        azlmbr.bus.Event, "GetWorldZ", general.find_game_entity("Trigger3")
    )
    Report.info("Spline z position is : {}".format(spline_z_position))
    Sphere.start_position_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", Sphere.id)
    Report.info("Sphere z position is : {}".format(Sphere.start_position_z))

    # 5) Wait till the sphere drops
    def sphere_fell():
        if not Sphere.fell:
            sphere_position_z = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", Sphere.id)
            if sphere_position_z < (Sphere.start_position_z - CLOSE_ENOUGH):
                Report.info("Sphere position is lower than the starting position now")
                Sphere.fell = True
        return Sphere.fell

    helper.wait_for_condition(sphere_fell, TIMEOUT)
    Report.critical_result(Tests.sphere_fell, Sphere.fell)

    # 6) Get position of sphere when it enters and exits from trigger area
    # Wait for ball to enter force region
    handler = azlmbr.physics.TriggerNotificationBusHandler()
    handler.connect(force_region_id)
    handler.add_callback("OnTriggerEnter", on_trigger_enter)

    helper.wait_for_condition(lambda: Sphere.entered_force_region, TIMEOUT)
    Report.critical_result(Tests.sphere_entered_force_region, Sphere.entered_force_region)
    sphere_start_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", Sphere.id)
    Report.info_vector3(sphere_start_position, "Sphere start position in Force Region")

    # Wait for ball to exit force region
    handler.add_callback("OnTriggerExit", on_trigger_exit)
    helper.wait_for_condition(lambda: Sphere.exited_force_region, TIMEOUT)
    Report.critical_result(Tests.sphere_exited_force_region, Sphere.exited_force_region)
    sphere_end_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", Sphere.id)
    Report.info_vector3(sphere_end_position, "Sphere end position in Force Region")

    # 7) Verify sphere drops through force region without spline force effect
    if (
        ((sphere_start_position.x - sphere_end_position.x) < CLOSE_ENOUGH) and
        ((sphere_start_position.y - sphere_end_position.y) < CLOSE_ENOUGH) and
        (sphere_end_position.z < (spline_z_position - CLOSE_ENOUGH))
    ):
        Sphere.sphere_drops = True

    Report.critical_result(Tests.sphere_drops_force_region, Sphere.sphere_drops)

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_ZeroSplineForceDoesNothing)
