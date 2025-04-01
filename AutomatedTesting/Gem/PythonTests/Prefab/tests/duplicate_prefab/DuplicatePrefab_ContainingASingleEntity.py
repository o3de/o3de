"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def DuplicatePrefab_ContainingASingleEntity():

    from pathlib import Path

    import azlmbr.entity as entity
    import azlmbr.bus as bus
    import azlmbr.legacy.general as general

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    from editor_python_test_tools.wait_utils import PrefabWaiter
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    CAR_PREFAB_FILE_NAME = Path(__file__).stem + 'car_prefab'

    prefab_test_utils.open_base_tests_level()

    # Creates a new entity at the root level
    car_entity = EditorEntity.create_editor_entity("Car")
    car_prefab_entities = [car_entity]

    # Creates a prefab from the new entity
    _, car = Prefab.create_prefab(
        car_prefab_entities, CAR_PREFAB_FILE_NAME)

    # Duplicates the prefab instance
    Prefab.duplicate_prefabs([car])

    # Test undo/redo on prefab duplication
    general.undo()
    PrefabWaiter.wait_for_propagation()
    search_filter = entity.SearchFilter()
    search_filter.names = [CAR_PREFAB_FILE_NAME]
    prefab_entities_found = len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter))
    assert prefab_entities_found == 1, "Undo failed: Found duplicated prefab entities"
    search_filter.names = ["Car"]
    child_entities_found = len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter))
    assert child_entities_found == 1, "Undo failed: Found duplicated child entities"

    general.redo()
    PrefabWaiter.wait_for_propagation()
    search_filter.names = [CAR_PREFAB_FILE_NAME]
    prefab_entities_found = len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter))
    assert prefab_entities_found == 2, "Redo failed: Failed to find duplicated prefab entities"
    search_filter.names = ["Car"]
    child_entities_found = len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter))
    assert child_entities_found == 2, "Redo failed: Failed to find duplicated child entities"


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(DuplicatePrefab_ContainingASingleEntity)
