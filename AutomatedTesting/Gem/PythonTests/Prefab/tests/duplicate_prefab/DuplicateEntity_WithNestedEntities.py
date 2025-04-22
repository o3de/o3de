"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def DuplicateEntity_WithNestedEntities():
    """
    Test description:
    - Creates linear nested entities.
    - Duplicates the nested entity structure, and validates.
    - Validates Undo/Redo operations on the duplication.
    """

    import azlmbr.bus as bus
    import azlmbr.legacy.general as general
    import azlmbr.prefab as prefab

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.wait_utils import PrefabWaiter
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    NESTED_ENTITIES_NAME_PREFIX = 'Entity_'
    CREATION_POSITION = azlmbr.math.Vector3(100.0, 100.0, 100.0)
    NUM_NESTED_ENTITIES_LEVELS = 3

    prefab_test_utils.open_base_tests_level()

    # Creates new nested entities at the root level
    # Asserts if creation didn't succeed
    nested_entities_root = prefab_test_utils.create_linear_nested_entities(
        NESTED_ENTITIES_NAME_PREFIX, NUM_NESTED_ENTITIES_LEVELS, CREATION_POSITION)
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root, NUM_NESTED_ENTITIES_LEVELS,
                                                      CREATION_POSITION)

    # Duplicates the entity hierarchy and validates
    duplicate_outcome = prefab.PrefabPublicRequestBus(bus.Broadcast, 'DuplicateEntitiesInInstance',
                                                      [nested_entities_root.id])
    assert duplicate_outcome.IsSuccess(), \
        f"Failed to duplicate nested entities with root entity {nested_entities_root.get_name()}"
    PrefabWaiter.wait_for_propagation()
    root_entities = EditorEntity.find_editor_entities(["Entity_0"])
    assert len(root_entities) == 2, "Failed to find duplicated root entity"
    for root_entity in root_entities:
        prefab_test_utils.validate_linear_nested_entities(root_entity, NUM_NESTED_ENTITIES_LEVELS, CREATION_POSITION)

    # Test undo/redo on nested hierarchy duplication
    general.undo()
    PrefabWaiter.wait_for_propagation()
    root_entities = EditorEntity.find_editor_entities(["Entity_0"])
    assert len(root_entities) == 1, f"Undo Failed: Found {len(root_entities)}, when 1 should be present"

    general.redo()
    PrefabWaiter.wait_for_propagation()
    root_entities = EditorEntity.find_editor_entities(["Entity_0"])
    assert len(root_entities) == 2, f"Undo Failed: Found {len(root_entities)}, when 1 should be present"
    for root_entity in root_entities:
        prefab_test_utils.validate_linear_nested_entities(root_entity, NUM_NESTED_ENTITIES_LEVELS, CREATION_POSITION)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(DuplicateEntity_WithNestedEntities)
