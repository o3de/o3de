"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Built-in Imports
from __future__ import annotations
from typing import List, Tuple, Union
from enum import Enum
import warnings

# Open 3D Engine Imports
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.legacy.general as general

# Helper file Imports
from editor_python_test_tools.utils import Report


class Entity_Type(Enum):
    GAME = azlmbr.entity.EntityType().Game
    LEVEL = azlmbr.entity.EntityType().Level


class EditorComponent:
    """
    EditorComponent class used to set and get the component property value using path
    EditorComponent object is returned from either of
        EditorEntity.add_component() or Entity.add_components() or EditorEntity.get_components_of_type()
    which also assigns self.id and self.type_id to the EditorComponent object.
    self.type_id is the UUID for the component type as provided by an ebus call.
    """

    def __init__(self, type_id: uuid):
        self.type_id = type_id
        self.id = None
        self.property_tree = None

    def get_component_name(self) -> str:
        """
        Used to get name of component
        :return: name of component
        """
        type_names = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeNames", [self.type_id])
        assert len(type_names) != 0, "Component object does not have type id"
        return type_names[0]

    def get_property_tree(self, force_get: bool = False):
        """
        Used to get the property tree object of component that has following functions associated with it:
            1. prop_tree.is_container(path)
            2. prop_tree.get_container_count(path)
            3. prop_tree.reset_container(path)
            4. prop_tree.add_container_item(path, key, item)
            5. prop_tree.remove_container_item(path, key)
            6. prop_tree.update_container_item(path, key, value)
            7. prop_tree.get_container_item(path, key)
        :param force_get: Force a fresh property tree to be returned rather than using an existing self.property_tree
        :return: Property tree object of a component
        """
        if (not force_get) and (self.property_tree is not None):
            return self.property_tree

        build_prop_tree_outcome = editor.EditorComponentAPIBus(
            bus.Broadcast, "BuildComponentPropertyTreeEditor", self.id
        )
        assert (
            build_prop_tree_outcome.IsSuccess()
        ), f"Failure: Could not build property tree of component: '{self.get_component_name()}'"
        prop_tree = build_prop_tree_outcome.GetValue()
        Report.info(prop_tree.build_paths_list())
        self.property_tree = prop_tree
        return self.property_tree

    def is_property_container(self, component_property_path: str) -> bool:
        """
        Used to determine if a component property is a container. Containers are similar to a dictionary with int keys.
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :return: Boolean True if the property is a container False if it is not.
        """
        if self.property_tree is None:
            self.get_property_tree()
        result = self.property_tree.is_container(component_property_path)
        if not result:
            Report.info(f"{self.get_component_name()}: '{component_property_path}' is not a container")
        return result

    def get_container_count(self, component_property_path: str) -> int:
        """
        Used to get the count of items in the container.
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :return: Count of items in the container as unsigned integer
        """
        assert (
            self.is_property_container(component_property_path)
        ), f"Failure: '{component_property_path}' is not a property container"
        container_count_outcome = self.property_tree.get_container_count(component_property_path)
        assert (
            container_count_outcome.IsSuccess()
        ), f"Failure: get_container_count did not return success for '{component_property_path}'"
        return container_count_outcome.GetValue()

    def reset_container(self, component_property_path: str):
        """
        Used to rest a container to empty
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :return: None
        """
        assert (
            self.is_property_container(component_property_path)
        ), f"Failure: '{component_property_path}' is not a property container"
        reset_outcome = self.property_tree.reset_container(component_property_path)
        assert (
            reset_outcome.IsSuccess()
        ), f"Failure: could not reset_container on '{component_property_path}'"

    def append_container_item(self, component_property_path: str, value: any):
        """
        Used to append a container item without providing an index key.
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :param value: Value to be set
        :return: None
        """
        assert (
            self.is_property_container(component_property_path)
        ), f"Failure: '{component_property_path}' is not a property container"
        append_outcome = self.property_tree.append_container_item(component_property_path, value)
        assert (
            append_outcome.IsSuccess()
        ), f"Failure: could not append_container_item to '{component_property_path}'"

    def add_container_item(self, component_property_path: str, key: any, value: any):
        """
        Used to add a container item at a specified key. In practice key should be an integer index.
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :param key: Zero index integer key, although this could be any unique unused key value
        :param value: Value to be set
        :return: None
        """
        assert (
            self.is_property_container(component_property_path)
        ), f"Failure: '{component_property_path}' is not a property container"
        add_outcome = self.property_tree.add_container_item(component_property_path, key, value)
        assert (
            add_outcome.IsSuccess()
        ), f"Failure: could not add_container_item '{key}' to '{component_property_path}'"

    def get_container_item(self, component_property_path: str, key: any) -> any:
        """
        Used to retrieve a container item value at the specified key. In practice key should be an integer index.
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :param key: Zero index integer key
        :return: Value stored at the key specified
        """
        assert (
            self.is_property_container(component_property_path)
        ), f"Failure: '{component_property_path}' is not a property container"
        get_outcome = self.property_tree.get_container_item(component_property_path, key)
        assert (
            get_outcome.IsSuccess()
        ), f"Failure: could not get a value for {self.get_component_name()}: '{component_property_path}' [{key}]"
        return get_outcome.GetValue()

    def remove_container_item(self, component_property_path: str, key: any):
        """
        Used to remove a container item value at the specified key. In practice key should be an integer index.
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :param key: Zero index integer key
        :return: None
        """
        assert (
            self.is_property_container(component_property_path)
        ), f"Failure: '{component_property_path}' is not a property container"
        remove_outcome = self.property_tree.remove_container_item(component_property_path, key)
        assert (
            remove_outcome.IsSuccess()
        ), f"Failure: could not remove_container_item '{key}' from '{component_property_path}'"

    def update_container_item(self, component_property_path: str, key: any, value: any):
        """
        Used to update a container item at a specified key. In practice key should be an integer index.
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :param key: Zero index integer key
        :param value: Value to be set
        :return: None
        """
        assert (
            self.is_property_container(component_property_path)
        ), f"Failure: '{component_property_path}' is not a property container"
        update_outcome = self.property_tree.update_container_item(component_property_path, key, value)
        assert (
            update_outcome.IsSuccess()
        ), f"Failure: could not update '{key}' in '{component_property_path}'"

    def get_component_property_value(self, component_property_path: str):
        """
        Given a component property path, outputs the property's value
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :return: Value set in given component_property_path. Type is dependent on component property
        """
        get_component_property_outcome = editor.EditorComponentAPIBus(
            bus.Broadcast, "GetComponentProperty", self.id, component_property_path
        )
        assert (
            get_component_property_outcome.IsSuccess()
        ), f"Failure: Could not get value from {self.get_component_name()} : {component_property_path}"
        return get_component_property_outcome.GetValue()

    def set_component_property_value(self, component_property_path: str, value: object):
        """
        Used to set component property value
        :param component_property_path: Path of property in the component to act on
        :param value: new value for the variable being changed in the component
        """
        outcome = editor.EditorComponentAPIBus(
            bus.Broadcast, "SetComponentProperty", self.id, component_property_path, value
        )
        assert (
            outcome.IsSuccess()
        ), f"Failure: Could not set value to '{self.get_component_name()}' : '{component_property_path}'"

    def is_enabled(self):
        """
        Used to verify if the component is enabled.
        :return: True if enabled, otherwise False.
        """
        return editor.EditorComponentAPIBus(bus.Broadcast, "IsComponentEnabled", self.id)

    def set_enabled(self, new_state: bool):
        """
        Used to set the component enabled state
        :param new_state: Boolean enabled True, disabled False
        :return: None
        """
        if new_state:
            editor.EditorComponentAPIBus(bus.Broadcast, "EnableComponents", [self.id])
        else:
            editor.EditorComponentAPIBus(bus.Broadcast, "DisableComponents", [self.id])

    def disable_component(self):
        """
        Used to disable the component using its id value.
        Deprecation warning! Use set_enable(False) instead as this method is in deprecation
        :return: None
        """
        warnings.warn("disable_component is deprecated, use set_enabled(False) instead.", DeprecationWarning)
        editor.EditorComponentAPIBus(bus.Broadcast, "DisableComponents", [self.id])

    @staticmethod
    def get_type_ids(component_names: list, entity_type: Entity_Type = Entity_Type.GAME) -> list:
        """
        Used to get type ids of given components list
        :param component_names: List of components to get type ids
        :param entity_type: Entity_Type enum value Entity_Type.GAME is the default
        :return: List of type ids of given components. Type id is a UUID as provided by the ebus call
        """
        type_ids = editor.EditorComponentAPIBus(
            bus.Broadcast, "FindComponentTypeIdsByEntityType", component_names, entity_type)
        return type_ids


