"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#fmt: off
class Tests():
    create_test_entity                      = ("Entity created successfully",    "Failed to create Entity")
    add_physx_heightfield_collider          = ("PhysX Heightfield Collider component added", "Failed to add PhysX Heightfield Collider component")
    add_axis_aligned_box_shape              = ("Axis Aligned Box Shape component added", "Failed to add Axis Aligned Box Shape component")
    add_terrain_collider                    = ("Terrain Physics Heightfield Collider component added", "Failed to add ATerrain Physics Heightfield Collider component")
    box_dimensions_changed                  = ("Aabb dimensions changed successfully", "Failed change Aabb dimensions")
    configuration_changed                   = ("Terrain size changed successfully", "Failed terrain size change")
    no_errors_and_warnings_found            = ("No errors and warnings found",   "Found errors and warnings")
#fmt: on

def TerrainPhysicsCollider_ChangesSizeWithAxisAlignedBoxShapeChanges():
    """
    Summary:
    Test aspects of the TerrainHeightGradientList through the BehaviorContext and the Property Tree.

    Test Steps:
    Expected Behavior:
    The Editor is stable there are no warnings or errors.

    Test Steps:
     1) Load the base level
     2) Create test entity
     3) Start the Tracer to catch any errors and warnings
     4) Add the PhysX Heightfield Collider, Axis Aligned Box Shape and Terrain Physics Heightfield Collider components
     5) Change the Axis Aligned Box Shape dimensions
     6) Chaeck the Heightfield provider is returning the correct size
     7) Verify there are no errors and warnings in the logs


    :return: None
    """

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import Tracer
    import azlmbr.legacy.general as general
    import azlmbr.physics as physics
    import azlmbr.math as math2
    import azlmbr.bus as bus
    import sys
    import math

    SET_BOX_X_SIZE = 5.0
    SET_BOX_Y_SIZE = 6.0
    EXPECTED_COLUMN_SIZE = SET_BOX_X_SIZE + 1
    EXPECTED_ROW_SIZE = SET_BOX_Y_SIZE + 1
    '''
    helper.init_idle()
    
    # 1) Load the level
    helper.open_level("", "Base")
    
    # 2) Create test entity
    test_entity = EditorEntity.create_editor_entity("TestEntity")
    Report.result(Tests.create_test_entity, test_entity.id.IsValid())

    # 3) Start the Tracer to catch any errors and warnings
    with Tracer() as section_tracer:
        # 4) Add the Axis Aligned Box Shape and Terrain Physics Heightfield Collider components
        aaBoxShape_component = test_entity.add_component("Axis Aligned Box Shape")
        Report.result(Tests.add_axis_aligned_box_shape, test_entity.has_component("Axis Aligned Box Shape"))
        terrainPhysics_component = test_entity.add_component("Terrain Physics Heightfield Collider")
        Report.result(Tests.add_terrain_collider, test_entity.has_component("Terrain Physics Heightfield Collider"))
        # 5) Change the Axis Aligned Box Shape dimensions
        aaBoxShape_component.set_component_property_value("Axis Aligned Box Shape|Box Configuration|Dimensions", math2.Vector3(SET_BOX_X_SIZE, SET_BOX_Y_SIZE, 1.0))
        add_check = aaBoxShape_component.get_component_property_value("Axis Aligned Box Shape|Box Configuration|Dimensions") == math2.Vector3(SET_BOX_X_SIZE, SET_BOX_Y_SIZE, 1.0)
        Report.result(Tests.box_dimensions_changed, add_check)

        # 6) Chaeck the Heightfield provider is returning the correct size
        columns = physics.HeightfieldProviderRequestsBus(bus.Broadcast, "GetHeightfieldGridColumns")
        rows = physics.HeightfieldProviderRequestsBus(bus.Broadcast, "GetHeightfieldGridRows")

        Report.result(Tests.configuration_changed, math.isclose(columns, EXPECTED_COLUMN_SIZE) and math.isclose(rows, EXPECTED_ROW_SIZE))
        '''
if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(AxisAlignedBoxShape_ConfigurationWorks)
