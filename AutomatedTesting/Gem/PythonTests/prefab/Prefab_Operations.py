"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from collections import Counter
from collections import deque

from PySide2 import QtWidgets

from azlmbr.entity import EntityId
from azlmbr.math import Vector3
from editor_python_test_tools.utils import Report

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.prefab as prefab
import editor_python_test_tools.pyside_utils as pyside_utils
import prefab.Prefab_Test_Utils as prefab_test_utils


class OperationResults:
    create_entity_succeeded = (
        "'CreateNewEntity' passed",
        "'CreateNewEntity' failed")

    create_prefab_succeeded = (
        "'CreatePrefab' passed",
        "'CreatePrefab' failed")

    instantiate_prefab_succeeded = (
        "'InstantiatePrefab' passed",
        "'InstantiatePrefab' failed")

    entity_has_expected_children_count = (
        "target entity owns exactly given count of children",
        "target entity does *not* owns given count of children")

    delete_prefab_succeeded = (
        "'DeleteEntitiesAndAllDescendantsInInstance' passed",
        "'DeleteEntitiesAndAllDescendantsInInstance' failed")

    delete_prefab_entities_and_descendants_succeeded = (
        "Entities and all the descendants in target prefab are deleted",
        "Entities and all the descendants in target prefab are *not* deleted")

    reparent_prefab_no_cyclical_dependency_detected = (
        "Cyclical dependency not detected while reparenting prefab",
        "Cyclical dependency detected while reparenting prefab")

    reparent_prefab_new_child_with_reparented_prefab_name_added = (
        "An entity with reparented prefab name becomes a child of target parent entity",
        "No entity with reparented prefab name become a child of target parent entity")

    reparent_prefab_reparented_prefab_under_parent = (
        "Prefab reparented is under the expected parent entity",
        "Prefab reparented is *not* under the expected parent entity")


def create_entity(entity_name=None, parent_entity_id=EntityId()):
    new_entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', parent_entity_id)
    Report.result(OperationResults.create_entity_succeeded, new_entity_id.IsValid())
    
    if new_entity_id.IsValid():
        if entity_name is not None:
            editor.EditorEntityAPIBus(bus.Event, 'SetName', new_entity_id, entity_name)
        return new_entity_id
    else:
        return EntityId()


def create_prefab(entity_ids, file_name, prefab_name=None):
    file_path = prefab_test_utils.get_prefab_file_path(file_name)
    create_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'CreatePrefabInMemory', entity_ids, file_path)
    Report.result(OperationResults.create_entity_succeeded, create_prefab_result.IsSuccess())
    
    if not create_prefab_result.IsSuccess():
        return EntityId()

    container_entity_id = create_prefab_result.GetValue()

    if prefab_name is not None:
        editor.EditorEntityAPIBus(bus.Event, 'SetName', container_entity_id, prefab_name)
    else:
        prefab_name = file_name

    prefab_test_utils.wait_for_propagation()
    container_entity_id = prefab_test_utils.find_entity_by_unique_name(prefab_name)
    return container_entity_id


def instantiate_prefab(file_name, prefab_name=None, parent_entity_id=EntityId(), expected_prefab_position=Vector3()):
    file_path = prefab_test_utils.get_prefab_file_path(file_name)
    instantiate_prefab_result = prefab.PrefabPublicRequestBus(
        bus.Broadcast, 'InstantiatePrefab', file_path, parent_entity_id, expected_prefab_position)
    Report.result(OperationResults.instantiate_prefab_succeeded, instantiate_prefab_result.IsSuccess())

    if not instantiate_prefab_result.IsSuccess():
        return EntityId()

    container_entity_id = instantiate_prefab_result.GetValue()

    if prefab_name is not None:
        editor.EditorEntityAPIBus(bus.Event, 'SetName', container_entity_id, prefab_name)
    else:
        prefab_name = file_name

    prefab_test_utils.wait_for_propagation()
    container_entity_id = prefab_test_utils.find_entity_by_unique_name(prefab_name)
    prefab_test_utils.check_entity_at_position(container_entity_id, expected_prefab_position)

    return container_entity_id


