"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def DeleteEntity_UnderLevelPrefab():
    """
    Test description:
    - Creates an entity.
    - Destroys the created entity.
    Checks that the entity is correctly destroyed.
    """

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import wait_for_propagation
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    prefab_test_utils.open_base_tests_level()

    # Creates a new Entity at the root level
    # Asserts if creation didn't succeed
    entity = EditorEntity.create_editor_entity_at((100.0, 100.0, 100.0), name = "TestEntity")
    assert entity.id.IsValid(), "Couldn't create entity"

    level_container_entity = EditorEntity(entity.get_parent_id())
    entity.delete()

    # Wait till prefab propagation finishes before validating entity deletion.
    wait_for_propagation()
    level_container_child_entities_count = len(level_container_entity.get_children_ids())
    assert level_container_child_entities_count == 0, f"The level still has {level_container_child_entities_count}" \
                                                      f" children when it should have 0."

    # Test undo/redo on entity delete
    azlmbr.legacy.general.undo()
    wait_for_propagation()
    level_container_child_entities_count = len(level_container_entity.get_children_ids())
    assert level_container_child_entities_count == 1, f"{level_container_child_entities_count} entities " \
                                                      f"found in level after Undo operation, when there should " \
                                                      f"be 1 entity"
    azlmbr.legacy.general.redo()
    wait_for_propagation()
    level_container_child_entities_count = len(level_container_entity.get_children_ids())
    assert level_container_child_entities_count == 0, f"{level_container_child_entities_count} entities " \
                                                      f"found in level after Redo operation, when there should " \
                                                      f"be 0 entities"


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(DeleteEntity_UnderLevelPrefab)
