"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5968759
# Test Case Title : Check nested force regions exert forces simultaneously on rigid body


# fmt: off
class Tests:
    enter_game_mode                  = ("Entered game mode",                  "Failed to enter game mode")
    find_vertical_sphere             = ("Vertical sphere found",              "Vertical sphere not found")
    find_angled_sphere               = ("Angled sphere found",                "Angled sphere not found")
    find_point_force_region          = ("Point force region found",           "Force Region not found")
    find_angled_force_region         = ("Angled force region found",          "Angled force region not found")
    vertical_entered_force_region    = ("Vertical Sphere actions completed",  "Vertical Sphere actions not completed")
    timed_out                        = ("Test did not time out",              "Test TIMED OUT")
    angled_sphere_enter_force_region = ("Angled Sphere Entered Force Region", "Angled Sphere didn't enter Force Region")
    vertical_sphere_fell_vertically  = ("Vertical Sphere fell vertically",    "Vertical Sphere didn't fall vertically")
    angled_sphere_fell_at_angle      = ("Angled Sphere fell at an angle",     "Angled Sphere didn't fall at an angle")
    vertical_sphere_slowed           = ("Vertical Sphere slowed",             "Vertical Sphere not slowed")
    angled_sphere_slowed             = ("Angled Sphere slowed",               "Angled Sphere not slowed")
    exit_game_mode                   = ("Exited game mode",                   "Couldn't exit game mode")


# fmt: on

import os, sys


