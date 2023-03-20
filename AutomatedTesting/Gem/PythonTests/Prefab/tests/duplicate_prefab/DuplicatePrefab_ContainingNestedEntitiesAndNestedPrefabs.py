"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def DuplicatePrefab_ContainingNestedEntitiesAndNestedPrefabs():
    """
    Test description:
    - Creates linear nested entities.
    - Creates linear nested prefabs based of an entity with a physx collider.
    - Creates a prefab from the nested entities and the nested prefabs.
    - Duplicates the prefab.
    - Checks that the prefab is correctly duplicated.
    - Checks Undo/Redo operations.
    """

    from pathlib import Path

    import azlmbr.legacy.general as general

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    from editor_python_test_tools.wait_utils import PrefabWaiter
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
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root, NUM_NESTED_ENTITIES_LEVELS,
                                                      CREATION_POSITION)
    nested_entities_root_name = nested_entities_root.get_name()

    # Creates new nested prefabs at the root level
    # Asserts if creation didn't succeed
    entity_to_nest = EditorEntity.create_editor_entity_at(CREATION_POSITION, name=NESTED_PREFABS_TEST_ENTITY_NAME)
    assert entity_to_nest.id.IsValid(), "Couldn't create TestEntity"
    entity_to_nest.add_component(PHYSX_PRIMITIVE_COLLIDER_NAME)
    assert entity_to_nest.has_component(PHYSX_PRIMITIVE_COLLIDER_NAME), f"Failed to add a {PHYSX_PRIMITIVE_COLLIDER_NAME}"
    _, nested_prefab_instances = prefab_test_utils.create_linear_nested_prefabs(
        [entity_to_nest], NESTED_PREFABS_FILE_NAME_PREFIX, NESTED_PREFABS_NAME_PREFIX, NUM_NESTED_PREFABS_LEVELS)
    prefab_test_utils.validate_linear_nested_prefab_instances_hierarchy(nested_prefab_instances)

    # Creates a new prefab containing the nested entities and the nested prefab instances
    # Asserts if prefab creation doesn't succeed
    _, new_prefab = Prefab.create_prefab(
        [nested_entities_root, nested_prefab_instances[0].container_entity],
        FILE_NAME_OF_PREFAB_WITH_NESTED_ENTITIES_AND_NESTED_PREFABS)
    new_prefab_container_entity = new_prefab.container_entity

    nested_entities_root_on_instance = new_prefab.get_direct_child_entity_by_name(nested_entities_root_name)

    assert nested_entities_root_on_instance.get_name() == nested_entities_root_name \
           and nested_entities_root_on_instance.get_parent_id() == new_prefab_container_entity.id, \
        f"The name of the first child entity of the new prefab '{new_prefab_container_entity.get_name()}' " \
        f"should be '{nested_entities_root_name}', " \
        f"not '{nested_entities_root_on_instance.get_name()}'"
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root_on_instance, NUM_NESTED_ENTITIES_LEVELS,
                                                      CREATION_POSITION)

    # Gather information on prefab structure to validate against Undo/Redo
    common_parent = EditorEntity(new_prefab.container_entity.get_parent_id())
    common_parent_children_ids_before_duplicate = set([child_id.ToString() for child_id in
                                                       common_parent.get_children_ids()])

    # Duplicates the prefab instance and asserts if duplication doesn't succeed
    duplicated_instance = Prefab.duplicate_prefabs([new_prefab])

    # Gather more information on prefab structure after duplication to validate against Undo/Redo
    common_parent_children_ids_after_duplicate = set([child_id.ToString() for child_id in
                                                      common_parent.get_children_ids()])
    duplicate_container_entity_ids = [duplicated_instance[0].container_entity.id]

    # Test undo/redo on prefab duplication
    general.undo()
    PrefabWaiter.wait_for_propagation()
    common_parent_children_ids_after_duplicate_undo = set([child_id.ToString() for child_id in
                                                           common_parent.get_children_ids()])
    assert common_parent_children_ids_before_duplicate == common_parent_children_ids_after_duplicate_undo, \
        "Undo Failed: Found unexpected children of common parent after Undo"

    general.redo()
    PrefabWaiter.wait_for_propagation()
    Prefab.validate_duplicated_prefab([new_prefab], common_parent_children_ids_before_duplicate,
                                      common_parent_children_ids_after_duplicate, duplicate_container_entity_ids,
                                      common_parent)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(DuplicatePrefab_ContainingNestedEntitiesAndNestedPrefabs)
