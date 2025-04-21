"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def CreatePrefab_WithNestedEntitiesAndNestedPrefabs():
    """
    Test description:
    - Creates linear nested entities. 
    - Creates linear nested prefabs based of an entity with a physx collider.
    - Creates a prefab from the nested entities and the nested prefabs.

    Test passed if the the nested entities/prefabs under the newly instanced prefab 
    have correct hierarchy and positions.
    """

    from pathlib import Path

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    from consts.physics import PHYSX_PRIMITIVE_COLLIDER as PHYSX_PRIMITIVE_COLLIDER_NAME
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    NESTED_ENTITIES_PREFAB_FILE_NAME = Path(__file__).stem + '_' + 'nested_entities_prefab'
    NESTED_ENTITIES_NAME_PREFIX = 'Entity_'
    NESTED_PREFABS_FILE_NAME_PREFIX = Path(__file__).stem + '_' + 'nested_prefabs_'
    NESTED_PREFABS_NAME_PREFIX = 'NestedPrefabs_Prefab_'
    FILE_NAME_OF_PREFAB_WITH_NESTED_ENTITIES_AND_NESTED_PREFABS = Path(__file__).stem + '_' + 'new_prefab'
    NESTED_PREFABS_TEST_ENTITY_NAME = 'TestEntity'
    CREATION_POSITION = azlmbr.math.Vector3(100.0, 100.0, 100.0)
    NUM_NESTED_ENTITIES_LEVELS = 3
    NUM_NESTED_PREFABS_LEVELS = 3

    prefab_test_utils.open_base_tests_level()

    # Creates new nested entities at the root level
    # Asserts if creation didn't succeed
    nested_entities_root = prefab_test_utils.create_linear_nested_entities(
        NESTED_ENTITIES_NAME_PREFIX, NUM_NESTED_ENTITIES_LEVELS, CREATION_POSITION)
    nested_entities_root_parent = nested_entities_root.get_parent_id()
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root, NUM_NESTED_ENTITIES_LEVELS, CREATION_POSITION)
    nested_entities_root_name = nested_entities_root.get_name()

    # Creates a new nested prefabs at the root level
    # Asserts if creation didn't succeed
    entity = EditorEntity.create_editor_entity_at(CREATION_POSITION, name=NESTED_PREFABS_TEST_ENTITY_NAME)
    assert entity.id.IsValid(), "Couldn't create TestEntity"
    entity.add_component(PHYSX_PRIMITIVE_COLLIDER_NAME)
    assert entity.has_component(PHYSX_PRIMITIVE_COLLIDER_NAME), f"Failed to add a {PHYSX_PRIMITIVE_COLLIDER_NAME}"
    _, nested_prefab_instances = prefab_test_utils.create_linear_nested_prefabs(
        [entity], NESTED_PREFABS_FILE_NAME_PREFIX, NESTED_PREFABS_NAME_PREFIX, NUM_NESTED_PREFABS_LEVELS)
    prefab_test_utils.validate_linear_nested_prefab_instances_hierarchy(nested_prefab_instances)

    # Asserts if prefab creation doesn't succeed
    _, new_prefab = Prefab.create_prefab(
        [nested_entities_root, nested_prefab_instances[0].container_entity], FILE_NAME_OF_PREFAB_WITH_NESTED_ENTITIES_AND_NESTED_PREFABS)
    new_prefab_container_entity = new_prefab.container_entity

    nested_entities_root_on_instance = new_prefab.get_direct_child_entities()[0]
    nested_prefabs_root_container_entity_on_instance = new_prefab.get_direct_child_entities()[1]
    if new_prefab.get_direct_child_entities()[0].get_name() != nested_entities_root_name:
        nested_entities_root_on_instance, nested_prefabs_root_container_entity_on_instance = \
            nested_prefabs_root_container_entity_on_instance, nested_entities_root_on_instance

    assert nested_entities_root_on_instance.get_name() == nested_entities_root_name \
        and nested_entities_root_on_instance.get_parent_id() == new_prefab_container_entity.id, \
        f"The name of the first child entity of the new prefab '{new_prefab_container_entity.get_name()}' " \
        f"should be '{nested_entities_root_name}', " \
        f"not '{nested_entities_root_on_instance.get_name()}'"
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root_on_instance, NUM_NESTED_ENTITIES_LEVELS, CREATION_POSITION)

    # Test undo/redo on prefab creation
    prefab_test_utils.validate_undo_redo_on_prefab_creation(new_prefab, nested_entities_root_parent)

    parent_prefab_container_entity_on_instance = new_prefab_container_entity
    child_entity_on_inner_instance = nested_prefabs_root_container_entity_on_instance
    for current_level in range(0, NUM_NESTED_PREFABS_LEVELS):
        assert child_entity_on_inner_instance.get_name() == prefab_test_utils.get_linear_nested_items_name(NESTED_PREFABS_NAME_PREFIX, current_level), \
            f"The name of a prefab inside the nested prefabs should be " \
            f"'{prefab_test_utils.get_linear_nested_items_name(NESTED_PREFABS_NAME_PREFIX, current_level)}', " \
            f"not '{child_entity_on_inner_instance.get_name()}'"
        assert child_entity_on_inner_instance.get_parent_id() == parent_prefab_container_entity_on_instance.id, \
            f"Prefab '{child_entity_on_inner_instance.get_name()}' should be the direct inner prefab of " \
            f"prefab with id '{parent_prefab_container_entity_on_instance.id.ToString()}', " \
            f"not '{child_entity_on_inner_instance.get_parent_id()}'"
        parent_prefab_container_entity_on_instance = child_entity_on_inner_instance
        child_entity_on_inner_instance = child_entity_on_inner_instance.get_children()[0]
    
    assert child_entity_on_inner_instance.id.IsValid(), \
        f"Entity '{child_entity_on_inner_instance.get_name()}' is not valid"
    assert child_entity_on_inner_instance.get_name() == NESTED_PREFABS_TEST_ENTITY_NAME, \
        f"The name of the entity inside the nested prefabs should be '{NESTED_PREFABS_TEST_ENTITY_NAME}', " \
        f"not '{child_entity_on_inner_instance.get_name()}'"
    assert child_entity_on_inner_instance.has_component(PHYSX_PRIMITIVE_COLLIDER_NAME), \
        "Entity inside inner_prefab doesn't have the collider component it should"
    assert child_entity_on_inner_instance.get_parent_id() == parent_prefab_container_entity_on_instance.id, \
        f"Entity '{child_entity_on_inner_instance.get_name()}' should be under " \
        f"prefab with id '{parent_prefab_container_entity_on_instance.id.ToString()}', " \
        f"not '{child_entity_on_inner_instance.get_parent_id()}'"

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(CreatePrefab_WithNestedEntitiesAndNestedPrefabs)
