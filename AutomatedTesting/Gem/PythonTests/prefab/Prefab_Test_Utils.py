"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os

from azlmbr.entity import EntityId
from azlmbr.math import Vector3
from editor_python_test_tools.editor_entity_utils import EditorEntity
from editor_python_test_tools.utils import Report
from editor_python_test_tools.utils import TestHelper as helper

import azlmbr.bus as bus
import azlmbr.components as components
import azlmbr.entity as entity
import azlmbr.legacy.general as general

def get_prefab_file_name(prefab_name):
    return prefab_name + ".prefab"

def get_prefab_file_path(prefab_name):
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), get_prefab_file_name(prefab_name))

def find_entities_by_name(entity_name):
    searchFilter = entity.SearchFilter()
    searchFilter.names = [entity_name]
    return entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)

def get_all_entities():
    return entity.SearchBus(bus.Broadcast, 'SearchEntities', entity.SearchFilter())

def check_entity_at_position(entity_id, expected_entity_position):
    entity_at_expected_position_result = (
        "entity is at expected position",
        "entity is *not* at expected position")

    actual_entity_position = components.TransformBus(bus.Event, "GetWorldTranslation", entity_id)
    is_at_position = actual_entity_position.IsClose(expected_entity_position)
    Report.result(entity_at_expected_position_result, is_at_position)

    if not is_at_position:
        Report.info(f"Entity '{entity_id.ToString()}'\'s expected position: {expected_entity_position.ToString()}, actual position: {actual_entity_position.ToString()}")
    
    return is_at_position

def check_entity_children_count(entity_id, expected_children_count):
    entity_children_count_matched_result = (
        "Entity with a unique name found",
        "Entity with a unique name *not* found")

    entity = EditorEntity(entity_id)
    children_entity_ids = entity.get_children_ids()
    entity_children_count_matched = len(children_entity_ids) == expected_children_count
    Report.result(entity_children_count_matched_result, entity_children_count_matched)

    if not entity_children_count_matched:
        Report.info(f"Entity '{entity_id.ToString()}' actual children count: {len(children_entity_ids)}. Expected children count: {expected_children_count}")

    return entity_children_count_matched

def get_children_ids_by_name(entity_id, entity_name):
    entity = EditorEntity(entity_id)
    children_entity_ids = entity.get_children_ids()
    
    result = []
    for child_entity_id in children_entity_ids:
        child_entity = EditorEntity(child_entity_id)
        child_entity_name = child_entity.get_name()
        if child_entity_name == entity_name:
            result.append(child_entity_id)

    return result

def wait_for_propagation():
    general.idle_wait_frames(1)

def open_base_tests_level():
    helper.init_idle()
    helper.open_level("Prefab", "Base")
