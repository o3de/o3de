"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.
SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class VegetationTests:
    vegetation_on_gradient_1 = (
        "Vegetation detected at correct position on Gradient1", 
        "Vegetation not detected at correct position on Gradient1"
    )
    vegetation_on_gradient_2 = (
        "Vegetation detected at correct position on Gradient2", 
        "Vegetation not detected at correct position on Gradient2"
    )
    unfiltered_vegetation_count_correct = (
        "Unfiltered vegetation spawn count correct", 
        "Unfiltered vegetation spawn count incorrect"
    )

    testTag2_excluded_vegetation_count_correct = (
        "TestTag2 filtered vegetation count correct", 
        "TestTag2 filtered vegetation count incorrect"
    )
    testTag2_excluded_vegetation_z_correct = (
        "TestTag2 filtered vegetation spawned in correct position", 
        "TestTag2 filtered vegetation failed to spawn in correct position"
    )

    testTag3_excluded_vegetation_count_correct = (
        "TestTag3 filtered vegetation count correct", 
        "TestTag3 filtered vegetation count incorrect"
    )
    testTag3_excluded_vegetation_z_correct = (
        "TestTag3 filtered vegetation spawned in correct position", 
        "TestTag3 filtered vegetation failed to spawn in correct position"
    )

    cleared_exclusion_vegetation_count_correct = (
        "Cleared filter vegetation count correct", 
        "Cleared filter vegetation count incorrect"
    )

