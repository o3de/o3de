"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5932041
# Test Case Title : Check that force region exerts local space force on rigid bodies

# Sphere drops and is acted upon in an upward and positive x-ward direction by a force
# with a magnitude close to the assigned force region magnitude when it reaches the force region.


# fmt: off
class Tests:
    enter_game_mode                 = ("Entered game mode",                             "Failed to enter game mode")
    find_sphere                     = ("Sphere entity found",                           "Sphere entity not found")
    find_box                        = ("Box entity found",                              "Box entity not found")
    sphere_pos_found                = ("Sphere position found",                         "Sphere position not found")
    sphere_velocity_found           = ("Sphere has downward velocity",                  "Sphere does not have downward velocity")
    box_pos_found                   = ("Box position found",                            "Box position not found")
    force_region_entered            = ("Force region entered",                          "Force region never entered")
    force_x_component_detected      = ("Force x-component detected on the Sphere",      "Force x-component was not detected on the Sphere")
    force_z_component_detected      = ("Force z-component detected on the Sphere",      "Force z-component was not detected on the Sphere")
    force_y_component_not_detected  = ("Force y-component not detected on the Sphere",  "Force y-component was detected on the Sphere")
    force_magnitude_detected        = ("Force magnitude detected on the Sphere",        "Force magnitude was not detected on the Sphere")
    exit_game_mode                  = ("Exited game mode",                              "Couldn't exit game mode")
# fmt: on


def ForceRegion_LocalSpaceForceOnRigidBodies():
    """
    Summary:
    Runs an automated test to ensure that when a rigid body enters a force region a local space force is exerted.

    Level Description:
    Box (entity) - suspended above terrain at 45 degree angle with Direction Z = 1.0, magnitude = 1000, 
        and gravity disabled; contains box mesh, PhysX Collider, and PhysX Force Region
    Sphere (entity) - suspended above Box with slight x-axis offset, initial velocity in negative z direction, 
        gravity disabled; contains sphere mesh, PhysX Rigid Body, PhysX Collider

    Expected Behavior:
    When game mode is entered, the Sphere entity will travel torward the terrain.
    It will reach the force region of the Box entity and be imbued with a net force in the x and z directions.

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Retrieve entities
     4) Log Sphere velocity and positions for Sphere and Box
     5) Set up handler and variables
     6) Wait for force region entry or time out
     7) Look for positive x, zero y, positive z force with a valid magnitude, and report findings
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

    import azlmbr.legacy.general as general
    import azlmbr.bus

    # Constants
    TIMEOUT = 2.0
    TOLERANCE = 1
    MAGNITUDE = 1000 # Magnitude assigned to the force region
    FORCE_Y_TOLERANCE = sys.float_info.epsilon

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "ForceRegion_LocalSpaceForceOnRigidBodies")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve entities
    sphere_id = general.find_game_entity("Sphere")
    box_id = general.find_game_entity("Box")

    Report.critical_result(Tests.find_sphere, sphere_id.isValid())
    Report.critical_result(Tests.find_box, box_id.isValid())

    # 4) Log Sphere velocity and positions for Sphere and Box
    sphere_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", sphere_id)

    box_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", sphere_id)

    # Validate and print positions and sphere velocity
    sphere_pos_found = sphere_pos is not None and sphere_pos.x != 0 and sphere_pos.y != 0 and sphere_pos.z != 0
    Report.critical_result(Tests.sphere_pos_found, sphere_pos_found)
    Report.info_vector3(sphere_pos, "Sphere Position:")

    sphere_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", sphere_id)
    Report.critical_result(Tests.sphere_velocity_found, sphere_velocity.z < 0)
    Report.info_vector3(sphere_velocity, "Sphere Initial Velocity:")

    box_pos_found = box_pos is not None and box_pos.x != 0 and box_pos.y != 0 and box_pos.z != 0
    Report.critical_result(Tests.box_pos_found, box_pos_found)
    Report.info_vector3(box_pos, "Box Position:")

    # 5) Set up handler and variables
    class RegionData:
        force_region_entered = False
        force_vector = None
        force_magnitude = 0

    # Force Region Event Handler
    def on_force_region_entered(args):
        region_id = args[0]
        object_id = args[1]
        force_vector = args[2]
        force_magnitude = args[3]
        if region_id.Equal(box_id) and object_id.Equal(sphere_id):
            if not RegionData.force_region_entered:
                RegionData.force_region_entered = True
                RegionData.force_vector = force_vector
                RegionData.force_magnitude = force_magnitude
                Report.info("Force Region entered")

    # Assign the handler
    handler = azlmbr.physics.ForceRegionNotificationBusHandler()
    handler.connect(None)
    handler.add_callback("OnCalculateNetForce", on_force_region_entered)

    # 6) Wait for force region entry or time out
    helper.wait_for_condition(lambda: RegionData.force_region_entered, TIMEOUT)

    # 7) Look for positive x, positive z force with a valid magnitude, and report findings
    force_x_component_detected = RegionData.force_vector.x > 0
    force_z_component_detected = RegionData.force_vector.z > 0
    force_y_component_detected = abs(RegionData.force_vector.y) > FORCE_Y_TOLERANCE
    force_magnitude_detected = abs(RegionData.force_magnitude - MAGNITUDE) < TOLERANCE

    Report.result(Tests.force_region_entered, RegionData.force_region_entered)
    Report.info_vector3(RegionData.force_vector, "Force vector detected", RegionData.force_magnitude)
    Report.result(Tests.force_x_component_detected, force_x_component_detected)
    Report.result(Tests.force_z_component_detected, force_z_component_detected)
    Report.result(Tests.force_y_component_not_detected, not force_y_component_detected)
    Report.result(Tests.force_magnitude_detected, force_magnitude_detected)

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_LocalSpaceForceOnRigidBodies)