def delete_prefabs(container_entity_ids):
    instances_to_remove_name_counts = Counter()
    instances_removed_expected_name_counts = Counter()

    entity_ids_to_remove = list(container_entity_ids)
    while entity_ids_to_remove:
        entity_id = entity_ids_to_remove.pop(-1)
        entity_name = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', entity_id)
        instances_to_remove_name_counts[entity_name] += 1

        children_entity_ids = editor.EditorEntityInfoRequestBus(bus.Event, 'GetChildren', entity_id)
        for child_entity_id in children_entity_ids:
            entity_ids_to_remove.append(child_entity_id)

    for entity_name, entity_count in instances_to_remove_name_counts.items():
        entities = prefab_test_utils.find_entities_by_name(entity_name)
        instances_removed_expected_name_counts[entity_name] = len(entities) - entity_count

    delete_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'DeleteEntitiesAndAllDescendantsInInstance', container_entity_ids)
    Report.result(OperationResults.delete_prefab_succeeded, delete_prefab_result.IsSuccess())

    if not delete_prefab_result.IsSuccess():
        return

    prefab_test_utils.wait_for_propagation()

    prefab_entities_deleted = True
    for entity_name, expected_entity_count in instances_removed_expected_name_counts.items():
        actual_entity_count = len(prefab_test_utils.find_entities_by_name(entity_name))
        if actual_entity_count is not expected_entity_count:
            prefab_entities_deleted = False
            break

    Report.result(OperationResults.delete_prefab_entities_and_descendants_succeeded, prefab_entities_deleted)


async def reparent_prefab(container_entity_id, parent_entity_id):
    prefab_name = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', container_entity_id)
    current_children_entity_ids_having_prefab_name = prefab_test_utils.get_children_by_name(parent_entity_id, prefab_name)

    pyside_utils.run_soon(lambda: editor.EditorEntityAPIBus(bus.Event, 'SetParent', container_entity_id, parent_entity_id))
    pyside_utils.run_soon(lambda: prefab_test_utils.wait_for_propagation())

    reparent_prefab_cyclical_dependency_detected = False
    try:
        active_modal_widget = await pyside_utils.wait_for_modal_widget()
        error_message_box = active_modal_widget.findChild(QtWidgets.QMessageBox)
        ok_button = error_message_box.button(QtWidgets.QMessageBox.Ok)
        ok_button.click()
        reparent_prefab_cyclical_dependency_detected = True
    except pyside_utils.EventLoopTimeoutException:
        pass

    Report.result(OperationResults.reparent_prefab_no_cyclical_dependency_detected, not reparent_prefab_cyclical_dependency_detected)

    if reparent_prefab_cyclical_dependency_detected:
        return EntityId()

    updated_children_entity_ids_having_prefab_name = prefab_test_utils.get_children_by_name(parent_entity_id, prefab_name)
    new_child_with_reparented_prefab_name_added = len(updated_children_entity_ids_having_prefab_name) is len(current_children_entity_ids_having_prefab_name) + 1
    Report.result(OperationResults.reparent_prefab_new_child_with_reparented_prefab_name_added, new_child_with_reparented_prefab_name_added)

    if not new_child_with_reparented_prefab_name_added:
        return EntityId()
        
    updated_container_entity_id = set(updated_children_entity_ids_having_prefab_name).difference(current_children_entity_ids_having_prefab_name).pop()
    updated_container_entity_parent_id = editor.EditorEntityInfoRequestBus(bus.Event, 'GetParent', updated_container_entity_id)
    has_correct_parent = updated_container_entity_parent_id.ToString() == parent_entity_id.ToString()
    Report.result(OperationResults.reparent_prefab_reparented_prefab_under_parent, has_correct_parent)

    if not has_correct_parent:
        return EntityId()

    return updated_container_entity_id
