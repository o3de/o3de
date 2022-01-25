"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#fmt: off
class Tests():
    level_components_added                      = ("Level components added correctly", "Failed to create level components")
    create_terrain_spawner_entity               = ("Terrain_spawner_entity created successfully", "Failed to create terrain_spawner_entity")
    create_height_provider_entity               = ("Height_provider_entity created successfully", "Failed to create height_provider_entity")
    bounds_max_changed                          = ("Terrain World Bounds Max changed successfully", "Failed to change Terrain World Bounds Max")
    bounds_min_changed                          = ("Terrain World Bounds Min changed successfully", "Failed to change Terrain World Bounds Min")
    height_query_changed                        = ("Terrain World Height Query Resolution changed successfully", "Failed to change Height Query Resolution")
    box_dimensions_changed                      = ("Aabb dimensions changed successfully", "Failed to change Aabb dimensions")
    shape_changed                               = ("Shape changed successfully", "Failed Shape change")
    frequency_changed                           = ("Frequency changed successfully", "Failed Frequency change")
    entity_added                                = ("Entity added successfully", "Failed Entity add")
    terrain_exists                              = ("Terrain exists at the provided point", "Terrain does not exist at the provided point")
    terrain_does_not_exist                      = ("Terrain does not exist at the provided point", "Terrain exists at the provided point")
    values_not_the_same                         = ("The tested values are not the same", "The tested values are the same")
    no_errors_and_warnings_found                = ("No errors and warnings found", "Found errors and warnings")
#fmt: on

