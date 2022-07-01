"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def CreatePrefab_WithSingleEntity():

    from pathlib import Path

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    CAR_PREFAB_FILE_NAME = Path(__file__).stem + 'car_prefab'

    prefab_test_utils.open_base_tests_level()

    # Creates a new entity at the root level
    car_entity = EditorEntity.create_editor_entity()
    car_entity_parent = car_entity.get_parent_id()
    car_prefab_entities = [car_entity]

    # Creates a prefab from the new entity
    _, car_instance = Prefab.create_prefab(car_prefab_entities, CAR_PREFAB_FILE_NAME)

    # Test undo/redo on prefab creation
    prefab_test_utils.validate_undo_redo_on_prefab_creation(car_instance, car_entity_parent)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(CreatePrefab_WithSingleEntity)
