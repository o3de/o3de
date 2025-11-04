"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C4982800
Test Case Title : Verify that the shape Sphere can be selected from the drop downlist and the value for its radius can be set

"""


# fmt: off
class Tests():
    entity_created           = ("Test Entity created successfully",          "Failed to create Test Entity")
    collider_added           = ("PhysX Primitive Collider added successfully",         "Failed to add PhysX Primitive Collider")
    collider_shape_changed   = ("PhysX Primitive Collider shape changed successfully", "Failed change PhysX Primitive Collider shape")
    shape_dimensions_changed = ("Shape dimensions modified successfully",    "Failed to modify Shape dimensions")
# fmt: on


def Collider_SphereShapeEditing():
    """
    Summary:
     Adding PhysX Primitive Collider and Shape components to test entity, then attempting to modify the shape's dimensions

    Expected Behavior:
     Sphere shape can be selected for the Shape Component and the value for Radius can be changed

    Test Steps:
     1) Load the empty level
     2) Create the test entity
     3) Add PhysX Primitive Collider component to test entity
     4) Change the PhysX Primitive Collider shape and store the original dimensions
     5) Modify the dimensions
     6) Verify they have been changed

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    # Helper Files

    from editor_python_test_tools.editor_entity_utils import EditorEntity as Entity
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from consts.physics import PHYSX_PRIMITIVE_COLLIDER

    # Open 3D Engine Imports
    import azlmbr.math as math

    SPHERE_SHAPETYPE_ENUM = 0
    DIMENSION_TO_SET = 2.5
    DIMENSION_SIZE_TOLERANCE = 0.5

    helper.init_idle()
    # 1) Load the empty level
    helper.open_level("", "Base")

    # 2) Create the test entity
    test_entity = Entity.create_editor_entity("Test Entity")
    test_entity.add_component("PhysX Static Rigid Body")
    Report.result(Tests.entity_created, test_entity.id.IsValid())

    # 3) Add PhysX Primitive Collider component to test entity
    test_component = test_entity.add_component(PHYSX_PRIMITIVE_COLLIDER)
    Report.result(Tests.collider_added, test_entity.has_component(PHYSX_PRIMITIVE_COLLIDER))

    # 4) Change the PhysX Primitive Collider shape and store the original dimensions
    test_component.set_component_property_value("Shape Configuration|Shape", SPHERE_SHAPETYPE_ENUM)
    add_check = test_component.get_component_property_value("Shape Configuration|Shape") == SPHERE_SHAPETYPE_ENUM
    Report.result(Tests.collider_shape_changed, add_check)

    # 5) Modify the dimensions
    test_component.set_component_property_value("Shape Configuration|Sphere|Radius", DIMENSION_TO_SET)

    # 6) Verify they have been changed
    modified_dimensions = test_component.get_component_property_value("Shape Configuration|Sphere|Radius")

    dimensions_successfully_modified = math.Math_IsClose(
        DIMENSION_TO_SET, modified_dimensions, DIMENSION_SIZE_TOLERANCE
    )
    if not dimensions_successfully_modified:
        assert (
            False
        ), f"The modified value was not within the allowed tolerance\nExpected:{DIMENSION_TO_SET}\nActual: {modified_dimensions}"
    Report.result(Tests.shape_dimensions_changed, dimensions_successfully_modified)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_SphereShapeEditing)
