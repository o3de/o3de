"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def AddEntity_UnderChildEntityOfPrefab():
    """
    Test description:
    - Creates an entity.
    - Creates a prefab out of the above entity.
    - Focuses on the created prefab and adds a new child entity.
    - Creates a child entity under the above child entity of the created prefab.
    - Checks that the entity is correctly added.
    """

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    from editor_python_test_tools.wait_utils import PrefabWaiter

    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    prefab_test_utils.open_base_tests_level()

    from pathlib import Path
    PREFAB_FILE_NAME = Path(__file__).stem + '_' + 'some_prefab'

    # Creates a new entity at the root level
    entity = EditorEntity.create_editor_entity()
    assert entity.id.IsValid(), "Couldn't create entity."

    # Creates a new Prefab, gets focus, and adds a child entity
    # Asserts if prefab creation doesn't succeed
    child_prefab, child_instance = Prefab.create_prefab([entity], PREFAB_FILE_NAME)
    child_entity_ids_inside_prefab_instance = child_instance.get_direct_child_entities()
    assert len(child_entity_ids_inside_prefab_instance) == 1, f"{len(child_entity_ids_inside_prefab_instance)} entities found inside prefab" \
                                              f" when there should have been just 1 entity"

    child_entity_inside_prefab = child_entity_ids_inside_prefab_instance[0]
    child_entity_inside_prefab.focus_on_owning_prefab()

    second_child_entity = child_entity_inside_prefab.create_editor_entity(parent_id=child_entity_inside_prefab.id)

    # Wait till prefab propagation finishes before validating add second child entity
    PrefabWaiter.wait_for_propagation()
    assert second_child_entity.id.IsValid(), "Couldn't add second child entity"

    # Test undo/redo on add second child entity
    azlmbr.legacy.general.undo()
    PrefabWaiter.wait_for_propagation()
    child_entity_ids_inside_prefab = child_entity_inside_prefab.get_children()
    assert len(child_entity_ids_inside_prefab) == 0, f"{len(child_entity_ids_inside_prefab)} child entities found inside prefab" \
                                              f" after Undo operation, when there should have been 0 child entities"
    azlmbr.legacy.general.redo()
    PrefabWaiter.wait_for_propagation()
    child_entity_ids_inside_prefab = child_entity_inside_prefab.get_children()
    assert len(child_entity_ids_inside_prefab) == 1, f"{len(child_entity_ids_inside_prefab)} child entities found inside prefab" \
                                              f" after Redo operation, when there should have been 1 child entity"


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AddEntity_UnderChildEntityOfPrefab)
