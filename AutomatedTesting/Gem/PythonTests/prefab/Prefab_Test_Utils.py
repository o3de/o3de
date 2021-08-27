"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.legacy.general as general

from prefab.Prefab_Test_Results import Results


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
    if len(entities) is not 1:
        print(Results.entity_not_have_unique_name)
        print(f'Name {entity_name} has been used for multiple entities. It is *not* a unique name.')
        return EntityId()

    return entities[0]


def check_entity_children_count(entity_id, expected_children_count):
    children_entity_ids = editor.EditorEntityInfoRequestBus(bus.Event, 'GetChildren', entity_id)
    if len(children_entity_ids) is expected_children_count:
        print(Results.entity_has_expected_children_count)
    else:
        print(Results.entity_not_have_expected_children_count)
        print(f"Entity {entity_id} actual children count: {len(children_entity_ids)}. Expected children count: {expected_children_count}")


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
