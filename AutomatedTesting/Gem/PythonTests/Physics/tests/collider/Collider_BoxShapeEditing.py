"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C4982801
Test Case Title : Verify that the shape Box can be selected from drop downlist and the value for its dimensions in x,y,z can be set after that

"""


# fmt: off
class Tests():
    entity_created           = ("Test Entity created successfully",          "Failed to create Test Entity")
    collider_added           = ("PhysX Collider added successfully",         "Failed to add PhysX Collider")
    collider_shape_changed   = ("PhysX Collider shape changed successfully", "Failed change PhysX Collider shape")
    shape_dimensions_changed = ("Shape dimensions modified successfully",    "Failed to modify Shape dimensions")
# fmt: on


def Collider_BoxShapeEditing():
    """
    Summary:
     Adding PhysX Collider and Shape components to test entity, then attempting to modify the shape's dimensions

    Expected Behavior:
     Box shape can be selected for the Shape Component and the value for X, Y, and Z dimensions can be changed

    Test Steps:
     1) Load the empty level
     2) Create the test entity
     3) Add PhysX Collider component to test entity
     4) Change the PhysX Collider shape and store the original dimensions
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

    # Open 3D Engine Imports
    import azlmbr.math as math

    BOX_SHAPETYPE_ENUM = 1
    SET_SIZE = 2.5
    SIZE_TOLERANCE = 0.5

    def check_dimensions_changed(set_dimension_value, modified_dimensions, tolerance):
        def compare_values(value_name, set_value, grabbed_value, tolerance):
            within_tolerance = math.Math_IsClose(set_value, grabbed_value, tolerance)
            if not within_tolerance:
                assert (
                    False
                ), f"The modified value for {value_name} was not within the allowed tolerance\nExpected:{set_value}\nActual: {grabbed_value}"
            return within_tolerance

        # Check for X, Y, & Z values
        x_value = compare_values("x_value", set_dimension_value, modified_dimensions.x, tolerance)
        y_value = compare_values("y_value", set_dimension_value, modified_dimensions.y, tolerance)
        z_value = compare_values("z_value", set_dimension_value, modified_dimensions.z, tolerance)

        return x_value and y_value and z_value

    helper.init_idle()
    # 1) Load the empty level
    helper.open_level("", "Base")

    # 2) Create the test entity
    test_entity = Entity.create_editor_entity("Test Entity")
    Report.result(Tests.entity_created, test_entity.id.IsValid())

    # 3) Add PhysX Collider component to test entity
    test_component = test_entity.add_component("PhysX Collider")
    Report.result(Tests.collider_added, test_entity.has_component("PhysX Collider"))

    # 4) Change the PhysX Collider shape and store the original dimensions
    test_component.set_component_property_value("Shape Configuration|Shape", BOX_SHAPETYPE_ENUM)
    add_check = test_component.get_component_property_value("Shape Configuration|Shape") == BOX_SHAPETYPE_ENUM
    Report.result(Tests.collider_shape_changed, add_check)

    # 5) Modify the dimensions
    Report.info(f"Attempting to set XYZ values to {SET_SIZE}")
    test_component.set_component_property_value(
        "Shape Configuration|Box|Dimensions", math.Vector3(SET_SIZE, SET_SIZE, SET_SIZE)
    )
    mod_dimensions = test_component.get_component_property_value("Shape Configuration|Box|Dimensions")

    # 6) Verify they have been changed
    dimensions_successfully_changed = check_dimensions_changed(SET_SIZE, mod_dimensions, SIZE_TOLERANCE)
    Report.result(Tests.shape_dimensions_changed, dimensions_successfully_changed)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_BoxShapeEditing)
