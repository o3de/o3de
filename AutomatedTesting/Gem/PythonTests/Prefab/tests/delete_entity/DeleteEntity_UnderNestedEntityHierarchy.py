"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def DeleteEntity_UnderNestedEntityHierarchy():
    """
    Test description:
    - Creates a nested entity hierarchy
    - Deletes an entity from the hierarchy

    Test passes if the entity is successfully deleted, and Undo/Redo restores/re-deletes the entity.
    """

    import azlmbr.bus as bus
    import azlmbr.entity as entity
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from EditorPythonTestTools.editor_python_test_tools.editor_entity_utils import EditorEntity
    from EditorPythonTestTools.editor_python_test_tools.wait_utils import PrefabWaiter
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    ENTITY_POS = math.Vector3(0.0, 0.0, 0.0)
    NUM_NESTED_ENTITY_LEVELS = 10
    NESTED_ENTITY_PREFIX = "TestEntity_"

    def validate_nested_entity_deletion(num_expected_deleted_entities, first_deleted_entity_suffix,
                                        last_deleted_entity_suffix):
        for i in range(first_deleted_entity_suffix, last_deleted_entity_suffix):
            search_filter = entity.SearchFilter()
            search_filter.names = [f"{NESTED_ENTITY_PREFIX}{i}"]
            nested_entities = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
            assert len(nested_entities) == 0, f"Unexpectedly found {NESTED_ENTITY_PREFIX}{i} when it should be deleted"
        prefab_test_utils.validate_linear_nested_entities(nested_entities_root,
                                                          NUM_NESTED_ENTITY_LEVELS - num_expected_deleted_entities,
                                                          ENTITY_POS)

    prefab_test_utils.open_base_tests_level()

    # Creates new nested entities at the root level
    # Asserts if creation didn't succeed
    nested_entities_root = prefab_test_utils.create_linear_nested_entities(NESTED_ENTITY_PREFIX,
                                                                           NUM_NESTED_ENTITY_LEVELS, ENTITY_POS)
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root, NUM_NESTED_ENTITY_LEVELS, ENTITY_POS)

    # Find an entity in the hierarchy, delete it, and validate Undo/Redo functionality
    search_filter = entity.SearchFilter()
    search_filter.names = [f"{NESTED_ENTITY_PREFIX}5"]
    entity_to_delete = EditorEntity(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)[0])
    entity_to_delete.delete()

    # TestEntity_5 -> TestEntity_9 should all be removed at this point
    validate_nested_entity_deletion(5, 5, 9)

    # Undo, and validate the nested entity hierarchy is restored
    general.undo()
    PrefabWaiter.wait_for_propagation()
    prefab_test_utils.validate_linear_nested_entities(nested_entities_root, NUM_NESTED_ENTITY_LEVELS, ENTITY_POS)

    # Redo, and validate that the appropriate nested entities are once again deleted
    general.redo()
    PrefabWaiter.wait_for_propagation()
    validate_nested_entity_deletion(5, 5, 9)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(DeleteEntity_UnderNestedEntityHierarchy)
