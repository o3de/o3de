"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def InstantiatePrefab_FromCreatedPrefabWithSingleEntity():

    from pathlib import Path

    import azlmbr.legacy.general as general

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    from editor_python_test_tools.wait_utils import PrefabWaiter
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    CAR_PREFAB_FILE_NAME = Path(__file__).stem + 'car_prefab'

    prefab_test_utils.open_base_tests_level()

    # Creates a new entity at the root level
    car_entity = EditorEntity.create_editor_entity()
    car_prefab_entities = [car_entity]

    # Creates a prefab from the new entity
    car_prefab, car_prefab_instance = Prefab.create_prefab(car_prefab_entities, CAR_PREFAB_FILE_NAME)

    # Instantiate another instance and verify
    car_prefab_instance_2 = car_prefab.instantiate()
    assert car_prefab_instance_2.is_valid(), "Failed to instantiate prefab"

    # Get parent entity and container id for verifying successful Undo/Redo operations
    instance_parent_id = EditorEntity(car_prefab_instance_2.container_entity.get_parent_id())
    instance = car_prefab_instance_2.container_entity

    # Undo the instantiation
    general.undo()
    PrefabWaiter.wait_for_propagation()
    child_ids = instance_parent_id.get_children_ids()
    assert instance.id not in child_ids, "Undo Failed: Instance was still found after undo."

    # Redo the instantiation
    general.redo()
    PrefabWaiter.wait_for_propagation()
    child_ids = instance_parent_id.get_children_ids()
    instance_children = instance.get_children()
    assert instance.id in child_ids, "Redo Failed: Instance was not found after redo"
    assert len(instance_children) == 1, \
        "Redo Failed: Did not find expected child entities in the prefab instance"


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(InstantiatePrefab_FromCreatedPrefabWithSingleEntity)
