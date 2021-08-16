"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt:off
class Tests():
    create_new_entity =           ("Entity: 'CreateNewEntity' passed",                                "Entity: 'CreateNewEntity' failed")
    create_prefab =               ("Prefab: 'CreatePrefab' passed",                                   "Prefab: 'CreatePrefab' failed")
    instantiate_prefab =          ("Prefab: 'InstantiatePrefab' passed",                              "Prefab: 'InstantiatePrefab' failed")
    new_prefab_position =         ("Prefab: new prefab's position is at the expected position",       "Prefab: new prefab's position is *not* at the expected position")
    delete_prefab =               ("Prefab: 'DeleteEntitiesAndAllDescendantsInInstance' passed",      "Prefab: 'DeleteEntitiesAndAllDescendantsInInstance' failed")
    instantiated_prefab_removed = ("Prefab: instantiated prefab's container entity has been removed", "Prefab: instantiated prefab's container entity has *not* been removed")
# fmt:on

def PrefabLevel_BasicWorkflow():
    """
    This test will help verify if the following functions related to Prefab work as expected: 
    - CreatePrefab
    - InstantiatePrefab
    - DeleteEntitiesAndAllDescendantsInInstance
    """

    import os
    import sys

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import editor_python_test_tools.hydra_editor_utils as hydra

    import azlmbr.bus as bus
    import azlmbr.entity as entity
    from azlmbr.entity import EntityId
    import azlmbr.editor as editor
    import azlmbr.prefab as prefab
    from azlmbr.math import Vector3
    import azlmbr.legacy.general as general

    EXPECTED_NEW_PREFAB_POSITION = Vector3(10.00, 20.0, 30.0)
    NEW_PREFAB_NAME = "new_prefab"
    NEW_PREFAB_FILE_NAME = NEW_PREFAB_NAME + ".prefab"
    NEW_PREFAB_FILE_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), NEW_PREFAB_FILE_NAME)
    INSTANTIATED_PREFAB_NAME = "instantiated_prefab"
    TEST_LEVEL_FOLDER = "Prefab"
    TEST_LEVEL_NAME = "Base"

    def find_entity_by_name(entity_name):
        searchFilter = entity.SearchFilter()
        searchFilter.names = [entity_name]
        entityIds = entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)
        if entityIds and entityIds[0].IsValid():
            return entityIds[0]
        return None

    def print_error_if_failed(prefab_operation_result):
        if not prefab_operation_result.IsSuccess():
            Report.info(f'Error message: {prefab_operation_result.GetError()}')


# Open the test level
    helper.init_idle()
    helper.open_level(TEST_LEVEL_FOLDER, TEST_LEVEL_NAME)

# Create a new Entity at the root level
    new_entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
    Report.result(Tests.create_new_entity, new_entity_id.IsValid())

# Checks for prefab creation passed or not 
    create_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'CreatePrefabInMemory', [new_entity_id], NEW_PREFAB_FILE_PATH)
    Report.result(Tests.create_prefab, create_prefab_result.IsSuccess())
    print_error_if_failed(create_prefab_result)

# Checks for prefab instantiation passed or not 
    instantiate_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'InstantiatePrefab', NEW_PREFAB_FILE_PATH, EntityId(), EXPECTED_NEW_PREFAB_POSITION)
    Report.result(Tests.instantiate_prefab, instantiate_prefab_result.IsSuccess() and instantiate_prefab_result.GetValue().IsValid())
    print_error_if_failed(instantiate_prefab_result)
    container_entity_id = instantiate_prefab_result.GetValue()
    editor.EditorEntityAPIBus(bus.Event, 'SetName', container_entity_id, INSTANTIATED_PREFAB_NAME)

# Checks if the new prefab is at the correct position and if it fails, it will provide the expected postion and the actual postion of the entity in the Editor log
    new_prefab_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", container_entity_id)
    is_at_position = new_prefab_position.IsClose(EXPECTED_NEW_PREFAB_POSITION)
    Report.result(Tests.new_prefab_position, is_at_position)
    if not is_at_position:
        Report.info(f'Expected position: {EXPECTED_NEW_PREFAB_POSITION.ToString()}, actual position: {new_prefab_position.ToString()}')

# Checks for prefab deletion passed or not 
    delete_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'DeleteEntitiesAndAllDescendantsInInstance', [container_entity_id])
    Report.result(Tests.delete_prefab, delete_prefab_result.IsSuccess())
    print_error_if_failed(delete_prefab_result)
    Report.result(Tests.instantiated_prefab_removed, find_entity_by_name(INSTANTIATED_PREFAB_NAME) is None)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(PrefabLevel_BasicWorkflow)