def convert_to_azvector3(xyz) -> azlmbr.math.Vector3:
    """
    Converts a vector3-like element into a azlmbr.math.Vector3
    """
    if isinstance(xyz, Tuple) or isinstance(xyz, List):
        assert len(xyz) == 3, ValueError("vector must be a 3 element list/tuple or azlmbr.math.Vector3")
        return math.Vector3(float(xyz[0]), float(xyz[1]), float(xyz[2]))
    elif isinstance(xyz, type(math.Vector3())):
        return xyz
    else:
        raise ValueError("vector must be a 3 element list/tuple or azlmbr.math.Vector3")


class EditorEntity:
    """
    Entity class is used to create and interact with Editor Entities.
    Example: To create Editor Entity, Use the code:
        test_entity = EditorEntity.create_editor_entity("TestEntity")
        # This creates a python object with 'test_entity' linked to entity name "TestEntity" in Editor.
        # To add component, use:
        test_entity.add_component(<COMPONENT_NAME>)
    """

    def __init__(self, id: azlmbr.entity.EntityId):
        self.id: azlmbr.entity.EntityId = id
        self.components: List[EditorComponent] = []

    # Creation functions
    @classmethod
    def find_editor_entity(cls, entity_name: str, must_be_unique: bool = False) -> EditorEntity:
        """
        Given Entity name, outputs entity object
        :param entity_name: Name of entity to find
        :param must_be_unique: bool that asserts the entity_name specified is unique when set to True
        :return: EditorEntity class object
        """
        entities = cls.find_editor_entities([entity_name])
        assert len(entities) != 0, f"Failure: Couldn't find entity with name: '{entity_name}'"
        if must_be_unique:
            assert len(entities) == 1, f"Failure: Multiple entities with name: '{entity_name}' when expected only one"

        entity = entities[0]
        return entity

    @classmethod
    def find_editor_entities(cls, entity_names: List[str]) -> List[EditorEntity]:
        """
        Given Entities names, returns a list of EditorEntity 
        :param entity_names: List of entity names to find
        :return: List[EditorEntity] class object
        """
        searchFilter = azlmbr.entity.SearchFilter()
        searchFilter.names = entity_names
        ids = azlmbr.entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)
        return [cls(id) for id in ids]

    @classmethod
    def create_editor_entity(cls, name: str = None, parent_id=None) -> EditorEntity:
        """
        Used to create entity at default position using 'CreateNewEntity' Bus
        :param name: Name of the Entity to be created
        :param parent_id: (optional) Used to create child entity under parent_id if specified
        :return: EditorEntity class object
        """
        if parent_id is None:
            parent_id = azlmbr.entity.EntityId()

        new_id = azlmbr.editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", parent_id)
        assert new_id.IsValid(), "Failure: Could not create Editor Entity"
        entity = cls(new_id)
        if name:
            entity.set_name(name)

        return entity

    @classmethod
    def create_editor_entity_at(
        cls,
        entity_position: Union[List, Tuple, math.Vector3],
        name: str = None,
        parent_id: azlmbr.entity.EntityId = None) -> EditorEntity:
        """
        Used to create entity at position using 'CreateNewEntityAtPosition' Bus.
        :param entity_position: World Position(X, Y, Z) of entity in viewport.
                                Example: [512.0, 512.0, 32.0]
        :param name: Name of the Entity to be created
        :parent_id: (optional) Used to create child entity under parent_id if specified
        :Example: test_entity = EditorEntity.create_editor_entity_at([512.0, 512.0, 32.0], "TestEntity")
        :return: EditorEntity class object
        """

        if parent_id is None:
            parent_id = azlmbr.entity.EntityId()

        new_id = azlmbr.editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", convert_to_azvector3(entity_position), parent_id
        )
        assert new_id.IsValid(), "Failure: Could not create Editor Entity"
        entity = cls(new_id)
        if name:
            entity.set_name(name)

        return entity

    # Methods
    def set_name(self, entity_name: str) -> None:
        """
        Given entity_name, sets name to Entity
        :param: entity_name: Name of the entity to set
        """
        editor.EditorEntityAPIBus(bus.Event, "SetName", self.id, entity_name)

    def get_name(self) -> str:
        """
        Used to get the name of entity
        """
        return editor.EditorEntityInfoRequestBus(bus.Event, "GetName", self.id)

    def set_parent_entity(self, parent_entity_id):
        """
        Used to set this entity to be child of parent entity passed in
        :param: parent_entity_id: Entity Id of parent to set
        """
        assert (
            parent_entity_id.IsValid()
        ), f"Failure: Could not set parent to entity: {self.get_name()}, Invalid parent id"
        editor.EditorEntityAPIBus(bus.Event, "SetParent", self.id, parent_entity_id)

    def get_parent_id(self) -> azlmbr.entity.EntityId:
        """
        :return: Entity id of parent. Type: entity.EntityId()
        """
        return editor.EditorEntityInfoRequestBus(bus.Event, "GetParent", self.id)

    def get_children_ids(self) -> List[azlmbr.entity.EntityId]:
        """
        :return: Entity ids of children. Type: [entity.EntityId()]
        """
        return editor.EditorEntityInfoRequestBus(bus.Event, "GetChildren", self.id)

    def get_children(self) -> List[EditorEntity]:
        """
        :return: List of EditorEntity children. Type: [EditorEntity]
        """
        return [EditorEntity(child_id) for child_id in self.get_children_ids()] 

    def add_component(self, component_name: str) -> EditorComponent:
        """
        Used to add new component to Entity.
        :param component_name: String of component name to add.
        :return: Component object of newly added component.
        """
        component = self.add_components([component_name])[0]
        return component

    def add_components(self, component_names: list) -> List[EditorComponent]:
        """
        Used to add multiple components
        :param: component_names: List of components to add to entity
        :return: List of newly added components to the entity
        """
        components = []
        type_ids = EditorComponent.get_type_ids(component_names, Entity_Type.GAME)
        for type_id in type_ids:
            new_comp = EditorComponent(type_id)
            add_component_outcome = editor.EditorComponentAPIBus(
                bus.Broadcast, "AddComponentsOfType", self.id, [type_id]
            )
            assert (
                add_component_outcome.IsSuccess()
            ), f"Failure: Could not add component: '{new_comp.get_component_name()}' to entity: '{self.get_name()}'"
            new_comp.id = add_component_outcome.GetValue()[0]
            components.append(new_comp)
            self.components.append(new_comp)
        return components

    def remove_component(self, component_name: str) -> None:
        """
        Used to remove a component from Entity
        :param component_name: String of component name to remove
        :return: None
        """
        self.remove_components([component_name])

    def remove_components(self, component_names: list):
        """
        Used to remove a list of components from Entity
        :param component_names: List of component names to remove
        :return: None
        """
        type_ids = EditorComponent.get_type_ids(component_names, Entity_Type.GAME)
        for type_id in type_ids:
            remove_outcome = editor.EditorComponentAPIBus(bus.Broadcast, "RemoveComponents", self.id, [type_id])
            assert (
                remove_outcome.IsSuccess()
            ), f"Failure: could not remove component from '{self.get_name()}'"

    def get_components_of_type(self, component_names: list) -> List[EditorComponent]:
        """
        Used to get components of type component_name that already exists on Entity
        :param component_names: List of names of components to check
        :return: List of Entity Component objects of given component name
        """
        component_list = []
        type_ids = EditorComponent.get_type_ids(component_names, Entity_Type.GAME)
        for type_id in type_ids:
            component = EditorComponent(type_id)
            get_component_of_type_outcome = editor.EditorComponentAPIBus(
                bus.Broadcast, "GetComponentOfType", self.id, type_id
            )
            assert (
                get_component_of_type_outcome.IsSuccess()
            ), f"Failure: Entity: '{self.get_name()}' does not have component:'{component.get_component_name()}'"
            component.id = get_component_of_type_outcome.GetValue()
            component_list.append(component)

        return component_list

    def has_component(self, component_name: str) -> bool:
        """
        Used to verify if the entity has the specified component
        :param component_name: Name of component to check for
        :return: True, if entity has specified component. Else, False
        """
        type_ids = EditorComponent.get_type_ids([component_name], Entity_Type.GAME)
        return editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", self.id, type_ids[0])

    def get_start_status(self) -> int:
        """
        This will return a value for an entity's starting status (active, inactive, or editor) in the form of azlmbr.globals.property.EditorEntityStartStatus_<start type>. For comparisons using this value, should compare against the azlmbr property and not the int value
        """
        status = editor.EditorEntityInfoRequestBus(bus.Event, "GetStartStatus", self.id)
        if status == azlmbr.globals.property.EditorEntityStartStatus_StartActive:
            status_text = "active"
        elif status == azlmbr.globals.property.EditorEntityStartStatus_StartInactive:
            status_text = "inactive"
        elif status == azlmbr.globals.property.EditorEntityStartStatus_EditorOnly:
            status_text = "editor"
        Report.info(f"The start status for {self.get_name} is {status_text}")
        self.start_status = status
        return status

    def set_start_status(self, desired_start_status: str) -> None:
        """
        Set an entity as active/inactive at beginning of runtime or it is editor-only,
        given its entity id and the start status then return set success
        :param desired_start_status: must be one of three choices: active, inactive, or editor
        """
        if desired_start_status == "active":
            status_to_set = azlmbr.globals.property.EditorEntityStartStatus_StartActive
        elif desired_start_status == "inactive":
            status_to_set = azlmbr.globals.property.EditorEntityStartStatus_StartInactive
        elif desired_start_status == "editor":
            status_to_set = azlmbr.globals.property.EditorEntityStartStatus_EditorOnly
        else:
            Report.info(
                f"Invalid desired_start_status argument for {self.get_name} set_start_status command;\
                Use editor, active, or inactive"
            )

        editor.EditorEntityAPIBus(bus.Event, "SetStartStatus", self.id, status_to_set)
        set_status = self.get_start_status()
        assert set_status == status_to_set, f"Failed to set start status of {desired_start_status} to {self.get_name}"

    def is_locked(self) -> bool:
        """
        Used to get the locked status of the entity
        :return: Boolean True if locked False if not locked
        """
        return editor.EditorEntityInfoRequestBus(bus.Event, "IsLocked", self.id)

    def set_lock_state(self, is_locked: bool) -> None:
        """
        Sets the lock state on the object to locked or not locked.
        :param is_locked: True for locking, False to unlock.
        :return: None
        """
        editor.EditorEntityAPIBus(bus.Event, "SetLockState", self.id, is_locked)

    def delete(self) -> None:
        """
        Used to delete the Entity.
        :return: None
        """
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntityById", self.id)

    def set_visibility_state(self, is_visible: bool) -> None:
        """
        Sets the visibility state on the object to visible or not visible.
        :param is_visible: True for making visible, False to make not visible.
        :return: None
        """
        editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", self.id, is_visible)

    def exists(self) -> bool:
        """
        Used to verify if the Entity exists.
        :return: True if the Entity exists, False otherwise.
        """
        return editor.ToolsApplicationRequestBus(bus.Broadcast, "EntityExists", self.id)

    def is_hidden(self) -> bool:
        """
        Gets the "isHidden" value from the Entity.
        :return: True if "isHidden" is enabled, False otherwise.
        """
        return editor.EditorEntityInfoRequestBus(bus.Event, "IsHidden", self.id)

    def is_visible(self) -> bool:
        """
        Gets the "isVisible" value from the Entity.
        :return: True if "isVisible" is enabled, False otherwise.
        """
        return editor.EditorEntityInfoRequestBus(bus.Event, "IsVisible", self.id)

    # World Transform Functions
    def get_world_translation(self) -> azlmbr.math.Vector3:
        """
        Gets the world translation of the entity
        """
        return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

    def set_world_translation(self, new_translation) -> None:
        """
        Sets the new world translation of the current entity
        """
        new_translation = convert_to_azvector3(new_translation)
        azlmbr.components.TransformBus(azlmbr.bus.Event, "SetWorldTranslation", self.id, new_translation)

    def get_world_rotation(self) -> azlmbr.math.Quaternion:
        """
        Gets the world rotation of the entity
        """
        return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldRotation", self.id)

    def set_world_rotation(self, new_rotation):
        """
        Sets the new world rotation of the current entity
        """
        new_rotation = convert_to_azvector3(new_rotation)
        azlmbr.components.TransformBus(azlmbr.bus.Event, "SetWorldRotation", self.id, new_rotation)

    # Local Transform Functions
    def get_local_uniform_scale(self) -> float:
        """
        Gets the local uniform scale of the entity
        """
        return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetLocalUniformScale", self.id)

    def set_local_uniform_scale(self, scale_float) -> None:
        """
        Sets the local uniform scale value(relative to the parent) on the entity.
        :param scale_float: value for "SetLocalUniformScale" to set to.
        :return: None
        """
        azlmbr.components.TransformBus(azlmbr.bus.Event, "SetLocalUniformScale", self.id, scale_float)

    def get_local_rotation(self) -> azlmbr.math.Quaternion:
        """
        Gets the local rotation of the entity
        """
        return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetLocalRotation", self.id)

    def set_local_rotation(self, new_rotation) -> None:
        """
        Sets the set the local rotation(relative to the parent) of the current entity.
        :param new_rotation: The math.Vector3 value to use for rotation on the entity (uses radians).
        :return: None
        """
        new_rotation = convert_to_azvector3(new_rotation)
        azlmbr.components.TransformBus(azlmbr.bus.Event, "SetLocalRotation", self.id, new_rotation)

    def get_local_translation(self) -> azlmbr.math.Vector3:
        """
        Gets the local translation of the current entity.
        :return: The math.Vector3 value of the local translation.
        """
        return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetLocalTranslation", self.id)

    def set_local_translation(self, new_translation) -> None:
        """
        Sets the local translation(relative to the parent) of the current entity.
        :param new_translation: The math.Vector3 value to use for translation on the entity.
        :return: None
        """
        new_translation = convert_to_azvector3(new_translation)
        azlmbr.components.TransformBus(azlmbr.bus.Event, "SetLocalTranslation", self.id, new_translation)

    # Use this only when prefab system is enabled as it will fail otherwise.
    def focus_on_owning_prefab(self) -> None:
        """
        Focuses on the owning prefab instance of the given entity.
        :param entity: The entity used to fetch the owning prefab to focus on.
        """

        assert self.id.isValid(), "A valid entity id is required to focus on its owning prefab."
        focus_prefab_result = azlmbr.prefab.PrefabFocusPublicRequestBus(bus.Broadcast, "FocusOnOwningPrefab", self.id)
        assert focus_prefab_result.IsSuccess(), f"Prefab operation 'FocusOnOwningPrefab' failed. Error: {focus_prefab_result.GetError()}"


