"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def CreatePrefab_ComponentConfigurationRetained():
    """
    Test description:
    - Creates an entity with components set to specific values.
    - Creates a prefab of the entity.
    Test is successful if the new instanced prefab retains the expected component values
    """

    from pathlib import Path

    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    TEST_PREFAB_FILE_NAME = Path(__file__).stem + '_prefab'
    TEST_ENTITY_NAME = "TestEntity"

    prefab_test_utils.open_base_tests_level()

    # Creates a new Entity at the root level
    # Asserts if creation didn't succeed
    test_entity = EditorEntity.create_editor_entity(TEST_ENTITY_NAME)
    original_parent_id = test_entity.get_parent_id()
    assert test_entity.id.IsValid(), "Couldn't create parent entity"

    # Add Box Shape component to the entity
    test_entity.add_component("Box Shape")

    # Set the Box Shape component's properties
    box_shape_dimensions = math.Vector3(16.0, 16.0, 16.0)
    test_entity.components[0].set_component_property_value("Box Shape|Box Configuration|Dimensions",
                                                           box_shape_dimensions)

    # Asserts if prefab creation doesn't succeed
    test_prefab, test_instance = Prefab.create_prefab([test_entity], TEST_PREFAB_FILE_NAME)

    # Validate that the Box Shape component properties persist after prefab creation
    test_entity = test_instance.get_direct_child_entity_by_name(TEST_ENTITY_NAME)
    box_shape_component = test_entity.get_components_of_type(["Box Shape"])[0]
    prefab_box_shape_dimensions = box_shape_component.get_component_property_value(
        "Box Shape|Box Configuration|Dimensions")
    assert box_shape_dimensions == prefab_box_shape_dimensions, \
        f"Found unexpected values on the Box Shape component after prefab creation. Expected {box_shape_dimensions}, " \
        f"Found {prefab_box_shape_dimensions}."
    
    # Test undo/redo on prefab creation
    prefab_test_utils.validate_undo_redo_on_prefab_creation(test_instance, original_parent_id)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(CreatePrefab_ComponentConfigurationRetained)
