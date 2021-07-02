"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Built-in Imports
from __future__ import annotations
from typing import List, Tuple, Union


# Open 3D Engine Imports
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.legacy.general as general

# Helper file Imports
from editor_python_test_tools.utils import Report

class EditorComponent:
    """
    EditorComponent class used to set and get the component property value using path
    EditorComponent object is returned from either of
        EditorEntity.add_component() or Entity.add_components() or EditorEntity.get_component_objects()
    which also assigns self.id and self.type_id to the EditorComponent object.
    """

    # Methods
    def get_component_name(self) -> str:
        """
        Used to get name of component
        :return: name of component
        """
        type_names = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeNames", [self.type_id])
        assert len(type_names) != 0, "Component object does not have type id"
        return type_names[0]

    def get_property_tree(self):
        """
        Used to get the property tree object of component that has following functions associated with it:
            1. prop_tree.is_container(path)
            2. prop_tree.get_container_count(path)
            3. prop_tree.reset_container(path)
            4. prop_tree.add_container_item(path, key, item)
            5. prop_tree.remove_container_item(path, key)
            6. prop_tree.update_container_item(path, key, value)
            7. prop_tree.get_container_item(path, key)
        :return: Property tree object of a component
        """
        build_prop_tree_outcome = editor.EditorComponentAPIBus(
            bus.Broadcast, "BuildComponentPropertyTreeEditor", self.id
        )
        assert (
            build_prop_tree_outcome.IsSuccess()
        ), f"Failure: Could not build property tree of component: '{self.get_component_name()}'"
        prop_tree = build_prop_tree_outcome.GetValue()
        Report.info(prop_tree.build_paths_list())
        return prop_tree

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

    @staticmethod
    def get_type_ids(component_names: list) -> list:
        """
        Used to get type ids of given components list
        :param: component_names: List of components to get type ids
        :return: List of type ids of given components.
        """
        type_ids = editor.EditorComponentAPIBus(
            bus.Broadcast, "FindComponentTypeIdsByEntityType", component_names, azlmbr.entity.EntityType().Game
        )
        return type_ids


class EditorEntity:
    """
    Entity class is used to create and interact with Editor Entities.
    Example: To create Editor Entity, Use the code:
        test_entity = Entity.create_editor_entity("TestEntity")
        # This creates a python object with 'test_entity' linked to entity name "TestEntity" in Editor.
        # To add component, use:
        test_entity.add_component(<COMPONENT_NAME>)
    """

    def __init__(self, id: azlmbr.entity.EntityId):
        self.id: azlmbr.entity.EntityId = id

    # Creation functions
    @classmethod
    def find_editor_entity(cls, entity_name: str) -> EditorEntity:
        """
        Given Entity name, outputs entity object
        :param entity_name: Name of entity to find
        :return: EditorEntity class object
        """
        entity_id = general.find_editor_entity(entity_name)
        assert entity_id.IsValid(), f"Failure: Couldn't find entity with name: '{entity_name}'"
        entity = cls(entity_id)
        return entity

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
        parent_id: azlmbr.entity.EntityId = None,
    ) -> EditorEntity:
        """
        Used to create entity at position using 'CreateNewEntityAtPosition' Bus.
        :param entity_position: World Position(X, Y, Z) of entity in viewport.
                                Example: [512.0, 512.0, 32.0]
        :param name: Name of the Entity to be created
        :parent_id: (optional) Used to create child entity under parent_id if specified
        :Example: test_entity = EditorEntity.create_editor_entity_at([512.0, 512.0, 32.0], "TestEntity")
        :return: EditorEntity class object
        """

        def convert_to_azvector3(xyz) -> math.Vector3:
            if isinstance(xyz, Tuple) or isinstance(xyz, List):
                assert len(xyz) == 3, ValueError("vector must be a 3 element list/tuple or azlmbr.math.Vector3")
                return math.Vector3(*xyz)
            elif isinstance(xyz, type(math.Vector3())):
                return xyz
            else:
                raise ValueError("vector must be a 3 element list/tuple or azlmbr.math.Vector3")

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
    def set_name(self, entity_name: str):
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
        type_ids = EditorComponent.get_type_ids(component_names)
        for type_id in type_ids:
            new_comp = EditorComponent()
            new_comp.type_id = type_id
            add_component_outcome = editor.EditorComponentAPIBus(
                bus.Broadcast, "AddComponentsOfType", self.id, [type_id]
            )
            assert (
                add_component_outcome.IsSuccess()
            ), f"Failure: Could not add component: '{new_comp.get_component_name()}' to entity: '{self.get_name()}'"
            new_comp.id = add_component_outcome.GetValue()[0]
            components.append(new_comp)

        return components

    def get_components_of_type(self, component_names: list) -> List[EditorComponent]:
        """
        Used to get components of type component_name that already exists on Entity
        :param component_name: Name to component to check
        :return: List of Entity Component objects of given component name
        """
        component_list = []
        type_ids = EditorComponent.get_type_ids(component_names)
        for type_id in type_ids:
            component = EditorComponent()
            component.type_id = type_id
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
        type_ids = EditorComponent.get_type_ids([component_name])
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

    def set_start_status(self, desired_start_status: str):
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
