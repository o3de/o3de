"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def InstantiatePrefab_WithNestedEntities():
    """
    Test description:
    - Creates linear nested entities.
    - Creates a prefab from the nested entities.
    - Instantiates another copy of the prefab and validates the structure is intact.

    Test passes if the 3 entities inside the newly instanced prefab have correct hierarchy and positions.
    """

    from pathlib import Path

    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.prefab_utils import Prefab
    from editor_python_test_tools.wait_utils import PrefabWaiter
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    NESTED_ENTITIES_PREFAB_FILE_NAME = Path(__file__).stem + 'nested_entities_prefab'
    NESTED_ENTITIES_NAME_PREFIX = 'Entity_'
    POSITION = math.Vector3(100.0, 100.0, 100.0)
    NUM_NESTED_ENTITIES_LEVELS = 3

    prefab_test_utils.open_base_tests_level()

    # Creates new nested entities at the root level
    # Asserts if creation didn't succeed
    nested_entities_root = prefab_test_utils.create_linear_nested_entities(NESTED_ENTITIES_NAME_PREFIX,
                                                                           NUM_NESTED_ENTITIES_LEVELS, POSITION)
    nested_entities_root_parent = EditorEntity(nested_entities_root.get_parent_id())
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root, NUM_NESTED_ENTITIES_LEVELS, POSITION)

    # Asserts if prefab creation doesn't succeed
    nested_entities_prefab, nested_entities_prefab_instance = Prefab.create_prefab([nested_entities_root],
                                                                                   NESTED_ENTITIES_PREFAB_FILE_NAME)
    nested_entities_root_on_instance = nested_entities_prefab_instance.get_direct_child_entities()[0]
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root_on_instance, NUM_NESTED_ENTITIES_LEVELS,
                                                      POSITION)

    # Instantiate another copy of the prefab, and validate its hierarchy and position
    nested_entities_prefab_instance_2 = nested_entities_prefab.instantiate(prefab_position=POSITION)
    nested_entities_root_on_instance_2 = nested_entities_prefab_instance_2.get_direct_child_entities()[0]
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root_on_instance_2, NUM_NESTED_ENTITIES_LEVELS,
                                                      POSITION)

    # Undo the instantiation
    general.undo()
    PrefabWaiter.wait_for_propagation()
    child_ids = nested_entities_root_parent.get_children_ids()
    assert nested_entities_prefab_instance_2.container_entity.id not in child_ids, \
        "Undo Failed: Instance was still found after undo."

    # Redo the instantiation
    general.redo()
    PrefabWaiter.wait_for_propagation()
    child_ids = nested_entities_root_parent.get_children_ids()
    assert nested_entities_prefab_instance_2.container_entity.id in child_ids, \
        "Redo Failed: Instance was not found after redo"
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root_on_instance_2, NUM_NESTED_ENTITIES_LEVELS,
                                                      POSITION)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(InstantiatePrefab_WithNestedEntities)
