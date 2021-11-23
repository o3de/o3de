"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#fmt: off
class Tests():
    create_terrain_spawner_entity               = ("Terrain_spawner_entity created successfully", "Failed to create terrain_spawner_entity")
    create_height_provider_entity               = ("Height_provider_entity created successfully", "Failed to create height_provider_entity")
    create_test_ball                            = ("Ball created successfully",    "Failed to create Ball")
    box_dimensions_changed                      = ("Aabb dimensions changed successfully", "Failed change Aabb dimensions")
    shape_changed                               = ("Shape changed successfully", "Failed Shape change")
    entity_added                                = ("Entity added successfully", "Failed Entity add")
    frequency_changed                           = ("Frequency changed successfully", "Failed Frequency change")
    shape_set                                   = ("Shape set to Sphere successfully", "Failed to set Sphere shape")
    test_collision                              = ("Ball collided with terrain", "Ball failed to collide with terrain")
    no_errors_and_warnings_found                = ("No errors and warnings found", "Found errors and warnings")
#fmt: on

def Terrain_SupportsPhysics():
    """
    Summary:
    Test aspects of the TerrainHeightGradientList through the BehaviorContext and the Property Tree.

    Test Steps:
    Expected Behavior:
    The Editor is stable there are no warnings or errors.

    Test Steps:
     1) Load the base level
     2) Create 2 test entities, one parent at 512.0, 512.0, 50.0 and one child at the default position and add the required components
     2a) Create a ball at 600.0, 600.0, 46.0 - This position is not too high over the heightfield so will collide in a reasonable time
     3) Start the Tracer to catch any errors and warnings
     4) Change the Axis Aligned Box Shape dimensions
     5) Set the Vegetation Shape reference to TestEntity1
     6) Set the FastNoise gradient frequency to 0.01
     7) Set the Gradient List to TestEntity2
     8) Set the PhysX Collider to Sphere mode
     9) Disable and Enable the Terrain Gradient List so that it is recognised
     10) Enter game mode and test if the ball hits the heightfield within 3 seconds
     11) Verify there are no errors and warnings in the logs


    :return: None
    """

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import TestHelper as helper, Report
    from editor_python_test_tools.utils import Report, Tracer
    import editor_python_test_tools.hydra_editor_utils as hydra
    import azlmbr.math as azmath
    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import math

    SET_BOX_X_SIZE = 1024.0
    SET_BOX_Y_SIZE = 1024.0
    SET_BOX_Z_SIZE = 100.0
    
    helper.init_idle()
    
    # 1) Load the level
    helper.open_level("", "Base")
    helper.wait_for_condition(lambda: general.get_current_level_name() == "Base", 2.0)

    #1a) Load the level components
    hydra.add_level_component("Terrain World")
    hydra.add_level_component("Terrain World Renderer")

    # 2) Create 2 test entities, one parent at 512.0, 512.0, 50.0 and one child at the default position and add the required components
    entity1_components_to_add = ["Axis Aligned Box Shape", "Terrain Layer Spawner", "Terrain Height Gradient List", "Terrain Physics Heightfield Collider", "PhysX Heightfield Collider"]
    entity2_components_to_add = ["Vegetation Reference Shape", "Gradient Transform Modifier", "FastNoise Gradient"]
    ball_components_to_add = ["Sphere Shape", "PhysX Collider", "PhysX Rigid Body"]
    terrain_spawner_entity = hydra.Entity("TestEntity1")
    terrain_spawner_entity.create_entity(azmath.Vector3(512.0, 512.0, 50.0), entity1_components_to_add)
    Report.result(Tests.create_terrain_spawner_entity, terrain_spawner_entity.id.IsValid())
    height_provider_entity = hydra.Entity("TestEntity2")
    height_provider_entity.create_entity(azmath.Vector3(0.0, 0.0, 0.0), entity2_components_to_add,terrain_spawner_entity.id)
    Report.result(Tests.create_height_provider_entity, height_provider_entity.id.IsValid())
    # 2a) Create a ball at 600.0, 600.0, 46.0 - This position is not too high over the heightfield so will collide in a reasonable time
    ball = hydra.Entity("Ball")
    ball.create_entity(azmath.Vector3(600.0, 600.0, 46.0), ball_components_to_add)
    Report.result(Tests.create_test_ball, ball.id.IsValid())
    # Give everything a chance to finish initializing.
    general.idle_wait_frames(1)

    # 3) Start the Tracer to catch any errors and warnings
    with Tracer() as section_tracer:
        # 4) Change the Axis Aligned Box Shape dimensions
        box_dimensions = azmath.Vector3(SET_BOX_X_SIZE, SET_BOX_Y_SIZE, SET_BOX_Z_SIZE)
        terrain_spawner_entity.get_set_test(0, "Axis Aligned Box Shape|Box Configuration|Dimensions", box_dimensions)
        box_shape_dimensions = hydra.get_component_property_value(terrain_spawner_entity.components[0], "Axis Aligned Box Shape|Box Configuration|Dimensions")
        Report.result(Tests.box_dimensions_changed, box_dimensions == box_shape_dimensions)
        
        # 5) Set the Vegetaion Shape reference to TestEntity1
        height_provider_entity.get_set_test(0, "Configuration|Shape Entity Id", terrain_spawner_entity.id)
        entityId = hydra.get_component_property_value(height_provider_entity.components[0], "Configuration|Shape Entity Id")
        Report.result(Tests.shape_changed, entityId == terrain_spawner_entity.id)

        # 6) Set the FastNoise Gradient frequency to 0.01
        Frequency = 0.01
        height_provider_entity.get_set_test(2, "Configuration|Frequency", Frequency)
        FrequencyVal = hydra.get_component_property_value(height_provider_entity.components[2], "Configuration|Frequency")
        Report.result(Tests.frequency_changed, math.isclose(Frequency, FrequencyVal, abs_tol = 0.00001))

        # 7) Set the Gradient List to TestEntity2
        propertyTree = hydra.get_property_tree(terrain_spawner_entity.components[2])
        propertyTree.add_container_item("Configuration|Gradient Entities", 0, height_provider_entity.id)
        checkID = propertyTree.get_container_item("Configuration|Gradient Entities", 0)
        Report.result(Tests.entity_added, checkID.GetValue() == height_provider_entity.id)

        # 8) Set the PhysX Collider to Sphere mode
        shape = 0
        hydra.get_set_test(ball, 1, "Shape Configuration|Shape", shape)
        setShape = hydra.get_component_property_value(ball.components[1], "Shape Configuration|Shape")
        Report.result(Tests.shape_set, shape == setShape)

        # 9) Disable and Enable the Terrain Gradient List so that it is recognised
        editor.EditorComponentAPIBus(bus.Broadcast, 'EnableComponents', [terrain_spawner_entity.components[2]])

        general.enter_game_mode()

        general.idle_wait_frames(1)

        # 10) Enter game mode and test if the ball hits the heightfield within 3 seconds
        TIMEOUT = 3.0

        class Collider:
            id = general.find_game_entity("Ball")
            touched_ground = False

        terrain_id = general.find_game_entity("TestEntity1")
 
        def on_collision_begin(args):
            other_id = args[0]
            if other_id.Equal(terrain_id):
                Report.info("Touched ground")
                Collider.touched_ground = True

        handler = azlmbr.physics.CollisionNotificationBusHandler()
        handler.connect(Collider.id)
        handler.add_callback("OnCollisionBegin", on_collision_begin)

        helper.wait_for_condition(lambda: Collider.touched_ground, TIMEOUT)
        Report.result(Tests.test_collision, Collider.touched_ground)

        general.exit_game_mode()

    # 11) Verify there are no errors and warnings in the logs
    helper.wait_for_condition(lambda: section_tracer.has_errors or section_tracer.has_asserts, 1.0)
    for error_info in section_tracer.errors:
        Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
    for assert_info in section_tracer.asserts:
        Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")
    

if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(Terrain_SupportsPhysics)

