"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


#fmt: off
class Tests:
    create_test_entity                      = ("Entity created successfully",    "Failed to create Entity")
    add_axis_aligned_box_shape              = ("Axis Aligned Box Shape component added", "Failed to add Axis Aligned Box Shape component")
    add_terrain_collider                    = ("Terrain Physics Heightfield Collider component added", "Failed to add a Terrain Physics Heightfield Collider component")
    box_dimensions_changed                  = ("Aabb dimensions changed successfully", "Failed change Aabb dimensions")
    configuration_changed                   = ("Terrain size changed successfully", "Failed terrain size change")
#fmt: on


def TerrainPhysicsCollider_ChangesSizeWithAxisAlignedBoxShapeChanges():
    """
    Summary:
    Test aspects of the Terrain Physics Heightfield Collider through the BehaviorContext and the Property Tree.

    Test Steps:
    Expected Behavior:
    The Editor is stable there are no warnings or errors.

    Test Steps:
     1) Load the base level
     2) Create test entity
     3) Start the Tracer to catch any errors and warnings
     4) Add the Axis Aligned Box Shape and Terrain Physics Heightfield Collider components
     5) Change the Axis Aligned Box Shape dimensions
     6) Check the Heightfield provider is returning the correct size
     7) Verify there are no errors and warnings in the logs


    :return: None
    """

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Report, Tracer

    import azlmbr.physics as physics
    import azlmbr.math as azmath
    import azlmbr.bus as bus

    import math

    SET_BOX_X_SIZE = 5.0
    SET_BOX_Y_SIZE = 6.0

    # Our box is being created at (0,0), so it goes from (-2.5,-3) to (2.5,3)
    # The default terrain grid step size is 1, and we expect our physics heightfield to be aligned to the grid,
    # so the heightfield should go from (-2, -3) to (2, 3).
    # The heightfield is also expected to create final vertex endpoints in each direction, so we expect the 
    # X axis to create (-2, -1, 0, 1, 2) = 5 points, and the Y axis to create (-3, -2, -1, 0, 1, 2, 3) = 7 points
    EXPECTED_COLUMN_SIZE = 5
    EXPECTED_ROW_SIZE = 7
    
    # 1) Load the level
    hydra.open_base_level()

    #1a) Load the level components
    hydra.add_level_component("Terrain World")
    hydra.add_level_component("Terrain World Renderer")

    # 2) Create test entity
    test_entity = EditorEntity.create_editor_entity_at(azmath.Vector3(0.0, 0.0, 0.0), "TestEntity")
    Report.result(Tests.create_test_entity, test_entity.id.IsValid())

    # 3) Start the Tracer to catch any errors and warnings
    with Tracer() as section_tracer:
        # 4) Add the Axis Aligned Box Shape and Terrain Physics Heightfield Collider components
        aaBoxShape_component = test_entity.add_component("Axis Aligned Box Shape")
        Report.result(Tests.add_axis_aligned_box_shape, test_entity.has_component("Axis Aligned Box Shape"))
        terrainPhysics_component = test_entity.add_component("Terrain Physics Heightfield Collider")
        Report.result(Tests.add_terrain_collider, test_entity.has_component("Terrain Physics Heightfield Collider"))

        # 5) Change the Axis Aligned Box Shape dimensions
        aaBoxShape_component.set_component_property_value("Axis Aligned Box Shape|Box Configuration|Dimensions", azmath.Vector3(SET_BOX_X_SIZE, SET_BOX_Y_SIZE, 1.0))
        add_check = aaBoxShape_component.get_component_property_value("Axis Aligned Box Shape|Box Configuration|Dimensions") == azmath.Vector3(SET_BOX_X_SIZE, SET_BOX_Y_SIZE, 1.0)
        Report.result(Tests.box_dimensions_changed, add_check)

        # 6) Check the Heightfield provider is returning the correct size
        columns = physics.HeightfieldProviderRequestsBus(bus.Broadcast, "GetHeightfieldGridColumns")
        rows = physics.HeightfieldProviderRequestsBus(bus.Broadcast, "GetHeightfieldGridRows")
        Report.result(Tests.configuration_changed, math.isclose(columns, EXPECTED_COLUMN_SIZE) and math.isclose(rows, EXPECTED_ROW_SIZE))

    helper.wait_for_condition(lambda: section_tracer.has_errors or section_tracer.has_asserts, 1.0)
    for error_info in section_tracer.errors:
        Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
    for assert_info in section_tracer.asserts:
        Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")

if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(TerrainPhysicsCollider_ChangesSizeWithAxisAlignedBoxShapeChanges)
