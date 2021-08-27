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

import azlmbr.bus as bus
import azlmbr.components as components
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.prefab as prefab

import editor_python_test_tools.pyside_utils as pyside_utils

from prefab.Prefab_Test_Results import Results

import prefab.Prefab_Test_Utils as prefab_test_utils


def create_entity(entity_name=None, parent_entity_id=EntityId()):
    new_entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', parent_entity_id)
    if new_entity_id.IsValid():
        print(Results.create_entity_succeeded)

        if entity_name is not None:
            editor.EditorEntityAPIBus(bus.Event, 'SetName', new_entity_id, entity_name)
    else:
        print(Results.create_entity_failed)

    return new_entity_id


def create_prefab(entity_ids, file_name, prefab_name=None):
    file_path = prefab_test_utils.get_prefab_file_path(file_name)
    create_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'CreatePrefabInMemory', entity_ids, file_path)
    if create_prefab_result.IsSuccess():
        print(Results.create_prefab_succeeded)
    else:
        print(Results.create_prefab_failed)
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
    if instantiate_prefab_result.IsSuccess():
        print(Results.instantiate_prefab_succeeded)
    else:
        print(Results.instantiate_prefab_failed)
        return EntityId()

    container_entity_id = instantiate_prefab_result.GetValue()

    if prefab_name is not None:
        editor.EditorEntityAPIBus(bus.Event, 'SetName', container_entity_id, prefab_name)
    else:
        prefab_name = file_name

    prefab_test_utils.wait_for_propagation()
    container_entity_id = prefab_test_utils.find_entity_by_unique_name(prefab_name)

    actual_prefab_position = components.TransformBus(bus.Event, "GetWorldTranslation", container_entity_id)
    is_at_position = actual_prefab_position.IsClose(expected_prefab_position)
    if is_at_position:
        print(Results.instantiated_prefab_at_expected_position)
    else:
        print(Results.instantiated_prefab_not_at_expected_position)
        print(f'Expected position: {expected_prefab_position.ToString()}, actual position: {actual_prefab_position.ToString()}')
        return EntityId()

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
    if delete_prefab_result.IsSuccess():
        print(Results.delete_prefab_succeeded)
    else:
        print(Results.delete_prefab_failed)
        return

    prefab_test_utils.wait_for_propagation()

    prefab_entities_deleted = True
    for entity_name, expected_entity_count in instances_removed_expected_name_counts.items():
        actual_entity_count = len(prefab_test_utils.find_entities_by_name(entity_name))
        if actual_entity_count is not expected_entity_count:
            prefab_entities_deleted = False
            break

    if prefab_entities_deleted:
        print(Results.delete_prefab_entities_and_descendants_succeeded)
    else:
        print(Results.delete_prefab_entities_and_descendants_failed)


async def reparent_prefab(container_entity_id, parent_entity_id):
    prefab_name = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', container_entity_id)
    current_children_entity_ids_having_prefab_name = prefab_test_utils.get_children_by_name(parent_entity_id, prefab_name)

    pyside_utils.run_soon(lambda: editor.EditorEntityAPIBus(bus.Event, 'SetParent', container_entity_id, parent_entity_id))
    pyside_utils.run_soon(lambda: prefab_test_utils.wait_for_propagation())

    try:
        active_modal_widget = await pyside_utils.wait_for_modal_widget()
        error_message_box = active_modal_widget.findChild(QtWidgets.QMessageBox)
        ok_button = error_message_box.button(QtWidgets.QMessageBox.Ok)
        ok_button.click()
        print(Results.reparent_prefab_cyclical_dependency_detected)
        return EntityId()
    except pyside_utils.EventLoopTimeoutException:
        pass        

    updated_children_entity_ids_having_prefab_name = prefab_test_utils.get_children_by_name(parent_entity_id, prefab_name)
    if len(updated_children_entity_ids_having_prefab_name) is not len(current_children_entity_ids_having_prefab_name) + 1:
        print(Results.reparent_prefab_parent_not_own_prefab)
        return EntityId()
        
    updated_container_entity_id = set(updated_children_entity_ids_having_prefab_name).difference(current_children_entity_ids_having_prefab_name).pop()
    updated_container_entity_parent_id = editor.EditorEntityInfoRequestBus(bus.Event, 'GetParent', updated_container_entity_id)
    has_correct_parent = updated_container_entity_parent_id.ToString() == parent_entity_id.ToString()
    if not has_correct_parent:
        print(Results.reparent_prefab_not_under_expected_parent)
        return EntityId()

    print(Results.reparent_prefab_succeeded)
    return updated_container_entity_id
