"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def DeleteEntity_UnderAnotherPrefab():
    """
    Test description:
    - Creates an entity
    - Destroys the created entity
    Checks that the entity is correctly destroyed.
    """

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab

    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    prefab_test_utils.open_base_tests_level()

    CAR_PREFAB_FILE_NAME = 'car_prefab'

    # Creates a new entity at the root level
    car_entity = EditorEntity.create_editor_entity()
    assert car_entity.id.IsValid(), "Couldn't create entity"

    # Asserts if prefab creation doesn't succeed
    child_prefab, child_instance = Prefab.create_prefab([car_entity], CAR_PREFAB_FILE_NAME)
    child_entity_ids_inside_prefab = child_instance.get_direct_child_entities()
    assert len(
        child_entity_ids_inside_prefab) == 1, f"{len(child_entity_ids_inside_prefab)} entities found inside prefab" \
                                              f" when there should have been just 1 entity"

    child_entity_inside_prefab = child_entity_ids_inside_prefab[0]
    Prefab.focus_on_owning_prefab(child_entity_inside_prefab)

    child_entity_inside_prefab.delete()

    child_entity_ids_inside_prefab = child_instance.get_direct_child_entities()
    assert len(
        child_entity_ids_inside_prefab) == 0, f"{len(child_entity_ids_inside_prefab)} entities found inside prefab" \
                                              f" when there should have been 0 entities"

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(DeleteEntity_UnderAnotherPrefab)
