"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.legacy.general as general
import azlmbr.object

from typing import List
from math import isclose
import collections.abc


def find_entity_by_name(entity_name):
    """
    Gets an entity ID from the entity with the given entity_name
    :param entity_name: String of entity name to search for
    :return entity ID
    """
    search_filter = entity.SearchFilter()
    search_filter.names = [entity_name]
    matching_entity_list = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
    if matching_entity_list:
        matching_entity = matching_entity_list[0]
        if matching_entity.IsValid():
            print(f'{entity_name} entity found with ID {matching_entity.ToString()}')
            return matching_entity
    else:
        return matching_entity_list


def get_component_type_id(component_name):
    """
    Gets the component_type_id from a given component name
    :param component_name: String of component name to search for
    :return component type ID
    """
    type_ids_list = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', [component_name],
                                                 entity.EntityType().Game)
    component_type_id = type_ids_list[0]
    return component_type_id


def add_level_component(component_name):
    """
    Adds the specified component to the Level Inspector
    :param component_name: String of component name to search for
    :return Component object.
    """
    level_component_list = [component_name]
    level_component_type_ids_list = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType',
                                                                 level_component_list, entity.EntityType().Level)
    level_component_outcome = editor.EditorLevelComponentAPIBus(bus.Broadcast, 'AddComponentsOfType',
                                                                [level_component_type_ids_list[0]])
    if not level_component_outcome.IsSuccess():
        print('Failed to add {} level component'.format(component_name))
        return None

    level_component = level_component_outcome.GetValue()[0]
    return level_component


def add_component(componentName, entityId):
    """
    Given a component name, finds component TypeId, adds to given entity, and verifies successful add/active state.
    :param componentName: String of component name to add.
    :param entityId: Entity to add component to.
    :return: Component object.
    """
    typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', [componentName],
                                               entity.EntityType().Game)
    typeNamesList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeNames', typeIdsList)

    # If the type name comes back as empty, then it means componentName is invalid
    if len(typeNamesList) != 1 or not typeNamesList[0]:
        print('Unable to find component TypeId for {}'.format(componentName))
        return None

    componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, typeIdsList)
    if not componentOutcome.IsSuccess():
        print('Failed to add {} component to entity'.format(typeNamesList[0]))
        return None

    isActive = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', componentOutcome.GetValue()[0])
    hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', entityId, typeIdsList[0])
    if isActive:
        print('{} component was added to entity'.format(typeNamesList[0]))
    else:
        print('{} component was added to entity, but the component is disabled'.format(typeNamesList[0]))
    if hasComponent:
        print('Entity has a {} component'.format(typeNamesList[0]))
    return componentOutcome.GetValue()[0]


def add_component_of_type(componentTypeId, entityId):
    typeIdsList = [componentTypeId]
    componentOutcome = editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 'AddComponentsOfType', entityId, typeIdsList)
    return componentOutcome.GetValue()[0]


def remove_component(component_name, entity_id):
    """
    Removes the specified component from the specified entity.
    :param component_name: String of component name to remove.
    :param entity_id: Entity to remove component from.
    :return: EntityComponentIdPair if removal was successful, else None.
    """
    type_ids_list = [get_component_type_id(component_name)]
    outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', entity_id, type_ids_list[0])
    if outcome.IsSuccess():
        component_entity_pair = outcome.GetValue()
        editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [component_entity_pair])
        has_component = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', entity_id, type_ids_list[0])
        if has_component:
            print(f"Failed to remove {component_name}")
            return None
        else:
            print(f"{component_name} was successfully removed")
            return component_entity_pair
    else:
        print(f"{component_name} not found on entity")
        return None


def get_component_property_value(component, component_propertyPath):
    """
    Given a component name and component property path, outputs the property's value.
    :param component: Component object to act on.
    :param componentPropertyPath: String of component property. (e.g. 'Settings|Visible')
    :return: Value set in given componentPropertyPath
    """
    componentPropertyObj = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component,
                                                        component_propertyPath)
    if componentPropertyObj.IsSuccess():
        componentProperty = componentPropertyObj.GetValue()
        print(f'{component_propertyPath} set to {componentProperty}')
        return componentProperty
    else:
        print(f'FAILURE: Could not get value from {component_propertyPath}')
        return None


def get_property_tree(component):
    """
    Given a configured component object, prints the property tree info from that component
    :param component: Component object to act on.
    """
    pteObj = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyTreeEditor', component)
    pte = pteObj.GetValue()
    print(pte.build_paths_list())
    return pte


