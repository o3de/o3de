"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def DeletePrefab_AndCreateWithSameName():

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

    # Deletes the prefab instance. This will remove the template too since this is the last instance.
    Prefab.remove_prefabs([car])

    # Creates another new car entity
    new_car_entity = EditorEntity.create_editor_entity()
    new_car_entity_parent = new_car_entity.get_parent_id()
    new_car_prefab_entities = [new_car_entity]

    # Creates a prefab from the new entity
    _, new_car = Prefab.create_prefab(
        new_car_prefab_entities, CAR_PREFAB_FILE_NAME)

    # Test undo/redo on prefab creation
    prefab_test_utils.validate_undo_redo_on_prefab_creation(new_car, new_car_entity_parent)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(DeletePrefab_AndCreateWithSameName)
