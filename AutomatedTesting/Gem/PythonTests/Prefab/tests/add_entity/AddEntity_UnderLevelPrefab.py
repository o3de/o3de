"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def AddEntity_UnderLevelPrefab():
    """
    Test description:
    - Creates an entity under level container.
    - Verifies Undo/Redo.
    """

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.wait_utils import PrefabWaiter
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    prefab_test_utils.open_base_tests_level()

    # Creates a new Entity at the root level
    # Asserts if creation didn't succeed
    entity = EditorEntity.create_editor_entity_at((100.0, 100.0, 100.0), name = "TestEntity")
    assert entity.id.IsValid(), "Couldn't create entity"

    level_container_entity = EditorEntity(entity.get_parent_id())

    # Test undo/redo on add entity
    azlmbr.legacy.general.undo()
    azlmbr.legacy.general.undo()
    PrefabWaiter.wait_for_propagation()
    level_container_child_entities_count = len(level_container_entity.get_children_ids())
    assert level_container_child_entities_count == 0, f"{level_container_child_entities_count} entities " \
                                                      f"found in level after Undo operation, when there should " \
                                                      f"be 0 entities"

    azlmbr.legacy.general.redo()
    PrefabWaiter.wait_for_propagation()
    level_container_child_entities_count = len(level_container_entity.get_children_ids())
    assert level_container_child_entities_count == 1, f"{level_container_child_entities_count} entities " \
                                                      f"found in level after Redo operation, when there should " \
                                                      f"be 1 entity"


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AddEntity_UnderLevelPrefab)
