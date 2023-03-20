# coding=utf-8
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5932043
# Test Case Title : Check that force region exerts simple drag force on rigid bodies


# fmt: off
class Tests:
    enter_game_mode        = ("Entered game mode",          "Failed to enter game mode")
    find_sphere            = ("Sphere found",        "Sphere not found")
    find_force_region      = ("Force Region found",         "Force Region not found")

    # entity_actions_success refers to if the entity completed all expected actions in the test. For this test, this
    # will be, did the Sphere enter and exit the force region
    entity_actions_success = ("Entity actions completed",   "Entity actions not completed")
    sphere_lost_height     = ("Sphere went down",           "Sphere didn't go down")
    force_region_slows   = ("Force Region slowed Sphere", "Force Region didn't slow Sphere")
    exit_game_mode         = ("Exited game mode",           "Couldn't exit game mode")
# fmt: on


def ForceRegion_SimpleDragForceOnRigidBodies():
    # This run() function will open a a level and validate that the force region slows down a spheres fall.
    # It does this by:
    #   1) Opens level with sphere above a force region
    #   2) Enters Game mode
    #   3) Finds the entities in the scene
    #   4) Listens for sphere to enter the force region
    #   5) Gets z velocity and position of sphere
    #   6) Listens for sphere to exit force region
    #   7) Gets new velocity and position of sphere
    #   8) Validate the results
    #   9) Exits game mode and editor

    # Setup path
    import os, sys



    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import azlmbr.legacy.general as general
    import azlmbr.bus

    # Holds details about the sphere
    class Sphere:
        id = None
        start_velocity_z = 0.0
        end_velocity_z = 0.0
        sphere_start_z_position = 0.0
        sphere_end_z_position = 0.0
        entered_force_region = False
        exited_force_region = False

    TIME_OUT = 4.0  # Time given to test to complete.

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "ForceRegion_SimpleDragForceOnRigidBodies")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Get Entities
    Sphere.id = general.find_game_entity("Sphere")
    Report.result(Tests.find_sphere, Sphere.id.IsValid())

    force_region_id = general.find_game_entity("Force Region")
    Report.result(Tests.find_force_region, force_region_id.IsValid())

    # Called if Sphere enters force region
    def on_trigger_begin(args):
        other_id = args[0]
        if other_id.Equal(Sphere.id):
            Report.info("Entered force region")
            Sphere.entered_force_region = True
            #  5) Gets z velocity and position of sphere
            Sphere.start_velocity_z = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", Sphere.id).z
            Sphere.sphere_start_z_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", Sphere.id)
            Report.info(
                "Sphere Start Z position = {}  Z Start Velocity = {}".format(
                    Sphere.sphere_start_z_position, Sphere.start_velocity_z
                )
            )

    # Called when sphere exits force region
    def on_trigger_end(args):
        other_id = args[0]
        if other_id.Equal(Sphere.id):
            Report.info("Exited force region")
            Sphere.exited_force_region = True
            #  7) Gets new velocity and position of sphere
            Sphere.end_velocity_z = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", Sphere.id).z
            Sphere.sphere_end_z_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", Sphere.id)
            Report.info(
                "Sphere End Z position = {}  Sphere End Z Velocity = {}".format(
                    Sphere.sphere_end_z_position, Sphere.end_velocity_z
                )
            )


    handler = azlmbr.physics.TriggerNotificationBusHandler()
    handler.connect(force_region_id)
    # 4) Listens for sphere to enter the force region
    handler.add_callback("OnTriggerEnter", on_trigger_begin)
    # 6) Listens for sphere to exit force region
    handler.add_callback("OnTriggerExit", on_trigger_end)

    # Wait until all entities in scene are done performing their actions or test times out.
    def done_with_entity_actions():
        return Sphere.entered_force_region and Sphere.exited_force_region

    test_completed = helper.wait_for_condition(done_with_entity_actions, TIME_OUT)
    Report.result(Tests.entity_actions_success, test_completed)

    #   8) Validate the results
    if test_completed:
        # Did Sphere fall
        sphere_descended = Sphere.sphere_end_z_position + 0.5 < Sphere.sphere_start_z_position  # 0.5 for buffer
        Report.result(Tests.sphere_lost_height, sphere_descended)

        # Did Force Region slow down sphere's falling
        # Note: The faster a sphere falls, the greater its negative/downward velocity will be. Adding 1.0 for buffer.
        force_region_result = round(Sphere.end_velocity_z, 2) > round(Sphere.start_velocity_z, 2) + 1.0
        Report.result(Tests.force_region_slows, force_region_result)

    #  9) Exits game mode and editor
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_SimpleDragForceOnRigidBodies)
