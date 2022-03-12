"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.
SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class HeightTests:
    single_gradient_height_correct = (
        "Successfully retrieved height for gradient1.",
        "Failed to retrieve height for gradient1."
    )
    double_gradient_height_correct = (
        "Successfully retrieved height when two gradients exist.",
        "Failed to retrieve height when two gradients exist."
    )
    triple_gradient_height_correct = (
        "Successfully retrieved height when three gradients exist.",
        "Failed to retrieve height when three gradients exist."
    )
    terrain_data_changed_call_count_correct = (
        "OnTerrainDataChanged called expected number of times.",
        "OnTerrainDataChanged call count incorrect."
    )

def TerrainHeightGradientList_AddRemoveGradientWorks():
    """
    Summary:
    Test aspects of the TerrainHeightGradientList through the BehaviorContext and the Property Tree.
    :return: None
    """

    import os
    import math as sys_math

    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.math as math
    import azlmbr.terrain as terrain
    import azlmbr.editor as editor
    import azlmbr.vegetation as vegetation
    import azlmbr.entity as EntityId

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import editor_python_test_tools.pyside_utils as pyside_utils
    from editor_python_test_tools.editor_entity_utils import EditorEntity

    terrain_changed_call_count = 0
    expected_terrain_changed_calls = 0

    aabb_component_name = "Axis Aligned Box Shape"
    gradientlist_component_name = "Terrain Height Gradient List"
    layerspawner_component_name = "Terrain Layer Spawner"

    gradient_value_path = "Configuration|Value"

    def create_entity_at(entity_name, components_to_add, x, y, z):
        entity = hydra.Entity(entity_name)
        entity.create_entity(math.Vector3(x, y, z), components_to_add)

        return entity
        
    def on_terrain_changed(args):
        nonlocal terrain_changed_call_count

        terrain_changed_call_count += 1

    def set_component_path_val(entity, component, path, value):
        entity.get_set_test(component, path, value)

    def set_gradients_check_height(main_entity, gradient_list, expected_height, test_results):
        nonlocal expected_terrain_changed_calls

        test_tolerance = 0.01
        gradient_list_path = "Configuration|Gradient Entities"

        set_component_path_val(main_entity, 1, gradient_list_path, gradient_list)

        expected_terrain_changed_calls += 1

        # Wait until the terrain data has been updated.
        helper.wait_for_condition(lambda: terrain_changed_call_count == expected_terrain_changed_calls, 2.0)

        # Get the height at the origin.
        height = terrain.TerrainDataRequestBus(bus.Broadcast, "GetHeightFromFloats", 0.0, 0.0, 0)

        Report.result(test_results, sys_math.isclose(height, expected_height, abs_tol=test_tolerance))

    helper.init_idle()

    # Open a level.
    helper.open_level("Physics", "Base")
    helper.wait_for_condition(lambda: general.get_current_level_name() == "Base", 2.0)
    
    general.idle_wait_frames(1)

    # Add a terrain world component
    world_component = hydra.add_level_component("Terrain World")

    aabb_height = 1024.0
    box_dimensions = math.Vector3(1.0, 1.0, aabb_height);

     # Create a main entity with a LayerSpawner, AAbb and HeightGradientList.
    main_entity = create_entity_at("entity2", [layerspawner_component_name, gradientlist_component_name, aabb_component_name], 0.0, 0.0, aabb_height/2.0)

    # Create three gradient entities.
    gradient_entity1 = create_entity_at("Constant Gradient1", ["Constant Gradient"], 0.0, 0.0, 0.0);
    gradient_entity2 = create_entity_at("Constant Gradient2", ["Constant Gradient"], 0.0, 0.0, 0.0);
    gradient_entity3 = create_entity_at("Constant Gradient3", ["Constant Gradient"], 0.0, 0.0, 0.0);

    # Give everything a chance to finish initializing.
    general.idle_wait_frames(1)

    # Set the gradients to different values.
    gradient_values = [0.5, 0.8, 0.3]
    set_component_path_val(gradient_entity1, 0, gradient_value_path, gradient_values[0])
    set_component_path_val(gradient_entity2, 0, gradient_value_path, gradient_values[1])
    set_component_path_val(gradient_entity3, 0, gradient_value_path, gradient_values[2])

    # Give the TerrainSystem time to tick.
    general.idle_wait_frames(1)

    # Set the dimensions of the Aabb.
    set_component_path_val(main_entity, 2, "Axis Aligned Box Shape|Box Configuration|Dimensions", box_dimensions)

    # Set up a handler to wait for notifications from the TerrainSystem.
    handler = azlmbr.terrain.TerrainDataNotificationBusHandler()
    handler.connect()
    handler.add_callback("OnTerrainDataChanged", on_terrain_changed)

    # Add a gradient to GradientList, then check the height returned from the TerrainSystem is correct.
    set_gradients_check_height(main_entity, [gradient_entity1.id], aabb_height * gradient_values[0], HeightTests.single_gradient_height_correct)

    # Add gradient2 and check height at the origin, this should have changed to match the second gradient value.
    set_gradients_check_height(main_entity, [gradient_entity1.id, gradient_entity2.id], aabb_height * gradient_values[1], HeightTests.double_gradient_height_correct)

    # Add gradient3, the  height should still be the second value, as that was the highest.
    set_gradients_check_height(main_entity, [gradient_entity1.id, gradient_entity2.id, gradient_entity3.id], aabb_height * gradient_values[1], HeightTests.triple_gradient_height_correct)

if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(TerrainHeightGradientList_AddRemoveGradientWorks)