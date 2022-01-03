"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C6090546
# Test Case Title : Check that a force region slice can be saved and instantiated



# fmt: off
class Tests():
    enter_game_mode   = ("Entered game mode",            "Failed to enter game mode")
    find_sphere       = ("SphereRigidBody found",        "SphereRigidBody not found")
    find_force_region = ("ForceRegionSliceEntity found", "ForceRegionSliceEntity not found")
    sphere_dropped    = ("Sphere dropped down",          "Sphere did not drop down") 
    sphere_bounced    = ("Sphere bounced up vertically", "Sphere did not bounce up vertically")
    exit_game_mode    = ("Exited game mode",             "Couldn't exit game mode")
# fmt: on


def ForceRegion_SliceFileInstantiates():

    """
    Summary:
    Check that a force region slice can be saved and instantiated

    Level Description:
    The SphereRigidBody entity is placed above the ForceRegionBox entity
    ForceRegionSliceEntity (entity) - Slice Asset which is imported from .slice file of another level which has
                                      an entity with force region component.
    SphereRigidBody (entity) - Entity with PhysX Rigid body, Mesh and collider components.
    The SphereRigidBody is placed above the ForceRegionEntity.

    Expected Behavior:
    Sphere drops and bounces vertically up from force region.
    We are checking if the ball started falling down from its initial position and then verifying if it has bounced up
    its initial position after entering into the force region.

    Test Steps:
     1)  Open level
     2)  Enter game mode
     3)  Retrieve and validate entities
     4)  Get the initial position of the Sphere (rigid body)
     5)  Check if the ball is falling down
     6)  Add trigger notification handler
     7)  Wait till the ball enters the force region
     8)  Check if the ball has bounced vertically up
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
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus

    # Constants
    TIMEOUT = 3.0  # wait a maximum of 3 seconds
    SPHERE_RADIUS = 0.5

    class Sphere:
        id = None
        initial_position = None
        current_position = None
        in_force_region = False
        bounced = False

    class ForceRegion:
        id = None

    def sphere_bounced():
        Sphere.current_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", Sphere.id)
        Sphere.bounced = Sphere.current_position.z > (Sphere.initial_position.z + SPHERE_RADIUS)
        return Sphere.bounced

    def on_trigger_enter(args):
        if args[0].Equal(Sphere.id):
            Sphere.in_force_region = True

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "ForceRegion_SliceFileInstantiates")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate entities
    Sphere.id = general.find_game_entity("SphereRigidBody")
    Report.critical_result(Tests.find_sphere, Sphere.id.IsValid())
    ForceRegion.id = general.find_game_entity("ForceRegionSliceEntity")
    Report.critical_result(Tests.find_force_region, ForceRegion.id.IsValid())

    # 4) Get the initial position of the Sphere (rigid body)
    Sphere.initial_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", Sphere.id)

    # 5) Check if the ball is falling down
    Sphere.current_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", Sphere.id)
    sphere_dropped = Sphere.current_position.z < (Sphere.initial_position.z + SPHERE_RADIUS)
    Report.critical_result(Tests.sphere_dropped, sphere_dropped)

    # 6) Add trigger notification handler
    handler = azlmbr.physics.TriggerNotificationBusHandler()
    handler.connect(ForceRegion.id)
    handler.add_callback("OnTriggerEnter", on_trigger_enter)

    # 7) Wait till the ball enters the force region
    helper.wait_for_condition(lambda: Sphere.in_force_region, TIMEOUT)

    # 8) Check if the ball has bounced vertically up
    # wait till the ball bounces
    helper.wait_for_condition(sphere_bounced, TIMEOUT)
    Report.critical_result(Tests.sphere_bounced, Sphere.bounced)

    # 9) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_SliceFileInstantiates)
