"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C6321601
# Test Case Title : Check that very high values of direction axes of forces do not throw error



# fmt: off
class Tests():
    enter_game_mode              = ("Entered game mode",                                    "Failed to enter game mode")
    find_terrain                 = ("Terrain found",                                        "Terrain not found")
    find_sphere_world_space      = ("Sphere above world force region found",                "Sphere above world force region not found")
    find_sphere_local_space      = ("Sphere above local force region found",                "Sphere above local force region not found")
    find_sphere_point            = ("Sphere above point force region found",                "Sphere above point force region not found")
    find_sphere_simple_drag      = ("Sphere above simple drag force region found",          "Sphere above simple drag force region not found")
    find_sphere_linear_damping   = ("Sphere above linear damping force region found",       "Sphere above linear damping force region not found")
    find_forcevol_world_space    = ("World force region found",                             "World force region not found")
    find_forcevol_local_space    = ("Local force region found",                             "Local force region not found")
    find_forcevol_point          = ("Point force region found",                             "Point force region not found")
    find_forcevol_simple_drag    = ("Simple drag force region found",                       "Simple drag force region not found")
    find_forcevol_linear_damping = ("Linear damping force region found",                    "Linear damping force region not found")
    world_force_magnitude        = ("World force magnitude equal to expected magnitude",    "World force magnitude not equal to expected magnitude")
    world_force_direction        = ("World force direction equal to expected direction",    "World force direction not equal to expected direction")
    local_force_magnitude        = ("Local force magnitude equal to expected magnitude",    "Local force magnitude not equal to expected magnitude")
    local_force_direction        = ("Local force direction equal to expected direction",    "Local force direction not equal to expected direction")
    point_force_magnitude        = ("Point force magnitude equal to expected magnitude",    "Point force magnitude not equal to expected magnitude")
    simp_drag_density            = ("Simple Drag force density equal to expected value",    "Simple Drag force density not equal to expected value")
    lin_damp_damping             = ("Linear Damping force damping equal to expected value", "Linear Damping force damping not equal to expected value")
    error_not_found              = ("Error not found",                                      "Error found")
    exit_game_mode               = ("Exited game mode",                                     "Couldn't exit game mode")
# fmt: on


