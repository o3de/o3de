"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os

from azlmbr.entity import EntityId
from azlmbr.math import Vector3
from editor_python_test_tools.utils import Report
from editor_python_test_tools.utils import TestHelper as helper

import azlmbr.bus as bus
import azlmbr.components as components
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.legacy.general as general


class UtilsResults:
    unique_name_entity_found = (
        "Entity with a unique name found",
        "Entity with a unique name *not* found")

    prefab_at_expected_position = (
        "prefab is at expected position",
        "prefab is *not* at expected position")

    entity_children_count_matched = (
        "Entity with a unique name found",
        "Entity with a unique name *not* found")


def get_prefab_file_name(prefab_name):
    return prefab_name + ".prefab"


def get_prefab_file_path(prefab_name):
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), get_prefab_file_name(prefab_name))


def find_entities_by_name(entity_name):
    searchFilter = entity.SearchFilter()
    searchFilter.names = [entity_name]
    return entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)


def find_entity_by_unique_name(entity_name):
    entities = find_entities_by_name(entity_name)
    unique_name_entity_found = len(entities) is 1
    Report.result(UtilsResults.unique_name_entity_found, unique_name_entity_found)

    if unique_name_entity_found:
        return entities[0]
    else:
        Report.info(f"{len(entities)} entities with name '{entity_name}' found")
        return EntityId()


def check_entity_at_position(entity_id, expected_prefab_position):
    actual_prefab_position = components.TransformBus(bus.Event, "GetWorldTranslation", entity_id)
    is_at_position = actual_prefab_position.IsClose(expected_prefab_position)
    Report.result(UtilsResults.prefab_at_expected_position, is_at_position)

    if not is_at_position:
        Report.info(f"Entity '{entity_id.ToString()}'\'s expected position: {expected_prefab_position.ToString()}, actual position: {actual_prefab_position.ToString()}")
    
    return is_at_position


def check_entity_children_count(entity_id, expected_children_count):
    children_entity_ids = editor.EditorEntityInfoRequestBus(bus.Event, 'GetChildren', entity_id)
    entity_children_count_matched = len(children_entity_ids) is expected_children_count
    Report.result(UtilsResults.entity_children_count_matched, entity_children_count_matched)

    if not entity_children_count_matched:
        Report.info(f"Entity '{entity_id.ToString()}' actual children count: {len(children_entity_ids)}. Expected children count: {expected_children_count}")

    return entity_children_count_matched


def get_children_by_name(parent_entity_id, entity_name):
    children_entity_ids = editor.EditorEntityInfoRequestBus(bus.Event, 'GetChildren', parent_entity_id)
    
    result = []
    for child_entity_id in children_entity_ids:
        child_entity_name = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', child_entity_id)
        if child_entity_name == entity_name:
            result.append(child_entity_id)

    return result


def wait_for_propagation():
    general.idle_wait_frames(1)


def open_base_tests_level():
    helper.init_idle()
    helper.open_level("Prefab", "Base")

