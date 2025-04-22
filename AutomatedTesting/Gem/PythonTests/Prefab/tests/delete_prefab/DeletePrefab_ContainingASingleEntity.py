"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def DeletePrefab_ContainingASingleEntity():

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

    # Get parent entity and container id for verifying successful Undo/Redo operations
    instance_parent_id = EditorEntity(car.container_entity.get_parent_id())
    instance_id = car.container_entity.id

    # Deletes the prefab instance
    Prefab.remove_prefabs([car])

    # Undo the prefab delete
    general.undo()
    PrefabWaiter.wait_for_propagation()
    child_ids = instance_parent_id.get_children_ids()
    assert instance_id in child_ids, \
        "Undo Failed: Failed to find restored prefab instance after Undo."

    # Redo the prefab delete
    general.redo()
    PrefabWaiter.wait_for_propagation()
    child_ids = instance_parent_id.get_children_ids()
    assert instance_id not in child_ids, \
        "Redo Failed: Instance was still found after redo of instance deletion."


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(DeletePrefab_ContainingASingleEntity)
