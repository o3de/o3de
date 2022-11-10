"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def DuplicateEntity_WithNestedEntitiesAndNestedPrefabs():
    """
    Test description:
    - Creates a root entity.
    - Creates linear nested entities with the root entity as a parent.
    - Creates linear nested prefabs with the root entity as a parent.
    - Duplicates the entire hierarchy, and validates.
    - Validates Undo/Redo operations on the duplication.
    """

    from pathlib import Path

    import azlmbr.bus as bus
    import azlmbr.legacy.general as general
    import azlmbr.prefab as prefab

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.wait_utils import PrefabWaiter
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    NESTED_ENTITIES_NAME_PREFIX = 'NestedEntity_'
    NESTED_PREFABS_FILE_NAME_PREFIX = Path(__file__).stem + '_' + 'nested_prefabs_'
    NESTED_PREFABS_NAME_PREFIX = 'NestedPrefabs_Prefab_'
    NESTED_PREFABS_TEST_ENTITY_NAME = 'TestEntity'
    PHYSX_COLLIDER_NAME = 'PhysX Collider'
    PARENT_CREATION_POSITION = azlmbr.math.Vector3(0.0, 0.0, 0.0)
    CHILDREN_CREATION_POSITION = azlmbr.math.Vector3(100.0, 100.0, 100.0)
    NUM_NESTED_ENTITIES_LEVELS = 3
    NUM_NESTED_PREFABS_LEVELS = 3

    def validate_duplicated_hierarchy():
        root_entities = EditorEntity.find_editor_entities(["Parent"])
        assert len(root_entities) == 2, "Failed to find expected duplicated entity hierarchy root"
        for root_entity in root_entities:
            expected_children_names = [f"{NESTED_PREFABS_NAME_PREFIX}0", f"{NESTED_ENTITIES_NAME_PREFIX}0"]
            child_entities = root_entity.get_children()
            for child in child_entities:
                assert child.get_name() in expected_children_names, "Failed to find expected children of root entity"
                # Validate the duplicated child nested entity hierarchy
                if child.get_name() == f"{NESTED_ENTITIES_NAME_PREFIX}0":
                    prefab_test_utils.validate_linear_nested_entities(child, NUM_NESTED_ENTITIES_LEVELS,
                                                                      CHILDREN_CREATION_POSITION)
                # Validate the duplicated child nested prefab hierarchy
                elif child.get_name() == f"{NESTED_PREFABS_NAME_PREFIX}0":
                    first_child_prefab = child.get_children()[0]
                    assert first_child_prefab.get_name() == f"{NESTED_PREFABS_NAME_PREFIX}1", \
                        f"Found unexpected child after duplication. Expected {NESTED_PREFABS_NAME_PREFIX}1, " \
                        f"found {first_child_prefab.get_name()}."
                    second_child_prefab = first_child_prefab.get_children()[0]
                    assert second_child_prefab.get_name() == f"{NESTED_PREFABS_NAME_PREFIX}2", \
                        f"Found unexpected child after duplication. Expected {NESTED_PREFABS_NAME_PREFIX}2, " \
                        f"found {second_child_prefab.get_name()}."

    prefab_test_utils.open_base_tests_level()

    # Creates a new entity to serve as the parent for the nested entity and nested prefab hierarchies
    parent_entity = EditorEntity.create_editor_entity_at(PARENT_CREATION_POSITION, "Parent")

    # Creates new nested entities as children of the parent entity
    # Asserts if creation didn't succeed
    nested_entities_root = prefab_test_utils.create_linear_nested_entities(
        NESTED_ENTITIES_NAME_PREFIX, NUM_NESTED_ENTITIES_LEVELS, CHILDREN_CREATION_POSITION, parent_id=parent_entity.id)
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root, NUM_NESTED_ENTITIES_LEVELS,
                                                      CHILDREN_CREATION_POSITION)

    # Creates new nested prefabs at the root level
    # Asserts if creation didn't succeed
    nested_prefab_root = EditorEntity.create_editor_entity_at(CHILDREN_CREATION_POSITION, name=NESTED_PREFABS_TEST_ENTITY_NAME,
                                                              parent_id=parent_entity.id)
    assert nested_prefab_root.id.IsValid(), "Couldn't create TestEntity"
    nested_prefab_root.add_component(PHYSX_COLLIDER_NAME)
    assert nested_prefab_root.has_component(PHYSX_COLLIDER_NAME), f"Failed to add a {PHYSX_COLLIDER_NAME}"
    nested_prefabs, nested_prefab_instances = prefab_test_utils.create_linear_nested_prefabs(
        [nested_prefab_root], NESTED_PREFABS_FILE_NAME_PREFIX, NESTED_PREFABS_NAME_PREFIX, NUM_NESTED_PREFABS_LEVELS)
    prefab_test_utils.validate_linear_nested_prefab_instances_hierarchy(nested_prefab_instances)

    # Duplicates the entity hierarchy and validates
    duplicate_outcome = prefab.PrefabPublicRequestBus(bus.Broadcast, 'DuplicateEntitiesInInstance',
                                                      [parent_entity.id])
    assert duplicate_outcome.IsSuccess(), \
        f"Failed to duplicate nested entity hierarchy with root entity {parent_entity.get_name()}"
    PrefabWaiter.wait_for_propagation()
    validate_duplicated_hierarchy()
    
    # Test undo/redo on entity hierarchy duplication
    general.undo()
    PrefabWaiter.wait_for_propagation()
    root_entities = EditorEntity.find_editor_entities(["Parent"])
    assert len(root_entities) == 1, "Undo Failed: Found unexpected duplicated entity hierarchy root after Undo"
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root, NUM_NESTED_ENTITIES_LEVELS,
                                                      CHILDREN_CREATION_POSITION)
    prefab_test_utils.validate_linear_nested_prefab_instances_hierarchy(nested_prefab_instances)

    general.redo()
    PrefabWaiter.wait_for_propagation()
    validate_duplicated_hierarchy()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(DuplicateEntity_WithNestedEntitiesAndNestedPrefabs)
