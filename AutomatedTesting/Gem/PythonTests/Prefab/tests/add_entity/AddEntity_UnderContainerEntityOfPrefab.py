"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def AddEntity_UnderContainerEntityOfPrefab():
    """
    Test description:
    - Creates an entity.
    - Creates a prefab out of the above entity.
    - Focuses on the created prefab.
    - Creates a second entity under the created prefab.
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
    assert len(
        child_entity_ids_inside_prefab_instance) == 1, f"{len(child_entity_ids_inside_prefab_instance)} entities found inside prefab" \
                                                       f" when there should have been just 1 entity"

    child_entity_inside_prefab = child_entity_ids_inside_prefab_instance[0]
    child_entity_inside_prefab.focus_on_owning_prefab()

    second_entity = EditorEntity.create_editor_entity(parent_id=child_instance.container_entity.id)

    # Wait till prefab propagation finishes before validating add second child entity
    PrefabWaiter.wait_for_propagation()
    assert second_entity.id.IsValid(), "Couldn't add second child entity"

    # Test undo/redo on add second entity
    azlmbr.legacy.general.undo()
    PrefabWaiter.wait_for_propagation()
    child_entity_ids_inside_prefab = child_instance.container_entity.get_children()
    assert len(child_entity_ids_inside_prefab) == 1, f"{len(child_entity_ids_inside_prefab)} entities found inside prefab" \
                                              f" after Undo operation, when there should have been 1 entity"
    azlmbr.legacy.general.redo()
    PrefabWaiter.wait_for_propagation()
    child_entity_ids_inside_prefab = child_instance.container_entity.get_children()
    assert len(child_entity_ids_inside_prefab) == 2, f"{len(child_entity_ids_inside_prefab)} entities found inside prefab" \
                                              f" after Redo operation, when there should have been 2 entities"


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AddEntity_UnderContainerEntityOfPrefab)
