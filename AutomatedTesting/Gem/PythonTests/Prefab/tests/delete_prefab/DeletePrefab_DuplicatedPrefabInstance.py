"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def DeletePrefab_DuplicatedPrefabInstance():

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
    _, car = Prefab.create_prefab(
        car_prefab_entities, CAR_PREFAB_FILE_NAME)

    # Duplicate the created instance
    duplicate_car = Prefab.duplicate_prefabs([car])[0]

    # Get parent entity and container id for verifying successful Undo/Redo operations
    duplicate_car_parent_id = EditorEntity(duplicate_car.container_entity.get_parent_id())
    duplicate_car_id = duplicate_car.container_entity.id

    # Deletes the prefab instance
    Prefab.remove_prefabs([duplicate_car])

    # Undo the prefab delete
    general.undo()
    PrefabWaiter.wait_for_propagation()
    child_ids = duplicate_car_parent_id.get_children_ids()
    assert duplicate_car_id in child_ids and len(child_ids) == 2, \
        "Undo Failed: Failed to find restored prefab instance after Undo."

    # Redo the prefab delete
    general.redo()
    PrefabWaiter.wait_for_propagation()
    child_ids = duplicate_car_parent_id.get_children_ids()
    assert duplicate_car_id not in child_ids and len(child_ids) == 1, \
        "Redo Failed: Instance was still found after redo of instance deletion."


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(DeletePrefab_DuplicatedPrefabInstance)
