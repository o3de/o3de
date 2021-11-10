"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C5959759
# Test Case Title : Check that force region (sphere) exerts point force



# fmt: off
class Tests:
    enter_game_mode             = ("Entered game mode",                                  "Failed to enter game mode")
    find_cube                   = ("Entity Cube found",                                  "Cube not found")
    find_force_region           = ("Entity force region found",                          "Force region not found")
    gravity_works               = ("Cube falls",                                         "Cube did not fall")
    sphere_gravity_enabled      = ("Gravity is enabled on the cube",                     "Gravity is not enabled on the cube")
    force_region_trigger        = ("Cube entered and exited force region",               "Cube did not enter adn exit force region")
    force_calculated            = ("OnCalculateNetForce calculated",                     "OnCalculateNetForce did not get calculated")
    force_x_vector              = ("Force x vector is positive",                         "Force x vector is not positive")
    force_y_vector              = ("Force y vector is positive",                         "Force y vector is not positive")
    point_force_magnitude_value = ("Magnitude is set to 1000",                           "Magnitude is not set to 1000")
    point_force_magnitude       = ("Calculated magnitude is greater than set magnitude", "Calculated magnitude is not greater than set magnitude")
    exit_game_mode              = ("Exited game mode",                                   "Couldn't exit game mode")
# fmt: on


def ForceRegion_SphereShapedForce():
    """
    Level Setup
    The level consists of a sherical force region with a point force of 1000.
    A RigidBody cube with mass 1kg is positioned above the force region at an offset.
    On entering game mode the cube will fall into the spherical point force region.
    This should cause the cube to bounce off at considerable velocity.
    We validate the point force observed is as expected
    """
    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus

    EXPECTED_MAGNITUDE = 1000.0
    NEGATIVE_VELOCITY = -0.001
    TOLERANCE = 0.001

    # 1) Open level and enter game mode
    helper.init_idle()
    helper.open_level("Physics", "ForceRegion_SphereShapedForce")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve entities
    cube_id = general.find_game_entity("CubeRigidBody")
    Report.critical_result(Tests.find_cube, cube_id.IsValid())

    sphere_force_region = general.find_game_entity("SphereForceRegion")
    Report.critical_result(Tests.find_force_region, sphere_force_region.IsValid())

    # 3) Gravity works
    gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", cube_id)
    Report.result(Tests.sphere_gravity_enabled, gravity_enabled)
    
    def is_going_down():
        vel = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", cube_id)
        return vel.z < NEGATIVE_VELOCITY
        
    helper.wait_for_condition(is_going_down, 1.0)
    cube_linear_velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", cube_id)
    Report.info("Cube z velocity from gravity: {}".format(cube_linear_velocity.z))
    Report.result(Tests.gravity_works, cube_linear_velocity.z < NEGATIVE_VELOCITY)

    # 4) Listen to trigger events and OnCalculateNetForce notification
    class SphereForceRegion:
        enter = False
        exit = False

    def on_enter(args):
        other_id = args[0]
        if other_id.Equal(cube_id):
            Report.info("Cube touched spherical force region")
            SphereForceRegion.enter = True

    def on_exit(args):
        other_id = args[0]
        if other_id.Equal(cube_id):
            Report.info("Cube touched spherical force region")
            SphereForceRegion.exit = True

    handler = azlmbr.physics.TriggerNotificationBusHandler()
    handler.connect(sphere_force_region)
    handler.add_callback("OnTriggerEnter", on_enter)
    handler.add_callback("OnTriggerExit", on_exit)

    class NetForce:
        vector = None
        magnitude = 0

    def on_calc_net_force(args):
        """
        args[0] - force region entity
        args[1] - entity entering
        args[2] - vector
        args[3] - magnitude
        """
        entering_entity = args[1]
        if entering_entity.Equal(cube_id):
            vector = args[2]
            magnitude = args[3]
            Report.info_vector3(vector, "Net Force vector", magnitude)
            NetForce.vector = vector
            NetForce.magnitude = magnitude

    force_notification_handler = azlmbr.physics.ForceRegionNotificationBusHandler()
    force_notification_handler.connect(None)
    force_notification_handler.add_callback("OnCalculateNetForce", on_calc_net_force)

    def force_region_enter_and_exit():
        return SphereForceRegion.enter and SphereForceRegion.exit

    helper.wait_for_condition(force_region_enter_and_exit, 3.0)

    # 5) Report results
    Report.result(Tests.force_region_trigger, force_region_enter_and_exit())
    force_region_magnitude = azlmbr.physics.ForcePointRequestBus(azlmbr.bus.Event, "GetMagnitude", sphere_force_region)
    Report.info("Force region magnitude = {}".format(force_region_magnitude))
    # explicit check that the value is as expected
    # set at level creation
    Report.result(Tests.point_force_magnitude_value, force_region_magnitude == EXPECTED_MAGNITUDE)
    # prevent the test from hanging if the force vector is not set
    if NetForce.vector:
        Report.success(Tests.force_calculated)
        Report.result(Tests.force_x_vector, NetForce.vector.x > 0)
        Report.result(Tests.force_y_vector, NetForce.vector.z > 0)
    else:
        Report.failure(Tests.force_calculated)
    outcome = abs(NetForce.magnitude - force_region_magnitude) < TOLERANCE
    Report.result(Tests.point_force_magnitude, outcome)

    # 6) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_SphereShapedForce)