def ForceRegion_HighValuesDirectionAxesWorkWithNoError():

    """
    Summary:
    Check that very high values of direction axes of forces do not throw error.

    Level Description:
    Sphere_World_Space, Sphere_Local_Space, Sphere_Point, Sphere_Simple_Drag, Sphere_Linear_Damping
    (Entities) Entities with components:
      - Physx Collider (Sphere shaped with radius 1.0)
      - Mesh(Prmitive sphere mesh)
      - PhysX Rigid Body Physics

    Below are the entities with common components
      - PhysX Collider (Trigger enabled)
      - PhysX Force Region(Visible and Debug Forces enabled)
    They differ in Force Region Force Type with the following properties:
      1) ForceVol_World_Space
            Type - World Space - Direction(0.0, 0.0, 999999.0) - Magnitude(999999.0)
      2) ForceVol_Local_Space
            Type - Local Space - Direction(0.0, 0.0, 999999.0) - Magnitude(999999.0)
      3) ForceVol_Point
            Type - Point - Magnitude(999999.0)
      4) ForceVol_Simple_Drag
            Type - Simple Drag - Region Density(999.0)
      5) ForceVol_Linear_Damping
            Type - Linear Damping - Damping(99.0)
    Each sphere is placed above its corresponding force regions.
    Each of the force regions are seperated by some distance

    Expected Behavior:
    The given force should be applied as it is without any error on the Sphere.
    We are verifying if the force being applied on each sphere is equal to the expected value

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Retrieve and validate entities
     4) Add force region handler and validate the forces
     5) Exit game mode
     6) Close the editor


    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Aed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer

    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr.math as lymath

    # Constants
    TOLERANCE_PERCENT = 0.001
    EXPECTED_DIRECTION = lymath.Vector3(0.0, 0.0, 1.0)
    EXPECTED_DAMPING = 99.0
    EXPECTED_DENSITY = 400.0
    CLOSE_THRESHOLD = 0.0001

    class SphereForceRegion:
        def __init__(self, force_name):
            self.force_name = force_name
            self.sphere_name = "Sphere_{}".format(force_name)
            self.force_region_name = "ForceVol_{}".format(force_name)
            self.sphere_id = general.find_game_entity(self.sphere_name)
            self.force_region_id = general.find_game_entity(self.force_region_name)
            self.in_force_region = False
            self.validate_entities()

        def validate_entities(self):
            Report.result(Tests.__dict__["find_{}".format(self.sphere_name.lower())], self.sphere_id.IsValid())
            Report.result(
                Tests.__dict__["find_{}".format(self.force_region_name.lower())], self.force_region_id.IsValid()
            )

    def validate_world_space_force(args):
        Report.info("Validating world space force...")
        world_expected_magnitude = azlmbr.physics.ForceWorldSpaceRequestBus(
            azlmbr.bus.Event, "GetMagnitude", regions[0].force_region_id
        )
        world_actual_magnitude = args[3]
        Report.info(
            "Actual Magnitude: {}\t Expected Magnitude: {}".format(world_actual_magnitude, world_expected_magnitude)
        )
        Report.result(
            Tests.world_force_magnitude,
            abs(world_actual_magnitude - world_expected_magnitude) < TOLERANCE_PERCENT * world_expected_magnitude,
        )
        world_actual_direction = args[2]
        Report.result(Tests.world_force_direction, world_actual_direction.IsClose(EXPECTED_DIRECTION, CLOSE_THRESHOLD))

    def validate_local_space_force(args):
        Report.info("Validating local space force...")
        local_expected_magnitude = azlmbr.physics.ForceLocalSpaceRequestBus(
            azlmbr.bus.Event, "GetMagnitude", regions[1].force_region_id
        )
        local_actual_magnitude = args[3]
        Report.info(
            "Actual Magnitude: {}\t Expected Magnitude: {}".format(local_actual_magnitude, local_expected_magnitude)
        )
        Report.result(
            Tests.local_force_magnitude,
            abs(local_actual_magnitude - local_expected_magnitude) < TOLERANCE_PERCENT * local_expected_magnitude,
        )
        local_actual_direction = args[2]
        Report.result(Tests.local_force_direction, local_actual_direction.IsClose(EXPECTED_DIRECTION, CLOSE_THRESHOLD))

    def validate_point_force(args):
        Report.info("Validating Point space force...")
        point_expected_magnitude = azlmbr.physics.ForcePointRequestBus(
            azlmbr.bus.Event, "GetMagnitude", regions[2].force_region_id
        )
        point_actual_magnitude = args[3]
        Report.info(
            "Actual Magnitude: {}\t Expected Magnitude: {}".format(point_actual_magnitude, point_expected_magnitude)
        )
        Report.result(
            Tests.point_force_magnitude,
            abs(point_actual_magnitude - point_expected_magnitude) < TOLERANCE_PERCENT * point_expected_magnitude,
        )

    def validate_simple_drag_force(args):
        Report.info("Validating Simple Drag force...")
        simp_drag_density = azlmbr.physics.ForceSimpleDragRequestBus(
            azlmbr.bus.Event, "GetDensity", regions[3].force_region_id
        )
        Report.info("Density: {}\t Expected Density: {}".format(simp_drag_density, EXPECTED_DENSITY))
        Report.result(Tests.simp_drag_density, simp_drag_density == EXPECTED_DENSITY)

    def validate_linear_damping_force(args):
        Report.info("Validating Linear Damping force...")
        lin_damp_damping = azlmbr.physics.ForceLinearDampingRequestBus(
            azlmbr.bus.Event, "GetDamping", regions[4].force_region_id
        )
        Report.info("Damping: {}\t Expected Damping: {}".format(lin_damp_damping, EXPECTED_DAMPING))
        Report.result(Tests.lin_damp_damping, lin_damp_damping == EXPECTED_DAMPING)

    def on_calc_net_force(args):
        """
        Args:
            args[0] - force region entity
            args[1] - entity entering
            args[2] - vector
            args[3] - magnitude
        """
        for index, region in enumerate(regions):
            if args[0].Equal(region.force_region_id) and args[1].Equal(region.sphere_id) and not region.in_force_region:
                region.in_force_region = True
                force_validations[index][1](args)

    helper.init_idle()

    with Tracer() as entity_error_tracer:

        def has_physx_error():
            return entity_error_tracer.has_errors

        # 1) Open level
        helper.open_level("Physics", "ForceRegion_HighValuesDirectionAxesWorkWithNoError")

        # 2) Enter game mode
        helper.enter_game_mode(Tests.enter_game_mode)

        # 3) Retrieve and validate entities
        # Terrain
        terrain_id = general.find_game_entity("Terrain")
        Report.result(Tests.find_terrain, terrain_id.IsValid())
        force_validations = (
            ("World_Space", validate_world_space_force),
            ("Local_Space", validate_local_space_force),
            ("Point", validate_point_force),
            ("Simple_Drag", validate_simple_drag_force),
            ("Linear_Damping", validate_linear_damping_force),
        )
        regions = []
        for item in force_validations:
            regions.append(SphereForceRegion(item[0]))

        # 4) Add force region handler and validate the forces
        force_notification_handler = azlmbr.physics.ForceRegionNotificationBusHandler()
        force_notification_handler.connect(None)
        force_notification_handler.add_callback("OnCalculateNetForce", on_calc_net_force)

        # Wait for 3 secs, because there is a known bug identified and filed in
        # JIRA LY-107677
        # The error "[Error] Huge object being added to a COctreeNode, name: 'MeshComponentRenderNode', objBox:"
        # will show (if occured) in about 3 sec into the game mode.
        helper.wait_for_condition(has_physx_error, 3.0)

        # 5) Exit game mode
        helper.exit_game_mode(Tests.exit_game_mode)

    Report.result(Tests.error_not_found, not has_physx_error())


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_HighValuesDirectionAxesWorkWithNoError)