def TerrainSystem_VegetationSpawnsOnTerrainSurfaces():
    """
    Summary:
    Load an empty level, 
    Create two entities with constant gradient components with different values.
    Create two non-overlapping entities with TerrainLayerSpawners in adjacent 20 m x 20 m boxes. 
      Each spawner has a different constant height.
    Create an entity to spawn vegetation in a 20 m x 20 m boxes where 10 m overlaps the first spawner, and 10 m overlaps the second.
    Ensure that vegetation spawns at the correct heights.
    Add a VegetationSurfaceMaskFilter and ensure it responds correctly to surface changes.
    :return: None
    """

    import os
    import math as sys_math

    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.math as math

    import azlmbr.areasystem as areasystem
    import azlmbr.vegetation as vegetation
    import azlmbr.terrain as terrain
    import azlmbr.surface_data as surface_data

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def create_entity_at(entity_name, components_to_add, x, y, z):
        entity = hydra.Entity(entity_name)
        entity.create_entity(math.Vector3(x, y, z), components_to_add)

        return entity

    def FindHighestAndLowestZValuesInArea(aabb):
        vegetation_items = areasystem.AreaSystemRequestBus(bus.Broadcast, 'GetInstancesInAabb', aabb)

        lowest_z = min([item.position.z for item in vegetation_items])
        highest_z = max([item.position.z for item in vegetation_items])

        return highest_z, lowest_z

    # Open an empty level.
    hydra.open_base_level()
    
    general.idle_wait_frames(1)

    box_height = 20.0
    box_y_position = 10.0
    box_dimensions = math.Vector3(20.0, 20.0, box_height)

    # Add Terrain Rendering
    hydra.add_level_component("Terrain World")
    hydra.add_level_component("Terrain World Renderer")

    # Create two terrain entities at adjoining positions
    terrain_entity_1 = create_entity_at("Terrain1", ["Terrain Layer Spawner", "Axis Aligned Box Shape", "Terrain Height Gradient List", "Terrain Surface Gradient List"], 0.0, box_y_position, box_height/2.0)
    terrain_entity_1.get_set_test(1, "Axis Aligned Box Shape|Box Configuration|Dimensions", box_dimensions)

    terrain_entity_2 = create_entity_at("Terrain2", ["Terrain Layer Spawner", "Axis Aligned Box Shape", "Terrain Height Gradient List", "Terrain Surface Gradient List"], 20.0, box_y_position, box_height/2.0)
    terrain_entity_2.get_set_test(1, "Axis Aligned Box Shape|Box Configuration|Dimensions", box_dimensions)

    # Create two gradient entities.
    gradient_value_1 = 0.25
    gradient_value_2 = 0.5

    gradient_entity_1 = create_entity_at("Gradient1", ["Constant Gradient"], 0.0, 0.0, 0.0)
    gradient_entity_1.get_set_test(0, "Configuration|Value", gradient_value_1)

    gradient_entity_2 = create_entity_at("Gradient2", ["Constant Gradient"], 0.0, 0.0, 0.0)
    gradient_entity_2.get_set_test(0, "Configuration|Value", gradient_value_2)

    mapping = terrain.TerrainSurfaceGradientMapping()
    mapping.gradientEntityId = gradient_entity_1.id
    pte = hydra.get_property_tree(terrain_entity_1.components[3])
    pte.add_container_item("Configuration|Gradient to Surface Mappings", 0, mapping)

    mapping = terrain.TerrainSurfaceGradientMapping()
    mapping.gradientEntityId = gradient_entity_2.id
    pte = hydra.get_property_tree(terrain_entity_2.components[3])
    pte.add_container_item("Configuration|Gradient to Surface Mappings", 0, mapping)

    # create a vegetation entity that overlaps both terrain entities.
    vegetation_entity = create_entity_at("Vegetation", ["Vegetation Layer Spawner", "Axis Aligned Box Shape", "Vegetation Asset List", "Vegetation Surface Mask Filter"], 10.0, box_y_position, box_height/2.0)
    vegetation_entity.get_set_test(1, "Axis Aligned Box Shape|Box Configuration|Dimensions", box_dimensions)

    # Set the vegetation area to a PrefabInstanceSpawner with a specific prefab asset selected.
    prefab_spawner = vegetation.PrefabInstanceSpawner()
    prefab_spawner.SetPrefabAssetPath(os.path.join("Prefabs", "PinkFlower.spawnable"))
    descriptor = hydra.get_component_property_value(vegetation_entity.components[2], 'Configuration|Embedded Assets|[0]')
    descriptor.spawner = prefab_spawner
    vegetation_entity.get_set_test(2, "Configuration|Embedded Assets|[0]", descriptor)

    # Assign gradients to layer spawners.
    terrain_entity_1.get_set_test(2, "Configuration|Gradient Entities", [gradient_entity_1.id])
    terrain_entity_2.get_set_test(2, "Configuration|Gradient Entities", [gradient_entity_2.id])

    # Move view so that the entities are visible.
    general.set_current_view_position(17.0, -66.0, 41.0)
    general.set_current_view_rotation(-15.0, 0.0, 0.0)

    # Expected item counts under conditions to be tested.
    # By default, vegetation spawns at a density of 20 items per 16 meters, so in a 20m square, there should 
    # be (20 m * (20/16)) ^ 2 , or 25 ^ 2 instances. However, there's a final spawn point that lands directly on the max edge 
    # of the 20 m box so vegetation actually spawns 26 ^ 2 = 676 instances. This is how many instances we expect with no filtering.
    expected_no_filtering_item_count = 676
    
    # When filtering to the 'terrain' tag, any points that fall on non-existent terrain (a hole) get filtered out.
    # The terrain excludes the max edges of the AABB for a spawner, so points that fall on or interpolate to the max edge will get
    # filtered out. The setup for this level is two adjacent terrain spawners that cover an area of (-10, 0) to (30, 20),
    # and the vegetation area covers (0, 0) to (20, 20). Any points that fall on max edge (20) in the Y direction get filtered out
    # because that's the max edge of the terrain, but points on the max edge (20) in the X direction do NOT get filtered out because
    # there is still more terrain past that point. Therefore, instead of 26 * 26 instances, we have 26 * 25 = 650 instances.
    expected_terrain_included_item_count = 650

    # When filtering to the 'test_tag2' tag, we're only keeping points in the vegetation area of (0, 0) to (20, 20) that come from the
    # first terrain spawner, which is (-10, 0) to (10, 20). The overlap box is (0, 0) to (10, 20), which contains 
    # (10 m * (20/16)) = 12.5 points in X, but since we can't have half points, it only contains 12 points
    # In Y, we have (20 m * (20/16)) = 25 points. Both X and Y would have an additional point on the max boundary, but since the terrain
    # spawner ends on both max boundaries, those are excluded and aren't a part of the spawner's surface points.
    # So the expected count is 12 * 25 = 300 instances.
    expected_tag2_included_item_count = 300

    # When filtering to the 'test_tag3' tag, we're only keeping points in the vegetation area of (0, 0) to (20, 20) that come from the
    # second terrain spawner, which is (10, 0) to (30, 20). The overlap box is (10, 0) to (20, 20), which again contains 
    # 12.5 points in X and 25 points in Y. Due to where the boundaries start, the half point *is* contained in this box, so 
    # X has 13 points. Also, the max X boundary isn't the max spawner boundary, so it also has a 14th point. Y is still 25 points.
    # The expected count is 14 * 25 = 350 instances.
    expected_tag3_included_item_count = 350

    # Wait for the vegetation to spawn
    helper.wait_for_condition(lambda: vegetation.VegetationSpawnerRequestBus(bus.Event, "GetAreaProductCount", vegetation_entity.id) == expected_no_filtering_item_count, 5.0)

    # Check the spawn count is correct.
    item_count = vegetation.VegetationSpawnerRequestBus(bus.Event, "GetAreaProductCount", vegetation_entity.id)
    Report.result(VegetationTests.unfiltered_vegetation_count_correct, item_count == expected_no_filtering_item_count)

    test_aabb = math.Aabb_CreateFromMinMax(math.Vector3(-10.0, -10.0, 0.0), math.Vector3(30.0, 10.0, box_height))

    # Find the z positions of the items with the lowest and highest x values, this will avoid the overlap area where z values are blended between the surface heights.
    highest_z, lowest_z = FindHighestAndLowestZValuesInArea(test_aabb)
    
    # Check that the z values are as expected.
    Report.result(VegetationTests.vegetation_on_gradient_1, sys_math.isclose(lowest_z, box_height * gradient_value_1, abs_tol=0.01))
    Report.result(VegetationTests.vegetation_on_gradient_2, sys_math.isclose(highest_z, box_height * gradient_value_2, abs_tol=0.01))

    # Assign SurfaceTags to the SurfaceGradientLists
    terrain_entity_1.get_set_test(3, "Configuration|Gradient to Surface Mappings|[0]|Surface Tag", surface_data.SurfaceTag("test_tag2"))
    terrain_entity_2.get_set_test(3, "Configuration|Gradient to Surface Mappings|[0]|Surface Tag", surface_data.SurfaceTag("test_tag3"))

    # Give the VegetationSurfaceFilter an inclusion list, set it to include test_tag3 which should
    # include only the instances on the upper terrain_entity_2.
    vegetation_entity.get_set_test(3, "Configuration|Inclusion|Surface Tags", [surface_data.SurfaceTag()])
    vegetation_entity.get_set_test(3, "Configuration|Inclusion|Surface Tags|[0]", surface_data.SurfaceTag("test_tag3"))
  
    # Wait for the vegetation to respawn and check z values.
    helper.wait_for_condition(lambda: vegetation.VegetationSpawnerRequestBus(bus.Event, "GetAreaProductCount", vegetation_entity.id) == expected_tag3_included_item_count, 5.0)

    item_count = vegetation.VegetationSpawnerRequestBus(bus.Event, "GetAreaProductCount", vegetation_entity.id)
    Report.result(VegetationTests.testTag2_excluded_vegetation_count_correct, item_count == expected_tag3_included_item_count)
    Report.info(f"Found item count for testTag3 inclusion: {item_count}")

    highest_z, lowest_z = FindHighestAndLowestZValuesInArea(test_aabb)

    Report.result(VegetationTests.testTag2_excluded_vegetation_z_correct, lowest_z > box_height * gradient_value_1)

    # Set the filter to 'terrain' and ensure that vegetation spawns across both layers
    vegetation_entity.get_set_test(3, "Configuration|Inclusion|Surface Tags|[0]", surface_data.SurfaceTag("terrain"))
    helper.wait_for_condition(lambda: vegetation.VegetationSpawnerRequestBus(bus.Event, "GetAreaProductCount", vegetation_entity.id) == expected_terrain_included_item_count, 5.0)

    item_count = vegetation.VegetationSpawnerRequestBus(bus.Event, "GetAreaProductCount", vegetation_entity.id)
    Report.result(VegetationTests.cleared_exclusion_vegetation_count_correct, item_count == expected_terrain_included_item_count)
    Report.info(f"Found item count for cleared exclusion: {item_count}")

    # Include test_tag2 so that only the instances on the lower terrain_entity_1 should be spawned.
    vegetation_entity.get_set_test(3, "Configuration|Inclusion|Surface Tags|[0]", surface_data.SurfaceTag("test_tag2"))

    helper.wait_for_condition(lambda: vegetation.VegetationSpawnerRequestBus(bus.Event, "GetAreaProductCount", vegetation_entity.id) == expected_tag2_included_item_count, 5.0)

    item_count = vegetation.VegetationSpawnerRequestBus(bus.Event, "GetAreaProductCount", vegetation_entity.id)
    Report.result(VegetationTests.testTag3_excluded_vegetation_count_correct, item_count == expected_tag2_included_item_count)
    Report.info(f"Found item count for testTag2 inclusion: {item_count}")

    highest_z, lowest_z = FindHighestAndLowestZValuesInArea(test_aabb)

    Report.result(VegetationTests.testTag3_excluded_vegetation_z_correct, highest_z < box_height * gradient_value_2)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(TerrainSystem_VegetationSpawnsOnTerrainSurfaces)
