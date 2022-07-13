"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def CreatePrefab_WithNestedEntities():
    """
    Test description:
    - Creates linear nested entities. 
    - Creates a prefab from the nested entities.

    Test passed if the 3 entities inside the newly instanced prefab have correct hierarchy and positions.
    """

    from pathlib import Path

    from editor_python_test_tools.prefab_utils import Prefab
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    NESTED_ENTITIES_PREFAB_FILE_NAME = Path(__file__).stem + 'nested_entities_prefab'
    NESTED_ENTITIES_NAME_PREFIX = 'Entity_'
    OLD_POSITION = azlmbr.math.Vector3(100.0, 100.0, 100.0)
    NEW_POSITION = azlmbr.math.Vector3(200.0, 200.0, 200.0)
    NUM_NESTED_ENTITIES_LEVELS = 3

    prefab_test_utils.open_base_tests_level()

    # Creates new nested entities at the root level
    # Asserts if creation didn't succeed
    nested_entities_root = prefab_test_utils.create_linear_nested_entities(NESTED_ENTITIES_NAME_PREFIX, NUM_NESTED_ENTITIES_LEVELS, OLD_POSITION)
    nested_entities_root_parent = nested_entities_root.get_parent_id()
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root, NUM_NESTED_ENTITIES_LEVELS, OLD_POSITION)

    # Asserts if prefab creation doesn't succeed
    _, nested_entities_prefab_instance = Prefab.create_prefab([nested_entities_root], NESTED_ENTITIES_PREFAB_FILE_NAME)
    nested_entities_root_on_instance = nested_entities_prefab_instance.get_direct_child_entities()[0]
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root_on_instance, NUM_NESTED_ENTITIES_LEVELS, OLD_POSITION)

    # Test undo/redo on prefab creation
    prefab_test_utils.validate_undo_redo_on_prefab_creation(nested_entities_prefab_instance, nested_entities_root_parent)

    # Moves the position of root of the nested entities, it should also update all the entities' positions
    nested_entities_root_on_instance.set_world_translation(NEW_POSITION)
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root_on_instance, NUM_NESTED_ENTITIES_LEVELS, NEW_POSITION)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(CreatePrefab_WithNestedEntities)