def Terrain_World_ConfigurationWorks():
    """
    Summary:
    Test the Terrain World configuration changes when parameters are changed in the component

    Test Steps:
    Expected Behavior:
    The Editor is stable there are no warnings or errors.

    Test Steps:
     1) Start the Tracer to catch any errors and warnings
     2) Load the base level
     3) Load the level components
     4) Create 2 test entities, one parent at 512.0, 512.0, 50.0 and one child at the default position and add the required components
     5) Set the base Terrain World values
     6) Change the Axis Aligned Box Shape dimensions
     7) Set the Shape Reference to terrain_spawner_entity
     8) Set the FastNoise Gradient frequency to 0.01
     9) Set the Gradient List to height_provider_entity
     10) Disable and Enable the Terrain Gradient List so that it is recognised
     11) Check terrain exists at a known position in the world
     12) Check terrain does not exist at a known position outside the world
     13) Check height value is the expected one when query resolution is changed
    """
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import TestHelper as helper, Report
    from editor_python_test_tools.utils import Report, Tracer
    import editor_python_test_tools.hydra_editor_utils as hydra
    import azlmbr.math as azmath
    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.terrain as terrain
    import math

    SET_BOX_X_SIZE = 2048.0
    SET_BOX_Y_SIZE = 2048.0
    SET_BOX_Z_SIZE = 100.0
    CLAMP = 1
    
    helper.init_idle()
    
    # 1) Start the Tracer to catch any errors and warnings
    with Tracer() as section_tracer:
        # 2) Load the level
        helper.open_level("", "Base")
        helper.wait_for_condition(lambda: general.get_current_level_name() == "Base", 2.0)
        
        # 3) Load the level components
        terrain_world_component = hydra.add_level_component("Terrain World")
        terrain_world_renderer = hydra.add_level_component("Terrain World Renderer")
        Report.critical_result(Tests.level_components_added, 
            terrain_world_component is not None and  terrain_world_renderer is not None)
 
        # 4) Create 2 test entities, one parent at 512.0, 512.0, 50.0 and one child at the default position and add the required components
        entity1_components_to_add = ["Axis Aligned Box Shape", "Terrain Layer Spawner", "Terrain Height Gradient List", "Terrain Physics Heightfield Collider"]
        entity2_components_to_add = ["Shape Reference", "Gradient Transform Modifier", "FastNoise Gradient"]
        terrain_spawner_entity = hydra.Entity("TerrainEntity")
        terrain_spawner_entity.create_entity(azmath.Vector3(512.0, 512.0, 50.0), entity1_components_to_add)
        Report.result(Tests.create_terrain_spawner_entity, terrain_spawner_entity.id.IsValid())
        height_provider_entity = hydra.Entity("HeightProviderEntity")
        height_provider_entity.create_entity(azmath.Vector3(0.0, 0.0, 0.0), entity2_components_to_add,terrain_spawner_entity.id)
        Report.result(Tests.create_height_provider_entity, height_provider_entity.id.IsValid())

        # Give everything a chance to finish initializing.
        general.idle_wait_frames(1)

        # 5) Set the base Terrain World values
        world_bounds_max = azmath.Vector3(1100.0, 1100.0, 1100.0)
        world_bounds_min = azmath.Vector3(10.0, 10.0, 10.0)
        height_query_resolution = azmath.Vector2(1.0, 1.0)
        hydra.set_component_property_value(terrain_world_component, "Configuration|World Bounds (Max)", world_bounds_max)
        hydra.set_component_property_value(terrain_world_component, "Configuration|World Bounds (Min)", world_bounds_min)
        hydra.set_component_property_value(terrain_world_component, "Configuration|Height Query Resolution (m)", height_query_resolution)
        world_max = hydra.get_component_property_value(terrain_world_component, "Configuration|World Bounds (Max)")
        world_min = hydra.get_component_property_value(terrain_world_component, "Configuration|World Bounds (Min)")
        world_query = hydra.get_component_property_value(terrain_world_component, "Configuration|Height Query Resolution (m)")
        Report.result(Tests.bounds_max_changed, world_max == world_bounds_max)
        Report.result(Tests.bounds_min_changed, world_min == world_bounds_min)
        Report.result(Tests.height_query_changed, world_query == height_query_resolution)

        # 6) Change the Axis Aligned Box Shape dimensions
        box_dimensions = azmath.Vector3(SET_BOX_X_SIZE, SET_BOX_Y_SIZE, SET_BOX_Z_SIZE)
        terrain_spawner_entity.get_set_test(0, "Axis Aligned Box Shape|Box Configuration|Dimensions", box_dimensions)
        box_shape_dimensions = hydra.get_component_property_value(terrain_spawner_entity.components[0], "Axis Aligned Box Shape|Box Configuration|Dimensions")
        Report.result(Tests.box_dimensions_changed, box_dimensions == box_shape_dimensions)
        
        # 7) Set the Shape Reference to terrain_spawner_entity
        height_provider_entity.get_set_test(0, "Configuration|Shape Entity Id", terrain_spawner_entity.id)
        entityId = hydra.get_component_property_value(height_provider_entity.components[0], "Configuration|Shape Entity Id")
        Report.result(Tests.shape_changed, entityId == terrain_spawner_entity.id)

        # 8) Set the FastNoise Gradient frequency to 0.01
        frequency = 0.01
        height_provider_entity.get_set_test(2, "Configuration|Frequency", frequency)
        frequencyVal = hydra.get_component_property_value(height_provider_entity.components[2], "Configuration|Frequency")
        Report.result(Tests.frequency_changed, math.isclose(frequency, frequencyVal, abs_tol = 0.00001))

        # 9) Set the Gradient List to height_provider_entity
        propertyTree = hydra.get_property_tree(terrain_spawner_entity.components[2])
        propertyTree.add_container_item("Configuration|Gradient Entities", 0, height_provider_entity.id)
        checkID = propertyTree.get_container_item("Configuration|Gradient Entities", 0)
        Report.result(Tests.entity_added, checkID.GetValue() == height_provider_entity.id)

        general.idle_wait_frames(1)

        # 10) Disable and Enable the Terrain Gradient List so that it is recognised, EnableComponents performs both actions.
        editor.EditorComponentAPIBus(bus.Broadcast, 'EnableComponents', [terrain_spawner_entity.components[2]])

        # 11) Check terrain exists at a known position in the world
        terrainExists = not terrain.TerrainDataRequestBus(bus.Broadcast, 'GetIsHoleFromFloats', 10.0, 10.0, CLAMP)
        Report.result(Tests.terrain_exists, terrainExists)

        terrainExists = not terrain.TerrainDataRequestBus(bus.Broadcast, 'GetIsHoleFromFloats', 1100.0, 1100.0, CLAMP)
        Report.result(Tests.terrain_exists, terrainExists)

        # 12) Check terrain does not exist at a known position outside the world
        terrainDoesNotExist = terrain.TerrainDataRequestBus(bus.Broadcast, 'GetIsHoleFromFloats', 1101.0, 1101.0, CLAMP)
        Report.result(Tests.terrain_does_not_exist, terrainDoesNotExist)

        terrainDoesNotExist = terrain.TerrainDataRequestBus(bus.Broadcast, 'GetIsHoleFromFloats', 9.0, 9.0, CLAMP)
        Report.result(Tests.terrain_does_not_exist, terrainDoesNotExist)

        # 13) Check height value is the expected one when query resolution is changed
        testpoint = terrain.TerrainDataRequestBus(bus.Broadcast, 'GetHeightFromFloats', 10.5, 10.5, CLAMP)
        height_query_resolution = azmath.Vector2(0.5, 0.5)
        hydra.set_component_property_value(terrain_world_component, "Configuration|Height Query Resolution (m)", height_query_resolution)
        general.idle_wait_frames(1)
        testpoint2 =  terrain.TerrainDataRequestBus(bus.Broadcast, 'GetHeightFromFloats', 10.5, 10.5, CLAMP)
        Report.result(Tests.values_not_the_same, not math.isclose(testpoint, testpoint2, abs_tol = 0.000000001))
        
    helper.wait_for_condition(lambda: section_tracer.has_errors or section_tracer.has_asserts, 1.0)
    for error_info in section_tracer.errors:
        Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
    for assert_info in section_tracer.asserts:
        Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(Terrain_World_ConfigurationWorks)

