"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def DeletePrefab_ContainingNestedEntitiesAndNestedPrefabs():
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
    import azlmbr.bus as bus
    import azlmbr.entity as entity

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab, get_all_entity_ids, wait_for_propagation
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    NESTED_ENTITIES_PREFAB_FILE_NAME = Path(__file__).stem + '_' + 'nested_entities_prefab'
    NESTED_ENTITIES_NAME_PREFIX = 'Entity_'
    NESTED_PREFABS_FILE_NAME_PREFIX = Path(__file__).stem + '_' + 'nested_prefabs_'
    NESTED_PREFABS_NAME_PREFIX = 'NestedPrefabs_Prefab_'
    FILE_NAME_OF_PREFAB_WITH_NESTED_ENTITIES_AND_NESTED_PREFABS = Path(__file__).stem + '_' + 'new_prefab'
    NESTED_PREFABS_TEST_ENTITY_NAME = 'TestEntity'
    PHYSX_COLLIDER_NAME = 'PhysX Collider'
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
    entity_to_nest.add_component(PHYSX_COLLIDER_NAME)
    assert entity_to_nest.has_component(PHYSX_COLLIDER_NAME), f"Failed to add a {PHYSX_COLLIDER_NAME}"
    _, nested_prefab_instances = prefab_test_utils.create_linear_nested_prefabs(
        [entity_to_nest], NESTED_PREFABS_FILE_NAME_PREFIX, NESTED_PREFABS_NAME_PREFIX, NUM_NESTED_PREFABS_LEVELS)
    prefab_test_utils.validate_linear_nested_prefab_instances_hierarchy(nested_prefab_instances)

    # Creates a new prefab containing the nested entities and the nested prefab instances
    # Asserts if prefab creation doesn't succeed
    _, new_prefab = Prefab.create_prefab(
        [nested_entities_root, nested_prefab_instances[0].container_entity],
        FILE_NAME_OF_PREFAB_WITH_NESTED_ENTITIES_AND_NESTED_PREFABS)
    new_prefab_container_entity = new_prefab.container_entity

    nested_entities_root_on_instance = new_prefab.get_direct_child_entities()[0]
    nested_prefabs_root_container_entity_on_instance = new_prefab.get_direct_child_entities()[1]
    if nested_entities_root_on_instance.get_name() != nested_entities_root_name:
        nested_entities_root_on_instance, nested_prefabs_root_container_entity_on_instance = \
            nested_prefabs_root_container_entity_on_instance, nested_entities_root_on_instance

    assert nested_entities_root_on_instance.get_name() == nested_entities_root_name \
           and nested_entities_root_on_instance.get_parent_id() == new_prefab_container_entity.id, \
        f"The name of the first child entity of the new prefab '{new_prefab_container_entity.get_name()}' " \
        f"should be '{nested_entities_root_name}', " \
        f"not '{nested_entities_root_on_instance.get_name()}'"
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root_on_instance, NUM_NESTED_ENTITIES_LEVELS,
                                                      CREATION_POSITION)

    # Duplicates the prefab instance and asserts if duplication doesn't succeed
    Prefab.duplicate_prefabs([new_prefab])

    # Test undo/redo on prefab duplication
    general.undo()
    wait_for_propagation()
    search_filter = entity.SearchFilter()
    search_filter.names = [FILE_NAME_OF_PREFAB_WITH_NESTED_ENTITIES_AND_NESTED_PREFABS]
    prefab_entities_found = len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter))
    assert prefab_entities_found == 1, "Undo failed: Found duplicated prefab entities"
    search_filter.names = ["Entity_2"]
    child_entities_found = len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter))
    assert child_entities_found == 1, "Undo failed: Found duplicated child entities"

    general.redo()
    wait_for_propagation()
    search_filter.names = [FILE_NAME_OF_PREFAB_WITH_NESTED_ENTITIES_AND_NESTED_PREFABS]
    prefab_entities_found = len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter))
    assert prefab_entities_found == 2, "Redo failed: Failed to find duplicated prefab entities"
    search_filter.names = ["Entity_2"]
    child_entities_found = len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter))
    assert child_entities_found == 2, "Redo failed: Failed to find duplicated child entities"


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(DeletePrefab_ContainingNestedEntitiesAndNestedPrefabs)
