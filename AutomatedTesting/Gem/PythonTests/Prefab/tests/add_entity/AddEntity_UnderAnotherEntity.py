"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def AddEntity_UnderAnotherEntity():
    """
    Test description:
    - Creates an entity at the root level
    - Creates a child entity of the root entity
    - Verifies Undo/Redo.
    """

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.wait_utils import PrefabWaiter
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    prefab_test_utils.open_base_tests_level()

    # Creates a new Entity at the root level
    # Asserts if creation didn't succeed
    parent_entity = EditorEntity.create_editor_entity_at((100.0, 100.0, 100.0))
    assert parent_entity.id.IsValid(), "Couldn't create parent entity"


    # Creates a new child Entity
    # Asserts if creation didn't succeed
    child_entity = EditorEntity.create_editor_entity(parent_id=parent_entity.id)
    assert child_entity.id.IsValid(), "Couldn't create child entity"
    assert parent_entity.get_children_ids()[0] == child_entity.id, "Couldn't create child entity of parent entity"

    PrefabWaiter.wait_for_propagation()

    # Test undo/redo on add child Entity
    azlmbr.legacy.general.undo()
    PrefabWaiter.wait_for_propagation()
    child_entities_count = len(parent_entity.get_children_ids())
    assert child_entities_count == 0, f"{child_entities_count} child entities " \
                                                      f"found in level after Undo operation, when there should " \
                                                      f"be 0 child entities"
    azlmbr.legacy.general.redo()
    PrefabWaiter.wait_for_propagation()
    child_entities_count = len(parent_entity.get_children_ids())
    assert child_entities_count == 1, f"{child_entities_count} entities " \
                                                      f"found in level after Redo operation, when there should " \
                                                      f"be 1 child entity"


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AddEntity_UnderAnotherEntity)
