"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt:off
class Tests():
    create_new_entity =            ("'CreateNewEntity' passed",                                   "'CreateNewEntity' failed")
    create_prefab =                ("'CreatePrefab' passed",                                      "'CreatePrefab' failed")
    instantiate_prefab =           ("'InstantiatePrefab' passed",                                 "'InstantiatePrefab' failed")
    has_one_child =                ("instantiated prefab contains only one child as expected",    "instantiated prefab does *not* contain only one child as expected")
    instantiated_prefab_position = ("instantiated prefab's position is at the expected position", "instantiated prefab's position is *not* at the expected position")
    delete_prefab =                ("'DeleteEntitiesAndAllDescendantsInInstance' passed",         "'DeleteEntitiesAndAllDescendantsInInstance' failed")
    instantiated_prefab_removed =  ("instantiated prefab's container entity has been removed",    "instantiated prefab's container entity has *not* been removed")
    instantiated_child_removed =   ("instantiated prefab's child entity has been removed",        "instantiated prefab's child entity has *not* been removed")
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

    NEW_PREFAB_NAME = "new_prefab"
    NEW_PREFAB_FILE_NAME = NEW_PREFAB_NAME + ".prefab"
    NEW_PREFAB_FILE_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), NEW_PREFAB_FILE_NAME)
    INSTANTIATED_PREFAB_POSITION = Vector3(10.00, 20.0, 30.0)
    INSTANTIATED_PREFAB_NAME = "instantiated_prefab"
    INSTANTIATED_CHILD_ENTITY_NAME = "child_1"
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
    instantiate_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'InstantiatePrefab', NEW_PREFAB_FILE_PATH, EntityId(), INSTANTIATED_PREFAB_POSITION)
    Report.result(Tests.instantiate_prefab, instantiate_prefab_result.IsSuccess() and instantiate_prefab_result.GetValue().IsValid())
    print_error_if_failed(instantiate_prefab_result)

    container_entity_id = instantiate_prefab_result.GetValue()
    editor.EditorEntityAPIBus(bus.Event, 'SetName', container_entity_id, INSTANTIATED_PREFAB_NAME)
    
    children_entity_ids = editor.EditorEntityInfoRequestBus(bus.Event, 'GetChildren', container_entity_id)
    Report.result(Tests.has_one_child, len(children_entity_ids) is 1)

    child_entity_id = children_entity_ids[0]
    editor.EditorEntityAPIBus(bus.Event, 'SetName', child_entity_id, INSTANTIATED_CHILD_ENTITY_NAME)

# Checks if the new prefab is at the correct position and if it fails, it will provide the expected postion and the actual postion of the entity in the Editor log
    actual_prefab_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", container_entity_id)
    is_at_position = actual_prefab_position.IsClose(INSTANTIATED_PREFAB_POSITION)
    Report.result(Tests.instantiated_prefab_position, is_at_position)
    if not is_at_position:
        Report.info(f'Expected position: {INSTANTIATED_PREFAB_POSITION.ToString()}, actual position: {actual_prefab_position.ToString()}')

# Checks for prefab deletion passed or not 
    delete_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'DeleteEntitiesAndAllDescendantsInInstance', [container_entity_id])
    Report.result(Tests.delete_prefab, delete_prefab_result.IsSuccess())
    print_error_if_failed(delete_prefab_result)
    Report.result(Tests.instantiated_prefab_removed, find_entity_by_name(INSTANTIATED_PREFAB_NAME) is None)
    Report.result(Tests.instantiated_child_removed, find_entity_by_name(INSTANTIATED_CHILD_ENTITY_NAME) is None)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(PrefabLevel_BasicWorkflow)
