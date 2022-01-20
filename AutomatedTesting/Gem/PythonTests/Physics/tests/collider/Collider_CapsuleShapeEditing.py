"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C4982802
Test Case Title : Verify that the shape capsule can be selected from drop downlist and the value for its height and radius can be set after that

"""


# fmt: off
class Tests():
    entity_created           = ("Test Entity created successfully",          "Failed to create Test Entity")
    collider_added           = ("PhysX Collider added successfully",         "Failed to add PhysX Collider")
    collider_shape_changed   = ("PhysX Collider shape changed successfully", "Failed change PhysX Collider shape")
    shape_dimensions_changed = ("Shape dimensions modified successfully",    "Failed to modify Shape dimensions")
# fmt: on


def Collider_CapsuleShapeEditing():
    """
    Summary:
     Adding PhysX Collider and Shape components to test entity, then attempting to modify the shape's dimensions

    Expected Behavior:
     Capsule shape can be selected for the Shape Component and the value for Height and Radius can be changed

    Test Steps:
     1) Load the empty level
     2) Create the test entity
     3) Add PhysX Collider component to test entity
     4) Change the PhysX Collider shape
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

    CAPSULE_SHAPETYPE_ENUM = 2
    # Note from Editor Console: Height must exceed twice the radius in capsule configuration
    SIZE_RADIUS = 4.0
    SIZE_HEIGHT = SIZE_RADIUS * 2 + 1.0
    SIZE_TOLERANCE = 0.5

    def change_dimension_value(component, component_property_path, value):
        Report.info(f"Attempting to set value for {component_property_path} to {value}")
        component.set_component_property_value(component_property_path, value)
        returning_value = component.get_component_property_value(component_property_path)
        Report.info(f"Value for {component_property_path} is currently {returning_value}")
        return returning_value

    def check_dimension_change(value_name, value_set, grabbed_value, tolerance):
        within_tolerance = math.Math_IsClose(value_set, grabbed_value, tolerance)
        if not within_tolerance:
            assert (
                False
            ), f"The modified value for {value_name} was not within the allowed tolerance\nExpected:{value_set}\nActual: {grabbed_value}"
        return within_tolerance

    helper.init_idle()
    # 1) Load the empty level
    helper.open_level("", "Base")

    # 2) Create the test entity
    test_entity = Entity.create_editor_entity("Test Entity")
    Report.result(Tests.entity_created, test_entity.id.IsValid())

    # 3) Add PhysX Collider component to test entity
    test_component = test_entity.add_component("PhysX Collider")
    Report.result(Tests.collider_added, test_entity.has_component("PhysX Collider"))

    # 4) Change the PhysX Collider shape
    test_component.set_component_property_value("Shape Configuration|Shape", CAPSULE_SHAPETYPE_ENUM)
    add_check = test_component.get_component_property_value("Shape Configuration|Shape") == CAPSULE_SHAPETYPE_ENUM
    Report.result(Tests.collider_shape_changed, add_check)

    # 5) Modify the dimensions
    mod_height = change_dimension_value(test_component, "Shape Configuration|Capsule|Height", SIZE_HEIGHT)
    mod_radius = change_dimension_value(test_component, "Shape Configuration|Capsule|Radius", SIZE_RADIUS)

    # 6) Verify they have been changed
    resulting_check = check_dimension_change(
        "Height", SIZE_HEIGHT, mod_height, SIZE_TOLERANCE
    ) and check_dimension_change("Radius", SIZE_RADIUS, mod_radius, SIZE_TOLERANCE)
    Report.result(Tests.shape_dimensions_changed, resulting_check)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_CapsuleShapeEditing)