def compare_values(first_object: object, second_object: object, name: str) -> bool:
    # Quick case - can we just directly compare the two objects successfully?
    if (first_object == second_object):
        result = True
    # No, so get a lot more specific
    elif isinstance(first_object, collections.abc.Container):
        # If they aren't both containers, they're different
        if not isinstance(second_object, collections.abc.Container):
            result = False
        # If they have different lengths, they're different
        elif len(first_object) != len (second_object):
            result = False
        # If they're different strings, they're containers but they failed the == check so
        # we know they're different
        elif isinstance(first_object, str):
            result = False
        else:
            # It's a collection of values, so iterate through them all...
            collection_idx = 0
            result = True
            for val1, val2 in zip(first_object, second_object):
                result = result and compare_values(val1, val2, f"{name} (index [{collection_idx}])")
                collection_idx = collection_idx + 1

    else:
        # Do approximate comparisons for floats
        if isinstance(first_object, float) and isclose(first_object, second_object, rel_tol=0.001):
            result = True
        # We currently don't have a generic way to compare PythonProxyObject contents, so return a
        # false positive result for now.
        elif isinstance(first_object, azlmbr.object.PythonProxyObject):
            print(f"{name}: validation inconclusive, the two objects cannot be directly compared.")
            result = True
        else:
            result = False

    if not result:
        print(f"compare_values failed: {first_object} ({type(first_object)}) vs {second_object} ({type(second_object)})")

    print(f"{name}: {'SUCCESS' if result else 'FAILURE'}")
    return result


class Entity:
    """
    Entity class used to create entity objects
    :param name: String for the name of the Entity
    :param id: The ID of the entity
    """

    def __init__(self, name: str, id: object = entity.EntityId()):
        self.name: str = name
        self.id: object = id
        self.components: List[object] = None
        self.parent_id = None
        self.parent_name = None

    def create_entity(self, entity_position, components, parent_id=entity.EntityId()):
        self.id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, parent_id
        )
        if self.id.IsValid():
            print(f"{self.name} Entity successfully created")
            editor.EditorEntityAPIBus(bus.Event, 'SetName', self.id, self.name)
            self.components = []
            for component in components:
                self.add_component(component)

    def add_component(self, component):
        new_component = add_component(component, self.id)
        if new_component:
            self.components.append(new_component)

    def add_component_of_type(self, componentTypeId):
        new_component = add_component_of_type(componentTypeId, self.id)
        self.components.append(new_component)

    def remove_component(self, component):
        removed_component = remove_component(component, self.id)
        if removed_component is not None:
            self.components.remove(removed_component)

    def get_parent_info(self):
        """
        Sets the value for parent_id and parent_name on the entity (self)
        Prints the string for papertrail
        :return: None
        """
        self.parent_id = editor.EditorEntityInfoRequestBus(bus.Event, "GetParent", self.id)
        self.parent_name = editor.EditorEntityInfoRequestBus(bus.Event, "GetName", self.parent_id)
        print(f"The parent entity of {self.name} is {self.parent_name}")

    def set_test_parent_entity(self, parent_entity_obj):
        editor.EditorEntityAPIBus(bus.Event, "SetParent", self.id, parent_entity_obj.id)
        self.get_parent_info()

    def get_set_test(self, component_index: int, path: str, value: object, expected_result: object = None) -> bool:
        """
        Used to set and validate changes in component values
        :param component_index: Index location in the self.components list
        :param path: asset path in the component
        :param value: new value for the variable being changed in the component
        :param expected_result: (optional) check the result against a specific expected value
        """

        if expected_result is None:
            expected_result = value

        # Test Get/Set (get old value, set new value, check that new value was set correctly)
        print(f"Entity {self.name} Path {path} Component Index {component_index} ")

        component = self.components[component_index]
        old_value = get_component_property_value(component, path)

        if old_value is not None:
            print(f"SUCCESS: Retrieved property Value for {self.name}")
        else:
            print(f"FAILURE: Failed to find value in {self.name} {path}")
            return False

        if old_value == expected_result:
            print((f"WARNING: get_set_test on {self.name} is setting the same value that already exists ({old_value})."
                    "The set results will be inconclusive."))

        editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", component, path, value)

        new_value = get_component_property_value(self.components[component_index], path)

        if new_value is not None:
            print(f"SUCCESS: Retrieved new property Value for {self.name}")
        else:
            print(f"FAILURE: Failed to find new value in {self.name}")
            return False

        return compare_values(new_value, expected_result, f"{self.name} {path}")


def get_set_test(entity: object, component_index: int, path: str, value: object) -> bool:
    """
    Used to set and validate changes in component values
    :param component_index: Index location in the entity.components list
    :param path: asset path in the component
    :param value: new value for the variable being changed in the component
    """
    return entity.get_set_test(component_index, path, value)