class EditorLevelEntity:
    """
    EditorLevel class used to add and fetch level components.
    Level entity is a special entity that you do not create/destroy independently of larger systems of level creation.
    This collects a number of staticmethods that do not rely on entityId since Level entity is found internally by
    EditorLevelComponentAPIBus requests.
    """

    @staticmethod
    def add_component(component_name: str) -> EditorComponent:
        """
        Used to add new component to Level.
        :param component_name: String of component name to add.
        :return: Component object of newly added component.
        """
        component = EditorLevelEntity.add_components([component_name])[0]
        return component

    @staticmethod
    def add_components(component_names: list) -> List[EditorComponent]:
        """
        Used to add multiple components
        :param: component_names: List of components to add to level
        :return: List of newly added components to the level
        """
        components = []
        type_ids = EditorComponent.get_type_ids(component_names, Entity_Type.LEVEL)
        for type_id in type_ids:
            new_comp = EditorComponent(type_id)
            add_component_outcome = editor.EditorLevelComponentAPIBus(
                bus.Broadcast, "AddComponentsOfType", [type_id]
            )
            assert (
                add_component_outcome.IsSuccess()
            ), f"Failure: Could not add component: '{new_comp.get_component_name()}' to level"
            new_comp.id = add_component_outcome.GetValue()[0]
            components.append(new_comp)
        return components

    @staticmethod
    def get_components_of_type(component_names: list) -> List[EditorComponent]:
        """
        Used to get components of type component_name that already exists on the level
        :param component_names: List of names of components to check
        :return: List of Level Component objects of given component name
        """
        component_list = []
        type_ids = EditorComponent.get_type_ids(component_names, Entity_Type.LEVEL)
        for type_id in type_ids:
            component = EditorComponent(type_id)
            get_component_of_type_outcome = editor.EditorLevelComponentAPIBus(
                bus.Broadcast, "GetComponentOfType", type_id
            )
            assert (
                get_component_of_type_outcome.IsSuccess()
            ), f"Failure: Level does not have component:'{component.get_component_name()}'"
            component.id = get_component_of_type_outcome.GetValue()
            component_list.append(component)

        return component_list

    @staticmethod
    def has_component(component_name: str) -> bool:
        """
        Used to verify if the level has the specified component
        :param component_name: Name of component to check for
        :return: True, if level has specified component. Else, False
        """
        type_ids = EditorComponent.get_type_ids([component_name], Entity_Type.LEVEL)
        return editor.EditorLevelComponentAPIBus(bus.Broadcast, "HasComponentOfType", type_ids[0])

    @staticmethod
    def count_components_of_type(component_name: str) -> int:
        """
        Used to get a count of the specified level component attached to the level
        :param component_name: Name of component to check for
        :return: integer count of occurences of level component attached to level or zero if none are present
        """
        type_ids = EditorComponent.get_type_ids([component_name], Entity_Type.LEVEL)
        return editor.EditorLevelComponentAPIBus(bus.Broadcast, "CountComponentsOfType", type_ids[0])
