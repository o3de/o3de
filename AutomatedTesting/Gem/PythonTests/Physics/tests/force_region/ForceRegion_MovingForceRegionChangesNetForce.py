"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5968760
# Test Case Title : Check moving force region changes net force



# fmt: off
class Tests():
    enter_game_mode   = ("Entered game mode",          "Failed to enter game mode")
    find_sphere       = ("SphereRigidBody found",      "SphereRigidBody not found")
    find_force_region = ("ForceRegionBox is found",    "ForceRegionBox is not found")
    sphere_dropped    = ("Sphere dropped down",        "Sphere did not drop down") 
    sphere_bounced    = ("Sphere bounced to its left", "Sphere did not bounce to its left")
    exit_game_mode    = ("Exited game mode",           "Couldn't exit game mode")
# fmt: on


def ForceRegion_MovingForceRegionChangesNetForce():

    """
    Summary:
    Check moving force region changes net force.

    Level Description:
    The SphereRigidBody entity is placed above the ForceRegionBox entity.
    ForceRegionBox (entity) - Entity with PhysX Force Region, Mesh, PhysX Collider
    SphereRigidBody (entity) - Entity with PhysX Rigid body, Mesh and collider components

    Expected Behavior:
    We are checking if the ball falls down initially and then moving the force region to the right to verify if the
    ball bounces off to the left when it collides with the force region.

    Test Steps:
     1)  Open level
     2)  Enter game mode
     3)  Retrieve and validate entities
     4)  Get the initial position of the Sphere (rigid body)
     5)  Move the object to right (X - direction) and rotate in Y - direction
     6)  Check if the ball is falling down
     7)  Add force region notification handler
     8)  Wait till the ball enters the force region
     9)  Check if the ball has bounced and moved left
     10) Exit game mode
     11) Close the editor


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
    TIMEOUT = 3.0  # wait a maximum of 3 seconds
    SPHERE_RADIUS = 0.5
    TRANSLATION_OFFSET = 0.2
    ROTATION_OFFSET = -0.005

    class Sphere:
        id = None
        initial_position = None
        current_position = None
        z_at_collision = None
        in_force_region = False
        bounced = False

    class ForceRegion:
        id = None
        translation_position = None
        rotation_position = None

    def sphere_bounced():
        Sphere.current_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", Sphere.id)
        bounced_up = Sphere.current_position.z > Sphere.z_at_collision + SPHERE_RADIUS
        bounced_left = Sphere.current_position.x < Sphere.initial_position.x
        Sphere.bounced = bounced_left and bounced_up
        return Sphere.bounced

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "ForceRegion_MovingForceRegionChangesNetForce")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate entities
    Sphere.id = general.find_game_entity("SphereRigidBody")
    Report.critical_result(Tests.find_sphere, Sphere.id.IsValid())
    ForceRegion.id = general.find_game_entity("ForceRegionBox")
    Report.critical_result(Tests.find_force_region, ForceRegion.id.IsValid())

    # 4) Get the initial position of the Sphere (rigid body)
    Sphere.initial_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", Sphere.id)

    # 5) Move the object to right (X - direction) and rotate in Y - direction
    ForceRegion.translation_position = azlmbr.components.TransformBus(
        azlmbr.bus.Event, "GetWorldTranslation", ForceRegion.id
    )
    azlmbr.components.TransformBus(
        azlmbr.bus.Event, "SetWorldX", ForceRegion.id, (ForceRegion.translation_position.x + TRANSLATION_OFFSET)
    )
    # Rotation in y direction anti clockwise
    ForceRegion.rotation_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldRotation", ForceRegion.id)
    azlmbr.components.TransformBus(
        azlmbr.bus.Event, "RotateAroundLocalY", ForceRegion.id, (ForceRegion.rotation_position.y + ROTATION_OFFSET)
    )
    Report.info("The force region has been repositioned")

    # 6) Check if the ball is falling down
    Sphere.current_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", Sphere.id)
    sphere_dropped = Sphere.current_position.z < (Sphere.initial_position.z + SPHERE_RADIUS)
    Report.critical_result(Tests.sphere_dropped, sphere_dropped)

    # 7) Add force region notification handler
    def on_force_region_entered(args):
        region_id = args[0]
        object_id = args[1]
        if region_id.Equal(ForceRegion.id) and object_id.Equal(Sphere.id):
            if not Sphere.in_force_region:
                Sphere.in_force_region = True
                Report.info("Force Region entered")

    force_notification_handler = azlmbr.physics.ForceRegionNotificationBusHandler()
    force_notification_handler.connect(None)
    force_notification_handler.add_callback("OnCalculateNetForce", on_force_region_entered)

    # 8) Wait till the ball enters the force region
    helper.wait_for_condition(lambda: Sphere.in_force_region, TIMEOUT)
    # sphere z position when it entered force region
    Sphere.z_at_collision = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", Sphere.id).z

    # 9) Check if the ball has bounced and moved left
    # wait frames till the ball bounces
    helper.wait_for_condition(sphere_bounced, TIMEOUT)
    Report.result(Tests.sphere_bounced, Sphere.bounced)
    
    # 10) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_MovingForceRegionChangesNetForce)