def get_set_property_test(ly_object: object, attribute_name: str, value: object, expected_result: object = None) -> bool:
    """
    Used to set and validate BehaviorContext property changes in Open 3D Engine objects
    :param ly_object: The Open 3D Engine object to test
    :param attribute_name: property (attribute) name in the BehaviorContext
    :param value: new value for the variable being changed in the component
    :param expected_result: (optional) check the result against a specific expected value other than the one set
    """

    if expected_result is None:
        expected_result = value

    # Test Get/Set (get old value, set new value, check that new value was set correctly)
    print(f"Attempting to set {ly_object.typename}.{attribute_name} = {value} (expected result is {expected_result})")

    if hasattr(ly_object, attribute_name):
        print(f"SUCCESS: Located attribute {attribute_name} for {ly_object.typename}")
    else:
        print(f"FAILURE: Failed to find attribute {attribute_name} in {ly_object.typename}")
        return False

    old_value = getattr(ly_object, attribute_name)

    if old_value is not None:
        print(f"SUCCESS: Retrieved existing value {old_value} for {attribute_name} in {ly_object.typename}")
    else:
        print(f"FAILURE: Failed to retrieve value for {attribute_name} in {ly_object.typename}")
        return False

    if old_value == expected_result:
        print((f"WARNING: get_set_test on {attribute_name} is setting the same value that already exists ({old_value})."
                "The 'set' result for the test will be inconclusive."))

    setattr(ly_object, attribute_name, expected_result)

    new_value = getattr(ly_object, attribute_name)

    if new_value is not None:
        print(f"SUCCESS: Retrieved new value {new_value} for {attribute_name} in {ly_object.typename}")
    else:
        print(f"FAILURE: Failed to retrieve value for {attribute_name} in {ly_object.typename}")
        return False

    return compare_values(new_value, expected_result, f"{ly_object.typename}.{attribute_name}")


def has_components(entity_id: object, component_list: list) -> bool:
    """
    Used to verify if a given entity has all the components of components_list. Returns True if all the
    components are present, else False
    :param entity_id: entity id of the entity
    :param component_list: list of component names to be verified
    """
    typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', component_list,
                                               entity.EntityType().Game)
    for type_id in typeIdsList:
        if not editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', entity_id, type_id):
            return False
    return True


class PathNotFoundError(Exception):
    def __init__(self, path):
        self.path = path

    def __str__(self):
        return f"Path \"{self.path}\" not found in Editor Settings"


def get_editor_settings_path_list():
    """
    Get the list of Editor Settings paths
    """
    paths = editor.EditorSettingsAPIBus(bus.Broadcast, 'BuildSettingsList')
    return paths


def get_editor_settings_by_path(path):
    """
    Get the value of Editor Settings based on the path.
    :param path: path to the Editor Settings to get the value
    """
    if path not in get_editor_settings_path_list():
        raise PathNotFoundError(path)
    outcome = editor.EditorSettingsAPIBus(bus.Broadcast, 'GetValue', path)
    if outcome.isSuccess():
        return outcome.GetValue()
    raise RuntimeError(f"GetValue for path '{path}' failed")


def set_editor_settings_by_path(path, value, is_bool = False):
    """
    Set the value of Editor Settings based on the path.
    # NOTE: Some Editor Settings may need an Editor restart to apply. 
    # Ex: Enabling or disabling New Viewport Interaction Model
    :param path: path to the Editor Settings to get the value
    :param value: value to be set
    :param is_bool: True for Boolean settings (enable/disable), False for other settings
    """
    if path not in get_editor_settings_path_list():
        raise PathNotFoundError(path)
    if is_bool and not isinstance(value, bool):
        def ParseBoolValue(value):
            if(value == "0"):
                return False
            return True
        value = ParseBoolValue(value)
    outcome = editor.EditorSettingsAPIBus(bus.Broadcast, 'SetValue', path, value)
    if not outcome.isSuccess():
        raise RuntimeError(f"SetValue for path '{path}' failed")
    print(f"Value for path '{path}' is set to {value}")


def get_component_type_id_map(component_name_list):
    """
    Given a list of component names, returns a map of component name -> component type id
    :param component_name_list: The Open 3D Engine object to test
    :return: Dictionary of component name -> component type id pairs
    """
    # Remove any duplicates so we don't have to query for the same TypeId
    component_names = list(set(component_name_list))

    type_ids_by_component = {}
    type_ids = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', component_names,
                                            entity.EntityType().Game)
    for i, typeId in enumerate(type_ids):
        type_ids_by_component[component_names[i]] = typeId

    return type_ids_by_component


def attach_component_to_entity(entity_id, component_name):
    # type: (azlmbr.entity.EntityId, str) -> azlmbr.entity.EntityComponentIdPair
    """
    Adds the component if not added already.
    :param entity_id: EntityId of the entity to attach the component to
    :param component_name: name of the component
    :return: If successful, returns the EntityComponentIdPair, otherwise returns None.
    """
    type_ids_list = editor.EditorComponentAPIBus(
        bus.Broadcast, 'FindComponentTypeIdsByEntityType', [component_name], 0)
    general.log(f"Components found = {len(type_ids_list)}")
    if len(type_ids_list) < 1:
        general.log(f"ERROR: A component class with name {component_name} doesn't exist")
        return None
    elif len(type_ids_list) > 1:
        general.log(f"ERROR: Found more than one component classes with same name: {component_name}")
        return None
    # Before adding the component let's check if it is already attached to the entity.
    component_outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', entity_id, type_ids_list[0])
    if component_outcome.IsSuccess():
        return component_outcome.GetValue()  # In this case the value is not a list.
    component_outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entity_id, type_ids_list)
    if component_outcome.IsSuccess():
        general.log(f"{component_name} Component added to entity.")
        return component_outcome.GetValue()[0]
    general.log(f"ERROR: Failed to add component [{component_name}] to entity")
    return None
