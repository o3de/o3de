"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import math

from editor_python_test_tools.editor_entity_utils import EditorEntity
from editor_python_test_tools.prefab_utils import Prefab
from editor_python_test_tools.wait_utils import PrefabWaiter
from editor_python_test_tools.utils import Report
from editor_python_test_tools.utils import TestHelper as helper

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.legacy.general as general
import azlmbr.globals as globals


def get_linear_nested_items_name(nested_items_name_prefix, current_level):
    return f"{nested_items_name_prefix}{current_level}"


def create_linear_nested_entities(nested_entities_name_prefix, level_count, pos, parent_id=None):
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
    :param parent_id: EntityId of the intended parent to the root entity
    :return: Root of the newly created nested entities.
    """
    assert level_count > 0, "Can't create nested entities with less than one level"

    current_entity = EditorEntity.create_editor_entity_at(
        pos, name=get_linear_nested_items_name(nested_entities_name_prefix, 0), parent_id=parent_id)
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


def validate_count_for_named_editor_entity(entity_name, expected_count):
    """
    This is a helper function which helps validate the number of entities for a given name in editor.

    :param entity_name: Entity name for the entities to be validated.
    :param expected_count: Expected number of entities.
    """
    entities = EditorEntity.find_editor_entities([entity_name])
    assert len(entities) == expected_count, f"{len(entities)} entity(s) found. " \
                                            f"Expected {expected_count} {entity_name} entity(s)."


def validate_child_count_for_named_editor_entity(entity_name, expected_child_count):
    """
    This is a helper function which helps validate the number of children of entities for a given name in editor.

    :param entity_name: Entity name for the entities to be validated.
    :param expected_child_count: Expected number of children.
    """
    entities = EditorEntity.find_editor_entities([entity_name])
    for entity in entities:
        child_entities = entity.get_children()
        assert len(child_entities) == expected_child_count, f"{len(child_entities)} children found. " \
                                                            f"Expected {expected_child_count} children for all {entity_name} entity(s)."


def open_base_tests_level():
    helper.init_idle()
    helper.open_level("Prefab", "Base")


def validate_undo_redo_on_prefab_creation(prefab_instance, original_parent_id):
    """
    This is a helper function which helps validate undo/redo functionality after creating a prefab.

    :param prefab_instance: A prefab instance generated by Prefab.create_prefab()
    :param original_parent_id: The EntityId of the original parent to the entity that a prefab was created from
    """
    # Get information on prefab instance's child entities
    child_entities = prefab_instance.get_direct_child_entities()
    child_entity_names = []
    for child_entity in child_entities:
        child_entity_names.append(child_entity.get_name())

    # Undo the create prefab operation
    general.undo()
    PrefabWaiter.wait_for_propagation()

    # Validate that undo has reverted the addition of the EditorPrefabComponent
    is_prefab = editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", prefab_instance.container_entity.id,
                                             globals.property.EditorPrefabComponentTypeId)
    assert not is_prefab, "Undo operation failed. Entity is still recognized as a prefab."

    # Validate that undo has restored the original parent entity of the entity used to create the prefab
    search_filter = entity.SearchFilter()
    search_filter.names = child_entity_names
    entities_found = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
    for child_entity in entities_found:
        assert EditorEntity(child_entity).get_parent_id() == original_parent_id, \
            "Original parent was not restored on Undo."

    # Redo the create prefab operation
    general.redo()
    PrefabWaiter.wait_for_propagation()

    # Validate that redo has re-added the EditorPrefabComponent to the prefab instance
    is_prefab = editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", prefab_instance.container_entity.id,
                                             globals.property.EditorPrefabComponentTypeId)
    assert is_prefab, "Redo operation failed. Entity is not recognized as a prefab."

    # Validate the redo has restored the child entities under the container entity
    entities_found = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
    for child_entity in entities_found:
        assert EditorEntity(child_entity).get_parent_id() == prefab_instance.container_entity.id, \
            "Prefab parent was not restored on Redo."


def validate_spawned_entity_rotation(entity, expected_rotation):
    """
    This is a helper function which helps validate the rotation of entities spawned via the spawnable API
    :param entity: The spawned entity on which to validate transform values
    :param expected_rotation: The expected world rotation of the spawned entity
    """
    spawned_entity_rotation = entity.get_world_rotation()
    x_rotation_success = math.isclose(spawned_entity_rotation.x, expected_rotation.x,
                                      rel_tol=1e-5)
    y_rotation_success = math.isclose(spawned_entity_rotation.y, expected_rotation.y,
                                      rel_tol=1e-5)
    z_rotation_success = math.isclose(spawned_entity_rotation.z, expected_rotation.z,
                                      rel_tol=1e-5)
    Report.info(f"Spawned Entity Rotation: Found {spawned_entity_rotation}, expected {expected_rotation}")
    return x_rotation_success and y_rotation_success and z_rotation_success


def validate_spawned_entity_scale(entity, expected_scale):
    """
    This is a helper function which helps validate the scale of entities spawned via the spawnable API
    :param entity: The spawned entity on which to validate transform values
    :param expected_scale: The expected world scale of the spawned entity
    """
    spawned_entity_scale = entity.get_world_uniform_scale()
    scale_success = spawned_entity_scale == expected_scale
    Report.info(f"Spawned Entity Scale: Found {spawned_entity_scale}, expected {expected_scale}")
    return scale_success


def validate_spawned_entity_translation(entity, expected_position):
    """
    This is a helper function which helps validate the world position of entities spawned via the spawnable API
    :param entity: The spawned entity on which to validate transform values
    :param expected_position: The expected world translation of the spawned entity
    """
    spawned_entity_position = entity.get_world_translation()
    position_success = spawned_entity_position == expected_position
    Report.info(f"Spawned Entity Translation: Found {spawned_entity_position}, expected {expected_position}")
    return position_success


def validate_spawned_entity_transform(entity, expected_position, expected_rotation, expected_scale):
    """
    This is a helper function which helps validate the transform of entities spawned via the spawnable API
    :param entity: The spawned entity on which to validate transform values
    :param expected_position: The expected world position of the spawned entity
    :param expected_rotation: The expected world rotation of the spawned entity
    :param expected_scale: The expected local scale of the spawned entity
    """

    position_success = helper.wait_for_condition(lambda: validate_spawned_entity_translation(entity, expected_position),
                                                 5.0)
    rotation_success = helper.wait_for_condition(lambda: validate_spawned_entity_rotation(entity, expected_rotation),
                                                 5.0)
    scale_success = helper.wait_for_condition(lambda: validate_spawned_entity_scale(entity, expected_scale),
                                              5.0)

    assert position_success, \
        f"Entity was not spawned in the position expected: Found {entity.get_world_translation()}, " \
        f"expected {expected_position}"
    assert rotation_success, \
        f"Entity was not spawned with the rotation expected: Found {entity.get_world_rotation()}, " \
        f"expected {expected_rotation}"
    assert scale_success, \
        f"Entity was not spawned with the scale expected: Found {entity.get_world_uniform_scale()}, " \
        f"expected {expected_scale}"

    return position_success and rotation_success and scale_success
