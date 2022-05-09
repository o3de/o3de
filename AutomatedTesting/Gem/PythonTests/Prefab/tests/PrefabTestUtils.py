"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os

from azlmbr.entity import EntityId
from azlmbr.math import Vector3
from editor_python_test_tools.editor_entity_utils import EditorEntity
from editor_python_test_tools.prefab_utils import Prefab
from editor_python_test_tools.utils import Report
from editor_python_test_tools.utils import TestHelper as helper

import azlmbr.bus as bus
import azlmbr.components as components
import azlmbr.entity as entity
import azlmbr.legacy.general as general

def get_linear_nested_items_name(nested_items_name_prefix, current_level):
    return f"{nested_items_name_prefix}{current_level}"

def create_linear_nested_entities(nested_entities_name_prefix, level_count, pos):
    """
    This is a helper function which helps create nested entities 
    where each nested entity has only one child entity at most. For example:

    Entity_0
    |- Entity_1 
    |  |- Entity_2
    ...

    :param nested_entities_name_prefix: Name prefix which will be used to generate names of newly created nested entities.
    :param level_count: Number of levels which the newly constructed nested entities will have.
    :param pos: The position where the nested entities will be.
    :return: Root of the newly created nested entities.
    """
    assert level_count > 0, "Can't create nested entities with less than one level"

    current_entity = EditorEntity.create_editor_entity_at(
        pos, name=get_linear_nested_items_name(nested_entities_name_prefix, 0))
    root_entity = current_entity
    for current_level in range(1, level_count):
        current_entity = EditorEntity.create_editor_entity(
            parent_id=current_entity.id, name=get_linear_nested_items_name(nested_entities_name_prefix, current_level))

    return root_entity

def validate_linear_nested_entities(nested_entities_root, expected_level_count, expected_pos):
    """
    This is a helper function which helps validate linear nested entities
    created by helper function create_linear_nested_entities.

    :param nested_entities_root: Root of nested entities created by create_linear_nested_entities.
    :param expected_level_count: Number of levels which target nested entities should have.
    :param expected_pos: The position where target nested entities should be.
    """
    assert expected_level_count > 0, "Can't validate nested entities with less than one layer"
    assert nested_entities_root.get_parent_id().IsValid(), \
        "Root of nested entities should have a valid parent entity"
    
    current_entity = nested_entities_root
    level_count = 1
    while True:
        assert current_entity.id.IsValid(), f"Entity '{current_entity.get_name()}' is not valid"

        current_entity_pos = current_entity.get_world_translation()
        assert current_entity_pos.IsClose(expected_pos), \
            f"Entity '{current_entity.get_name()}' position '{current_entity_pos.ToString()}' " \
            f"is not located at expected position '{expected_pos.ToString()}'"

        child_entities = current_entity.get_children()
        if len(child_entities) == 0:
            break

        assert len(child_entities) == 1, \
            f"These entities are not linearly nested. Entity '{current_entity.get_name}' has {len(child_entities)} child entities"

        level_count += 1
        child_entity = child_entities[0]
        assert child_entity.get_parent_id() == current_entity.id, \
            f"Entity '{child_entity.get_name()}' should be under entity '{current_entity.get_name()}'"

        current_entity = child_entity
        
    assert level_count == expected_level_count, \
        f"Number of levels of nested entities should be {expected_level_count}, not {level_count}"

def create_linear_nested_prefabs(entities, nested_prefabs_file_name_prefix, nested_prefabs_instance_name_prefix, level_count):
    """
    This is a helper function which helps create nested prefabs 
    where each nested prefab has only one child prefab at most. For example:

    Prefab_0
    |- Prefab_1 
    |  |- Prefab_2
    |  |  |- TestEntity
    ...

    :param entities: Entities which the nested prefabs will be built from.
    :param nested_prefabs_file_name_prefix: Name prefix which will be used to generate file names of newly created nested prefabs.
    :param level_count: Number of levels the newly constructed nested prefabs will have.
    :param nested_prefabs_instance_name_prefix: Name prefix which will be used to generate names of newly created nested prefab instances.
    :return: A list of created nested prefabs and a list of created nested prefab instances. Ordered from top to bottom.
    """
    assert level_count > 0, "Can't create nested prefabs with less than one level"

    created_prefabs = []
    created_prefab_instances = []

    for current_level in range(0, level_count):
        current_prefab, current_prefab_instance = Prefab.create_prefab(
            entities, get_linear_nested_items_name(nested_prefabs_file_name_prefix, current_level), 
            prefab_instance_name=get_linear_nested_items_name(nested_prefabs_instance_name_prefix, current_level))
        created_prefabs.append(current_prefab)
        created_prefab_instances.append(current_prefab_instance)
        entities = current_prefab_instance.get_direct_child_entities()
    
    return created_prefabs, created_prefab_instances

def validate_linear_nested_prefab_instances_hierarchy(nested_prefab_instances):
    """
    This is a helper function which helps validate linear nested prefabs 
    created by helper function create_linear_nested_prefabs.

    :param nested_prefab_instances: A list of nested prefab instances created by create_linear_nested_prefabs. Ordered from top to bottom.
    """
    if len(nested_prefab_instances) == 0:
        return

    nested_prefab_instances_root = nested_prefab_instances[0]
    assert nested_prefab_instances_root.container_entity.get_parent_id().IsValid(), \
        "Root of nested prefabs should have a valid parent entity"

    parent_prefab_instance = nested_prefab_instances_root
    for current_level in range(0, len(nested_prefab_instances) - 1):
        current_prefab_instance = nested_prefab_instances[current_level]
        current_prefab_instance_container_entity = current_prefab_instance.container_entity
        assert current_prefab_instance_container_entity.id.IsValid(), \
            f"Prefab '{current_prefab_instance_container_entity.get_name()}' is not valid"

        direct_child_entities = current_prefab_instance.get_direct_child_entities()
        assert len(direct_child_entities) == 1, \
            f"These prefab instances are not linearly nested. " \
            f"Entity '{current_entity.get_name}' has {len(direct_child_entities)} direct child entities"

        direct_child_entity = direct_child_entities[0]
        inner_prefab_instance = nested_prefab_instances[current_level + 1]
        inner_prefab_instance_container_entity = inner_prefab_instance.container_entity
        assert direct_child_entity.id == inner_prefab_instance_container_entity.id, \
            f"Direct child entity of prefab '{current_prefab_instance_container_entity.get_name()}' " \
            f"should be prefab '{inner_prefab_instance_container_entity.get_name()}', " \
            f"not '{direct_child_entity.get_name()}'."
        assert inner_prefab_instance_container_entity.get_parent_id() == current_prefab_instance_container_entity.id, \
            f"Prefab '{inner_prefab_instance_container_entity.get_name()}' should be the inner prefab of " \
            f"prefab '{current_prefab_instance_container_entity.get_name()}'"

    most_inner_prefab_instance = nested_prefab_instances[-1]
    most_inner_prefab_instance_container_entity = most_inner_prefab_instance.container_entity
    assert most_inner_prefab_instance_container_entity.id.IsValid(), \
        f"Prefab '{most_inner_prefab_instance_container_entity.get_name()}' is not valid"

    direct_child_entities = most_inner_prefab_instance.get_direct_child_entities()
    for entity in direct_child_entities:
        assert entity.get_parent_id() == most_inner_prefab_instance_container_entity.id, \
            f"Entity '{entity.get_name()}' should be under prefab '{most_inner_prefab_instance_container_entity.get_name()}'" 

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

def open_base_tests_level():
    helper.init_idle()
    helper.open_level("Prefab", "Base")
