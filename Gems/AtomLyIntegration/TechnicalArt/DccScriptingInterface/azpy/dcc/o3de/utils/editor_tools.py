"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

-------------------------------------------------------------------------
DEPRECATION NOTICE: Editor Entity & Editor Component utils are to be considered deprecated
and will be replaced with the EditorPythonBindings libraries once they've relocated from
AutomatedTesting into their own gem (o3de/sig-testing#67
"""
# -------------------------------------------------------------------------


import azlmbr
import azlmbr.bus as bus
import azlmbr.entity as entity
import azlmbr.legacy.general as general
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.asset as asset
import azlmbr.paths as projectPath


def get_property_list(component_id: object) -> str:
    """
    This function will get the component property list, this is used to print a list for development

    :param component_id: This is the Entity Component Object ID
    :returns value: value of property list for component
    """
    # Print the property list of a component
    property_list = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyList', component_id)

    return property_list


def has_component(entity_id: object, type_id: object) -> bool:
    """
    This function will check to see if entity has a component from a component type id

    :param entity_id: This is the Entity Object ID
    :param type_id: This is component type id
    :return: true if the entity has component(s)
    """
    component_bool = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', entity_id, type_id)

    return component_bool


def add_component(entity_id: object, component_name: str):
    """
    This function will add a component.

    :param entity_id:  The entity ID
    :param component_name: The component name
    """
    type_id = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', [component_name], 0)
    component_type_id = type_id[0]
    editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entity_id, [component_type_id])


def get_property(component_id: object, property_path: str) -> str:
    """
    This function will get the component property value

    :param component_id : This is the Entity Component Object ID
    :param property_path: This is the Component Property Path you wish to get a value.
    :return: value of property
    """
    # you can now get or set one of those properties by their strings, such as:
    object_property = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component_id, property_path)

    if (object_property.IsSuccess()):
        value = object_property.GetValue()

        return value
    else:
        return None


def set_property(component_id: object, property_path: str, value: any):
    """
    This function will set the component property

    :param component_id : This is the Entity Component Object ID
    :param property_path: This is the Component Property Path you wish to set a value.
    :return: value of property
    """
    # you can now get or set one of those properties by their strings, such as:
    property_set = editor.EditorComponentAPIBus(bus.Broadcast, 'SetComponentProperty', component_id, property_path,
                                                value)
    assert property_set.IsSuccess(), f"Failure: Unable to set the property {property_path} on {component_id} to {value}"


def get_entity_name(entity_id: object) -> str:
    """
    This function will get a name from and entity id.

    :param entity_id : The entity component object id
    :return: The name of entity
    """
    name = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', entity_id)

    return name


def create_object_from_id(id_int: int) -> object:
    """
    This function will create an object from an id number

    :param id_int : The id number to create the object with
    :return: The created object
    """
    object_id = entity.EntityId(id_int)

    return object_id


def get_parent(child_id: object) -> tuple:
    """
    This function will return the parent's entity id and name from the given ID of the child

    :param child_id : The child entity id to query for the parent
    :return: tuple of the id and name of the parent
    """
    parent_id = editor.EditorEntityInfoRequestBus(bus.Event, 'GetParent', child_id)
    parent_name = get_entity_name(parent_id)

    return parent_id, parent_name


def search_for_entity(entity_name: str) -> object:
    """
    This function will search for an entity

    :param entity_name : Name string: search_filter.names = ['TestEntity']
    :return: The resulting entity result
    """
    search_filter = azlmbr.entity.SearchFilter()
    search_filter.names = [entity_name]
    entity_object_id = azlmbr.entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)

    return entity_object_id


def convert_entity_object_to_id(entity_object_id: object) -> int:
    """
    This function will create an object to an id int

    :param entity_object_id : <EntityComponentIdPair via PythonProxyObject at 2428988078368>
    :return: The id integer
    """
    entity_id = entity_object_id[0]

    return entity_id


def find_component_by_type_id(entity_id: int, component_name: str) -> any:
    """
    This function will find a component by type_id

    :param entity_id: The entity id
    :param component_name: The component name
    :return: The resulting component
    """
    GAME = azlmbr.entity.EntityType().Game
    type_ids = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeIdsByEntityType", component_name, GAME)
    component_type_id = type_ids[0]
    component_outcome = editor.EditorComponentAPIBus(bus.Broadcast, "GetComponentOfType", entity_id, component_type_id)
    component_object_value = component_outcome.GetValue()

    return component_object_value


def get_value_from_property_path(component_object_id: object, property_path: str) -> int:
    """
    This function will get a component value from a property path

    :param component_object_id : The component to look up
    :param property_path: The property path to query
    :return: The resulting value
    """
    entity_outcome = editor.EditorComponentAPIBus(bus.Broadcast, "GetComponentProperty", component_object_id,
                                                  property_path)
    entity_id = entity_outcome.GetValue()

    return entity_id


def get_transforms_from_id(entity_id: object) -> tuple[object, object, object]:
    """
    This function will get a transforms from and entity id.

    :param entity_id : The entity id to get the transforms for
    :return: Translation Vector 3 (x,y,z), Entity Rotation Vector 3 (x,y,x), Entity Uniform Scale Vector 3 (x,y,x)
    """
    local_translation = azlmbr.components.TransformBus(azlmbr.bus.Event, 'GetLocalTranslation', entity_id)
    local_rotation = azlmbr.components.TransformBus(azlmbr.bus.Event, 'GetLocalRotation', entity_id)
    local_uniform_scale = azlmbr.components.TransformBus(azlmbr.bus.Event, 'GetLocalUniformScale',
                                                         entity_id)  # Maybe root we can get GetWorldUniformScale

    return local_translation, local_rotation, local_uniform_scale


def get_urdf_component_type_id(entity_id: object) -> tuple[object, object]:
    """
    This function will find a component types on a giving entity.
    Then build a list of a component type name and type id and return these two list.

    :param entity_id : The entity id
    :return: tuple of the [List of Components Name(s) Type String, List of component ID(s) on an Entity]
    """
    components_on_entity = []
    components_on_entity_id = []
    # Component_type_to_urdf_element_type
    component_types_to_urdf_types = {
        "Mesh":                  "mesh",
        "Material":              "material",
        "PhysX Collider":        "collision",
        "PhysX Rigid Body":      "rigid_body",
        "PhysX Ball Joint":      "continuous",
        "PhysX Hinge Joint":     "revolute",
        "PhysX Fixed Joint":     "fixed",
        "PhysX Prismatic Joint": "prismatic"
    }
    # Get Component Types Dict Keys
    component_types = list(component_types_to_urdf_types.keys())
    # Find Component Type IDs By Entity Type
    component_type_ids = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType',
                                                      component_types, 0)
    if not component_type_ids == None:
        count = 0
        for type_id in component_type_ids:
            # If entity has component
            if editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', entity_id, type_id):
                # Look the type in this count
                component_type = component_types[count]
                # Get the urdf_type string name
                urdf_type = component_types_to_urdf_types[component_type]
                # Lets add this to the lists
                if urdf_type:
                    components_on_entity.append(urdf_type)
                    components_on_entity_id.append(type_id)
            count += 1

    return components_on_entity, components_on_entity_id