def ForceRegion_MultipleComponentsCombineForces():
    """
    Run() will open a a level and validate that the spheres are affected by the force regions as expected.

    Expected Results: Both spheres fall into the force regions and are slowed. One of the spheres also falls at an angle

    It does this by:
      --> Opens level and enter game mode
      --> Finds the entities in the scene
      --> Listens for spheres to enter the force regions
      --> Set Spheres start position and velocity
      --> Listen for spheres to exit force regions
      --> Set Spheres end position and velocity
      --> Validate the results
      --> Exits game mode and editor

    Level Description: Two spheres floating above 2 force regions.
    Sphere: 1 Name = "Sphere_vertical_drop"  This sphere should fall vertically
    Sphere: 2 Name =  "Sphere_angled_drop"  This sphere should fall at an angle
    First force region: Name = "Force Region Point"  Applies point force along the X axis to only the second sphere
    Second force region: Name = "Force Region Simple Drag"  Applies a drag force on both spheres
    Setup path
    """



    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.physics
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Constants
    TIME_OUT = 6.0  # Second to wait before timing out
    POSITION_TOLERANCE = 0.1

    def is_close_XY_position(vec1, vec2):
        return abs(vec1.x - vec2.x) < POSITION_TOLERANCE and abs(vec1.y - vec2.y) < POSITION_TOLERANCE

    # Holds details about the sphere
    class Sphere:
        def __init__(self, sphere_id, sphere_name):
            self.name = sphere_name
            self.id = sphere_id
            self.start_position = None
            self.end_position = None
            self.start_velocity = None
            self.end_velocity = None
            self.entered_force_region = False
            self.exited_force_region = False

        def get_position(self):
            return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

        def get_velocity(self):
            return azlmbr.physics.RigidBodyRequestBus(bus.Event, "GetLinearVelocity", self.id)


    #   1) Opens level with spheres above a force region
    helper.init_idle()
    helper.open_level("Physics", "ForceRegion_MultipleComponentsCombineForces")
    helper.enter_game_mode(Tests.enter_game_mode)

    #   2) Finds the entities in the scene
    sphere_vertical = Sphere(general.find_game_entity("Sphere_vertical_drop"), "Sphere Vertical")
    Report.critical_result(Tests.find_vertical_sphere, sphere_vertical.id.IsValid())

    sphere_angled = Sphere(general.find_game_entity("Sphere_angled_drop"), "Sphere Angled")
    Report.critical_result(Tests.find_angled_sphere, sphere_angled.id.IsValid())

    point_force_region_id = general.find_game_entity("Force Region Point")
    Report.critical_result(Tests.find_point_force_region, point_force_region_id.IsValid())

    simple_drag_force_region_id = general.find_game_entity("Force Region Simple Drag")
    Report.critical_result(Tests.find_angled_force_region, simple_drag_force_region_id.IsValid())

    # ******** Handler Functions ********

    # Called if Sphere enters force region
    def on_trigger_begin(args):
        other_id = args[0]
        #  4) Gets start position and velocity of spheres
        if other_id.Equal(sphere_vertical.id) and sphere_vertical.entered_force_region is False:
            Report.info("Trigger Entered")
            sphere_vertical.entered_force_region = True
            sphere_vertical.start_position = sphere_vertical.get_position()
            sphere_vertical.start_velocity = sphere_vertical.get_velocity()
            Report.result(Tests.vertical_entered_force_region, sphere_vertical.entered_force_region)
        elif other_id.Equal(sphere_angled.id) and sphere_angled.entered_force_region is False:
            sphere_angled.entered_force_region = True
            sphere_angled.start_position = sphere_angled.get_position()
            sphere_angled.start_velocity = sphere_angled.get_velocity()
            Report.result(Tests.angled_sphere_enter_force_region, sphere_angled.entered_force_region)

    def on_trigger_exit(args):
        #  4) Gets end position and velocity of spheres
        other_id = args[0]
        if other_id.Equal(sphere_vertical.id):
            Report.info("Trigger exited")
            sphere_vertical.end_position = sphere_vertical.get_position()
            sphere_vertical.end_velocity = sphere_vertical.get_velocity()
            sphere_vertical.exited_force_region = True

        elif other_id.Equal(sphere_angled.id):
            Report.info("Trigger exited")
            sphere_angled.end_position = sphere_angled.get_position()
            sphere_angled.end_velocity = sphere_angled.get_velocity()

            sphere_angled.exited_force_region = True

    #  3) Listens for spheres to enter the force regions
    # Create a handler for each force region
    point_force_region_handler = azlmbr.physics.TriggerNotificationBusHandler()
    point_force_region_handler.connect(point_force_region_id)
    point_force_region_handler.add_callback("OnTriggerEnter", on_trigger_begin)
    point_force_region_handler.add_callback("OnTriggerExit", on_trigger_exit)

    simple_drag_force_region_handler = azlmbr.physics.TriggerNotificationBusHandler()
    simple_drag_force_region_handler.connect(simple_drag_force_region_id)
    simple_drag_force_region_handler.add_callback("OnTriggerEnter", on_trigger_begin)
    simple_drag_force_region_handler.add_callback("OnTriggerExit", on_trigger_exit)

    Report.result(Tests.timed_out, helper.wait_for_condition(lambda: sphere_angled.exited_force_region and
                                                                     sphere_vertical.exited_force_region, TIME_OUT))

    sphere_vertical_slowed_by_force_region = sphere_vertical.end_velocity.z > sphere_vertical.start_velocity.z
    sphere_angled_slowed_by_force_region = sphere_angled.end_velocity.z > sphere_angled.start_velocity.z

    sphere_angled_fell_at_expected_angle = sphere_angled.end_position.x > sphere_angled.start_position.x + POSITION_TOLERANCE
    sphere_vertical_fell_at_expected_angle = is_close_XY_position(sphere_vertical.end_position, sphere_vertical.start_position)

    Report.result(Tests.angled_sphere_fell_at_angle, sphere_angled_fell_at_expected_angle)
    Report.result(Tests.vertical_sphere_fell_vertically, sphere_vertical_fell_at_expected_angle)
    Report.result(Tests.angled_sphere_slowed, sphere_angled_slowed_by_force_region)
    Report.result(Tests.vertical_sphere_slowed, sphere_vertical_slowed_by_force_region)

    # 8) Exit Game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_MultipleComponentsCombineForces)
