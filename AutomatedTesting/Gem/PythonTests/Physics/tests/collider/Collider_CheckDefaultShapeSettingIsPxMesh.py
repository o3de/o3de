"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C14861500
Test Case Title : Verify Default shape is Physics Asset

"""


# fmt: off
class Tests():
    create_collider_entity = ("Created Collider Entity",         "Failed to create Collider Entity")
    add_physx_collider     = ("PhysX Collider added",            "Failed to add PhysX Collider")
    shape_is_correct       = ("PhysX Collider Shape is correct", "PhysX Collider Shape is not PhysicsAsset")
# fmt: on


def Collider_CheckDefaultShapeSettingIsPxMesh():
    """
    Summary:
     Check the default for Shape on the PhysX Collider component

    Expected Behavior:
     When adding the PhysX Collider, the default Shape should be PhysX Asset

    Test Steps:
     1) Load empty level
     2) Create an entity to hold the PhysX Shape Collider component
     3) Add the PhysX Collider component
     4) Check value of Shape property on PhysX Collider

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
    import azlmbr.legacy.general as general

    PHYSICS_ASSET_INDEX = 7  # Hardcoded enum index value for Shape property

    helper.init_idle()
    # 1) Load empty level
    helper.open_level("Physics", "Base")

    # 2) Create an entity to hold the PhysX Shape Collider component
    collider_entity = Entity.create_editor_entity("Collider")
    Report.result(Tests.create_collider_entity, collider_entity.id.IsValid())

    # 3) Add the PhysX Collider component
    test_component = collider_entity.add_component("PhysX Collider")
    Report.result(Tests.add_physx_collider, collider_entity.has_component("PhysX Collider"))

    # 4) Check value of Shape property on PhysX Collider
    value_to_test = test_component.get_component_property_value("Shape Configuration|Shape")
    Report.result(Tests.shape_is_correct, value_to_test == PHYSICS_ASSET_INDEX)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_CheckDefaultShapeSettingIsPxMesh)
