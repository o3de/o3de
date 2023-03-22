# Built-in Imports
from __future__ import annotations
from typing import List, Tuple, Union
import math as standard_math
from enum import Enum
import warnings
import uuid
import re

from PySide2 import QtCore
from azpy.dcc.o3de.utils.atom_constants import AtomComponentProperties
from azpy.dcc.o3de.helpers import o3de_cameras
from azpy.dcc.o3de.helpers import o3de_lights
from azpy.dcc.o3de.helpers import o3de_meshes
from azpy.dcc.o3de.helpers import o3de_materials
from pathlib import Path
import importlib
from box import BoxList

# Open 3D Engine Imports
import azlmbr.asset as azasset
import azlmbr.bus as bus
import azlmbr.object
import azlmbr.editor as editor
import azlmbr.components as components
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.atom
import azlmbr.entity
import azlmbr.globals
import azlmbr.paths

importlib.reload(o3de_lights)

# # ===================================================================================================
# # for m in sys.modules:
# #     print(f'--> {m}')
#
#
# # for name, obj in inspect.getmembers(editor_entity_utils):
# #     if inspect.isclass(obj):
# #         print(obj)
#
# # for setting in settings:
# #     print(f'Dyna: {setting}')
# # ===================================================================================================


class O3DELevelBuilder(QtCore.QObject):
    """! Live link control system """

    def __init__(self, **kwargs):
        super(O3DELevelBuilder, self).__init__()

        self.dcc_scene_info = kwargs['audit_info']
        self.up_axis = self.dcc_scene_info.pop('up_axis')
        self.scene_units = self.dcc_scene_info.pop('scene_units')
        self.exported_scene_data = kwargs['export_info']
        self.level_name = kwargs['level_name']
        self.operation = kwargs['operation']
        self.dccsi_base = kwargs['dccsi_base']
        self.asset_base_directory = kwargs['asset_base']
        self.start_index = len(Path(self.asset_base_directory).parts)
        self.entity_info = {}
        self.component_types = {
            'cameras': 'Camera',
            'lights':  'Light',
            'meshes': 'Mesh'
        }

    def start_operation(self):
        if self.operation == 'level-build':
            print(f'LevelName: {self.level_name}')
            print(f'Scene up axis: {self.up_axis}')
            print(f'DCCSceneInfo: {self.dcc_scene_info}')
            print(f'ExportedSceneData: {self.exported_scene_data}')
            self.create_new_level('test_auto_2')
            general.idle_wait_frames(1)
            self.remove_default_objects()
            general.idle_wait_frames(1)
            # self.test_code()
            self.start_level_build()
            return {'msg': 'scene_build_ready', 'result': 'success'}

    def test_code(self):
        new_entity = self.create_new_entity('TestLight')
        entity_id = self.get_entity_by_name('TestLight')

        object_attributes = {
            'type': 'spotLight',
            'position': [-102.21574445529367, 0.0, 0.0],
            'rotation':   [-102.21574445529367, 0.0, 0.0],
            'attributes': {
                'color':         [1.0, 1.0, 1.0],
                'intensity': 4.107142925262451,
                'coneAngle': 40.0,
                'penumbraAngle': 8.571428579411336
            }
        }

        object_type = 'spotLight'
        self.set_position(entity_id, object_attributes)
        print(f'Temp Attributes: {object_attributes}')
        component_list = self.get_component_list('lights', object_attributes)
        print(f'ComponentList: {component_list}')
        for component in component_list:
            component_info = self.set_component(new_entity, component, object_attributes)
            self.set_component_values(object_type, component, component_info)

    def start_level_build(self):
        for object_type, object_listings in self.dcc_scene_info.items():
            print(f'\n\n:::::::::::::::::::::::::::::::::::::::::::::::::::: Object Type: {object_type}')
            object_listings = self.set_export_data(object_listings) if object_type == 'meshes' else object_listings
            # if isinstance(object_listings, dict) and object_type == 'lights':
            if isinstance(object_listings, dict) and object_type != 'materials':
                for object_name, object_attributes in object_listings.items():
                    component_list = self.get_component_list(object_type, object_attributes)
                    print(f'++++++++++ ObjectName: {object_name}   ObjectAttributes: {object_attributes}')
                    new_entity = self.create_new_entity(object_name)
                    entity_id = self.get_entity_by_name(object_name)
                    self.set_position(entity_id, object_attributes)
                    self.entity_info[object_type] = {object_name: {'entity_id': new_entity}}
                    for component in component_list:
                        component, object_attributes = self.get_component_class(component, object_attributes)
                        component_info = self.set_component(new_entity, component, object_attributes)
                        self.set_component_values(object_type, component, component_info)
                        self.entity_info[object_type][object_name][component] = component_info
                    print(f'NewEntityInfo::> {self.entity_info[object_type][object_name]}')

    def get_component_class(self, component, object_attributes):
        if component.find('|') != -1:
            values = component.split('|')
            object_attributes['attributes'].update({'component_class': values[-1]})
            return values[0], object_attributes
        return component, object_attributes

    def create_new_level(self, level_name):
        result = general.create_level_no_prompt(level_name, 1024, 1, 4096, False)
        if result == 1:
            self.set_existing_level(level_name)

    def create_new_entity(self, entity_name):
        try:
            new_entity = EditorEntity.create_editor_entity(entity_name)
            return new_entity
        except Exception as e:
            print(f'Fail [{type(e)}] ::: {e}')

    def remove_default_objects(self):
        search_filter = azlmbr.entity.SearchFilter()
        search_filter.names = ['Shader Ball', 'Ground', 'Camera']
        bus_type = azlmbr.bus.Broadcast
        entity_id_list = azlmbr.entity.SearchBus(bus_type, 'SearchEntities', search_filter)
        for entity_id in entity_id_list:
            azlmbr.editor.ToolsApplicationRequestBus(azlmbr.bus.Broadcast, 'DeleteEntityById', entity_id)

    ####################################
    # Getters/Setters  #################
    ####################################

    def get_component_list(self, object_type, object_attributes):
        component_list = BoxList([self.component_types.get(object_type)])
        if object_type == 'meshes':
            component_list.append('Material')
        if object_type == 'lights':
            light_type = self.get_light_type(object_attributes)
            if light_type == 'directionalLight':
                component_list.pop()
                component_list.append('Directional Light')
            else:
                component_list[0] = component_list[0] + f'|{light_type}'
        return component_list

    def get_asset_path(self, target_component, object_attributes):
        asset_path = None
        if target_component == 'Mesh':
            target_path = Path(object_attributes['fbx'])
            asset_path = Path('/'.join(target_path.parts[self.start_index:])).with_suffix('.azmodel')
        elif target_component == 'Material':
            target_path = Path(object_attributes['material'])
            asset_path = Path('/'.join(target_path.parts[self.start_index:])).with_suffix('.azmaterial')
        elif target_component in ['Camera', 'Light']:
            return asset_path
        return asset_path.as_posix()

    def get_property_tree_list(self):
        pass
        # pteObj = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyTreeEditor', componentId)
        # check_result(pteObj.IsSuccess(), "Created a PropertyTreeEditor for the Mesh component")
        # pte = pteObj.GetValue()
        # that_class.build_paths_list()

    @staticmethod
    def get_entity_ids(entity_id):
        return azlmbr.render.MaterialComponentRequestBus(azlmbr.bus.Event, 'GetDefautMaterialMap', entity_id).keys()

    @staticmethod
    def get_light_type(object_attributes):
        light_type = object_attributes['type']
        if light_type == 'spotLight':
            return 'simple_spot'
        elif light_type == 'pointLight':
            return 'simple_point'
        elif light_type == 'areaLight':
            return 'quad'
        elif light_type == 'directionalLight':
            return 'directionalLight'
        else:
            return None

    @staticmethod
    def get_component_attribute_path(component_type, object_attributes=None):
        if component_type == 'Mesh':
            return AtomComponentProperties.mesh('Model Asset')
        elif component_type == 'Material':
            return AtomComponentProperties.material('Model Materials')
        elif component_type == 'Directional Light':
            return AtomComponentProperties.directional_light('name')

    @staticmethod
    def get_entity_by_name(entity_name):
        search_filter = azlmbr.entity.SearchFilter()
        search_filter.names = [entity_name]
        matching_entities = azlmbr.entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
        if matching_entities:
            return matching_entities[0]
        return None

    @staticmethod
    def get_entity(entity_name):
        search_filter = azlmbr.entity.SearchFilter()
        search_filter.names = [entity_name]
        entity_object_id = azlmbr.entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)[0]
        return entity_object_id

    @staticmethod
    def get_entity_property_list(entity_name):
        property_list = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyList', entity_name)
        return property_list

    def set_position(self, entity_id, object_attributes):
        print(f'SetPosition::: {object_attributes}')
        if 'position' in object_attributes.keys():
            self.set_translation(entity_id, object_attributes['position'])
        if 'rotation' in object_attributes.keys():
            self.set_rotation(entity_id, object_attributes['rotation'])
        if 'scale' in object_attributes.keys():
            self.set_scale(entity_id, object_attributes['scale'])

    def set_export_data(self, object_listings):
        for key, value in object_listings.items():
            try:
                for object_name, export_data in self.exported_scene_data.items():
                    if object_name == key:
                        object_listings[key].update(export_data)
            except Exception as e:
                print(f'Error: [{e}] ... could not find export data for [{key}]... moving on')
        return object_listings

    def get_asset_requirement(self, target_component, object_attributes):
        if target_component in ['Material', 'Mesh']:
            target_asset_path = self.get_asset_path(target_component, object_attributes)
            asset = Asset.find_asset_by_path(target_asset_path, False)
            return asset
        return None

    def set_component(self, target_entity, target_component, object_attributes):
        print(f'+++++++++++++++++++++++++++++++Setting Component [{target_component}] ++++++++++++++++++++++++'
              f'+++++++++++++++++++++++')
        entity_component = target_entity.add_component(target_component)
        general.idle_wait_frames(1)
        component_path = self.get_component_attribute_path(target_component)
        asset_requirement = self.get_asset_requirement(target_component, object_attributes)

        # Get asset path and assign (if needed)
        if target_component == 'Material':
            container_item = entity_component.get_container_item('Model Materials', key=0)
            container_item.SetAssetId(asset_requirement.id)
        elif target_component == 'Mesh':
            entity_component.set_component_property_value(component_path, asset_requirement.id)
        return {'component': entity_component, 'component_path': component_path, 'asset': asset_requirement,
                'attributes': object_attributes['attributes']}

    def set_translation(self, entity_id, data):
        data = [data[0], data[2], data[1]] if self.up_axis == 'y' else data
        components.TransformBus(bus.Event, "SetWorldTranslation", entity_id, math.Vector3(data[0], data[1], data[2]))
        general.idle_wait_frames(1)

    def set_rotation(self, entity_id, data):
        data = [data[0], data[2], data[1]] if self.up_axis == 'y' else data
        rotation = math.Vector3(float(self.get_radians(data[0])), float(self.get_radians(data[1])),
                                float(self.get_radians(data[2])))
        components.TransformBus(bus.Event, "SetLocalRotation", entity_id, rotation)
        general.idle_wait_frames(1)

    def set_scale(self, entity_id, data):
        if self.scene_units == 'cm':
            components.TransformBus(bus.Event, "SetLocalUniformScale", entity_id, 100.0)
            general.idle_wait_frames(1)


    @staticmethod
    def set_existing_level(level_name):
        general.open_level_no_prompt(level_name)

    @staticmethod
    def set_component_values(object_type, component, component_data):
        target_module_dict = {
            'lights': o3de_lights,
            'cameras': o3de_cameras,
            'meshes': o3de_meshes,
            'materials': o3de_materials
        }
        print(f'Firing: {target_module_dict[object_type]}')
        target_module_dict[object_type].set_component_values(component, component_data)

    @classmethod
    def get_radians(cls, degrees):
        return standard_math.radians(degrees)


# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------


def convert_to_azvector3(xyz) -> azlmbr.math.Vector3:
    """
    Converts a vector3-like element into a azlmbr.math.Vector3
    """
    if isinstance(xyz, Tuple) or isinstance(xyz, List):
        assert len(xyz) == 3, ValueError("vector must be a 3 element list/tuple or azlmbr.math.Vector3")
        # return math.Vector3(float(xyz[0]), float(xyz[1]), float(xyz[2]))
        return math.Vector3(float(145), float(20), float(30))
    elif isinstance(xyz, type(math.Vector3())):
        return xyz
    else:
        raise ValueError("vector must be a 3 element list/tuple or azlmbr.math.Vector3")


# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------

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
        type_ids = EditorComponent.get_type_ids(component_names, EditorEntityType.GAME)
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
        general.idle_wait_frames(1)
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
        component_ids = [component.id for component in self.get_components_of_type(component_names)]
        remove_success = editor.EditorComponentAPIBus(bus.Broadcast, "RemoveComponents", component_ids)
        assert (
            remove_success
        ), f"Failure: could not remove component from entity '{self.get_name()}'"

    def get_components_of_type(self, component_names: list) -> List[EditorComponent]:
        """
        Used to get components of type component_name that already exists on Entity
        :param component_names: List of names of components to check
        :return: List of Entity Component objects of given component name
        """
        component_list = []
        type_ids = EditorComponent.get_type_ids(component_names, EditorEntityType.GAME)
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
        type_ids = EditorComponent.get_type_ids([component_name], EditorEntityType.GAME)
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
        # Report.info(f"The start status for {self.get_name} is {status_text}")
        print(f"The start status for {self.get_name} is {status_text}")
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
            print(
                f"Invalid desired_start_status argument for {self.get_name} set_start_status command;\
                Use editor, active, or inactive"
            )
            # Report.info(
            #     f"Invalid desired_start_status argument for {self.get_name} set_start_status command;\
            #     Use editor, active, or inactive"
            # )

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
        general.idle_wait_frames(1)

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

    def get_world_uniform_scale(self) -> float:
        """
        Gets the world uniform scale of the current entity
        """
        return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldUniformScale", self.id)

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

    def validate_world_translate_position(self, expected_translation) -> bool:
        """
        Validates whether the actual world translation of the entity matches the provided translation value.
        :param expected_translation: The math.Vector3 value to compare against the world translation on the entity.
        :return: The bool indicating whether the translate position matched or not.
        """
        is_entity_at_expected_position = self.get_world_translation().IsClose(expected_translation)
        assert is_entity_at_expected_position, \
            f"Translation position of entity '{self.get_name()}' : {self.get_world_translation().ToString()} does not" \
            f" match the expected value : {expected_translation.ToString()}"
        return is_entity_at_expected_position

    # Use this only when prefab system is enabled as it will fail otherwise.
    def focus_on_owning_prefab(self) -> None:
        """
        Focuses on the owning prefab instance of the given entity.
        :param entity: The entity used to fetch the owning prefab to focus on.
        """

        assert self.id.isValid(), "A valid entity id is required to focus on its owning prefab."
        focus_prefab_result = azlmbr.prefab.PrefabFocusPublicRequestBus(bus.Broadcast, "FocusOnOwningPrefab", self.id)
        assert focus_prefab_result.IsSuccess(), f"Prefab operation 'FocusOnOwningPrefab' failed. Error: " \
                                                f"{focus_prefab_result.GetError()}"

# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------


class Asset:
    """
    Used to find Asset Id by its path and path of asset by its Id
    If a component has any asset property, then this class object can be called as:
        asset_id = editor_python_test_tools.editor_entity_utils.EditorComponent.get_component_property_value(<arguments>)
        asset = asset_utils.Asset(asset_id)
    """
    def __init__(self, id: azasset.AssetId):
        self.id: azasset.AssetId = id

    # Creation functions
    @classmethod
    def find_asset_by_path(cls, path: str, RegisterType: bool = False) -> Asset:
        """
        :param path: Absolute file path of the asset
        :param RegisterType: Whether to register the asset if it's not in the database,
                             default to false for the general case
        :return: Asset object associated with file path
        """
        asset_id = azasset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", path, math.Uuid(), RegisterType)
        assert asset_id.is_valid(), f"Couldn't find Asset with path: {path}"
        asset = cls(asset_id)
        return asset

    # Methods
    def get_path(self) -> str:
        """
        :return: Absolute file path of Asset
        """
        assert self.id.is_valid(), "Invalid Asset Id"
        return azasset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetPathById", self.id)


# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------


class EditorEntityType(Enum):
    GAME = azlmbr.entity.EntityType().Game
    LEVEL = azlmbr.entity.EntityType().Level


# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------


class EditorComponent:
    """
    EditorComponent class used to set and get the component property value using path
    EditorComponent object is returned from either of
        EditorEntity.add_component() or Entity.add_components() or EditorEntity.get_components_of_type()
    which also assigns self.id and self.type_id to the EditorComponent object.
    self.type_id is the UUID for the component type as provided by an ebus call.
    self.id is an azlmbr.entity.EntityComponentIdPair which contains both entity and component id's
    """

    def __init__(self, type_id: uuid):
        self.type_id = type_id
        self.id = None
        self.property_tree_editor = None

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
        Used to get and cache the property tree editor of component that has following functions associated with it:
            1. prop_tree.is_container(path)
            2. prop_tree.get_container_count(path)
            3. prop_tree.reset_container(path)
            4. prop_tree.add_container_item(path, key, item)
            5. prop_tree.remove_container_item(path, key)
            6. prop_tree.update_container_item(path, key, value)
            7. prop_tree.get_container_item(path, key)
        :param force_get: Force a fresh property tree editor rather than the cached self.property_tree_editor
        :return: Property tree editor of the component
        """
        if (not force_get) and (self.property_tree_editor is not None):
            return self.property_tree_editor

        build_prop_tree_outcome = editor.EditorComponentAPIBus(
            bus.Broadcast, "BuildComponentPropertyTreeEditor", self.id
        )
        assert (
            build_prop_tree_outcome.IsSuccess()
        ), f"Failure: Could not build property tree editor of component: '{self.get_component_name()}'"
        prop_tree = build_prop_tree_outcome.GetValue()
        self.property_tree_editor = prop_tree
        return self.property_tree_editor

    def get_property_type_visibility(self):
        """
        Used to work with Property Tree Editor build_paths_list_with_types.
        Some component properties have unclear type or return to much information to contain within one Report.info.
        The iterable dictionary can be used to print each property path and type individually with a for loop.
        :return: Dictionary where key is property path and value is a tuple (property az_type, UI visible)
        """
        if self.property_tree_editor is None:
            self.get_property_tree()
        path_type_re = re.compile(r"([ A-z_|]+)(?=\s) \(([A-z0-9:<> ]+),([A-z]+)")
        result = {}
        path_types = self.property_tree_editor.build_paths_list_with_types()
        for path_type in path_types:
            match_line = path_type_re.search(path_type)
            if match_line is None:
                # Report.info(path_type)
                print(path_type)
                continue
            path, az_type, visible = match_line.groups()
            result[path] = (az_type, visible)
        return result

    def is_property_container(self, component_property_path: str) -> bool:
        """
        Used to determine if a component property is a container.
        Containers are a collection of same typed values that can expand/shrink to contain more or less.
        There are two types of containers; indexed and associative.
        Indexed containers use integer key and are something like a linked list
        Associative containers utilize keys of the same type which could be any supported type.
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :return: Boolean True if the property is a container False if it is not.
        """
        if self.property_tree_editor is None:
            self.get_property_tree()
        result = self.property_tree_editor.is_container(component_property_path)
        if not result:
            # Report.info(f"{self.get_component_name()}: '{component_property_path}' is not a container")
            print(f"{self.get_component_name()}: '{component_property_path}' is not a container")
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
        container_count_outcome = self.property_tree_editor.get_container_count(component_property_path)
        assert (
            container_count_outcome.IsSuccess()
        ), f"Failure: get_container_count did not return success for '{component_property_path}'"
        return container_count_outcome.GetValue()

    def reset_container(self, component_property_path: str):
        """
        Used to reset a container to empty
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :return: None
        """
        assert (
            self.is_property_container(component_property_path)
        ), f"Failure: '{component_property_path}' is not a property container"
        reset_outcome = self.property_tree_editor.reset_container(component_property_path)
        assert (
            reset_outcome.IsSuccess()
        ), f"Failure: could not reset_container on '{component_property_path}'"

    def append_container_item(self, component_property_path: str, value: any):
        """
        Used to append a value to an indexed container item without providing an index key.
        Append will fail on an associative container
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :param value: Value to be set
        :return: None
        """
        assert (
            self.is_property_container(component_property_path)
        ), f"Failure: '{component_property_path}' is not a property container"
        append_outcome = self.property_tree_editor.append_container_item(component_property_path, value)
        assert (
            append_outcome.IsSuccess()
        ), f"Failure: could not append_container_item to '{component_property_path}'"

    def add_container_item(self, component_property_path: str, key: any, value: any):
        """
        Used to add a container item at a specified key.
        There are two types of containers; indexed and associative.
        Indexed containers use integer key.
        Associative containers utilize keys of the same type which could be any supported type.
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :param key: Zero index integer key or any supported type for associative container
        :param value: Value to be set
        :return: None
        """
        assert (
            self.is_property_container(component_property_path)
        ), f"Failure: '{component_property_path}' is not a property container"
        add_outcome = self.property_tree_editor.add_container_item(component_property_path, key, value)
        assert (
            add_outcome.IsSuccess()
        ), f"Failure: could not add_container_item '{key}' to '{component_property_path}'"

    def get_container_item(self, component_property_path: str, key: any) -> any:
        """
        Used to retrieve a container item value at the specified key.
        There are two types of containers; indexed and associative.
        Indexed containers use integer key.
        Associative containers utilize keys of the same type which could be any supported type.
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :param key: Zero index integer key or any supported type for associative container
        :return: Value stored at the key specified
        """
        assert (
            self.is_property_container(component_property_path)
        ), f"Failure: '{component_property_path}' is not a property container"
        get_outcome = self.property_tree_editor.get_container_item(component_property_path, key)
        assert (
            get_outcome.IsSuccess()
        ), (
            f"Failure: could not get a value for {self.get_component_name()}: '{component_property_path}' [{key}]. "
            f"Error returned by get_container_item: {get_outcome.GetError()}")
        return get_outcome.GetValue()

    def remove_container_item(self, component_property_path: str, key: any):
        """
        Used to remove a container item value at the specified key.
        There are two types of containers; indexed and associative.
        Indexed containers use integer key.
        Associative containers utilize keys of the same type which could be any supported type.
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :param key: Zero index integer key or any supported type for associative container
        :return: None
        """
        assert (
            self.is_property_container(component_property_path)
        ), f"Failure: '{component_property_path}' is not a property container"
        remove_outcome = self.property_tree_editor.remove_container_item(component_property_path, key)
        assert (
            remove_outcome.IsSuccess()
        ), f"Failure: could not remove_container_item '{key}' from '{component_property_path}'"

    def update_container_item(self, component_property_path: str, key: any, value: any):
        """
        Used to update a container item at a specified key.
        There are two types of containers; indexed and associative.
        Indexed containers use integer key.
        Associative containers utilize keys of the same type which could be any supported type.
        :param component_property_path: String of component property. (e.g. 'Settings|Visible')
        :param key: Zero index integer key or any supported type for associative container
        :param value: Value to be set
        :return: None
        """
        assert (
            self.is_property_container(component_property_path)
        ), f"Failure: '{component_property_path}' is not a property container"
        update_outcome = self.property_tree_editor.update_container_item(component_property_path, key, value)
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
        general.idle_wait_frames(1)
        self.get_property_tree(True)

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
        Deprecation warning! Use set_enabled(False) instead as this method is in deprecation
        :return: None
        """
        warnings.warn("disable_component is deprecated, use set_enabled(False) instead.", DeprecationWarning)
        editor.EditorComponentAPIBus(bus.Broadcast, "DisableComponents", [self.id])

    def remove(self):
        """
        Removes the component from its associated entity. Essentially a delete since only UNDO can return it.
        :return: None
        """
        editor.EditorComponentAPIBus(bus.Broadcast, "RemoveComponents", [self.id])

    @staticmethod
    def get_type_ids(component_names: list, entity_type: EditorEntityType = EditorEntityType.GAME) -> list:
        """
        Used to get type ids of given components list
        :param component_names: List of components to get type ids
        :param entity_type: Entity_Type enum value Entity_Type.GAME is the default
        :return: List of type ids of given components. Type id is a UUID as provided by the ebus call
        """
        type_ids = editor.EditorComponentAPIBus(
            bus.Broadcast, "FindComponentTypeIdsByEntityType", component_names, entity_type.value)
        return type_ids


#
# class EditorLevelEntity:
#     """
#     EditorLevel class used to add and fetch level components.
#     Level entity is a special entity that you do not create/destroy independently of larger systems of level creation.
#     This collects a number of staticmethods that do not rely on entityId since Level entity is found internally by
#     EditorLevelComponentAPIBus requests.
#     """
#
#     @staticmethod
#     def add_component(component_name: str) -> EditorComponent:
#         """
#         Used to add new component to Level.
#         :param component_name: String of component name to add.
#         :return: Component object of newly added component.
#         """
#         component = EditorLevelEntity.add_components([component_name])[0]
#         return component
#
#     @staticmethod
#     def add_components(component_names: list) -> List[EditorComponent]:
#         """
#         Used to add multiple components
#         :param: component_names: List of components to add to level
#         :return: List of newly added components to the level
#         """
#         components = []
#         type_ids = EditorComponent.get_type_ids(component_names, EditorEntityType.LEVEL)
#         for type_id in type_ids:
#             new_comp = EditorComponent(type_id)
#             add_component_outcome = editor.EditorLevelComponentAPIBus(
#                 bus.Broadcast, "AddComponentsOfType", [type_id]
#             )
#             assert (
#                 add_component_outcome.IsSuccess()
#             ), f"Failure: Could not add component: '{new_comp.get_component_name()}' to level"
#             new_comp.id = add_component_outcome.GetValue()[0]
#             components.append(new_comp)
#         return components
#
#     @staticmethod
#     def get_components_of_type(component_names: list) -> List[EditorComponent]:
#         """
#         Used to get components of type component_name that already exists on the level
#         :param component_names: List of names of components to check
#         :return: List of Level Component objects of given component name
#         """
#         component_list = []
#         type_ids = EditorComponent.get_type_ids(component_names, EditorEntityType.LEVEL)
#         for type_id in type_ids:
#             component = EditorComponent(type_id)
#             get_component_of_type_outcome = editor.EditorLevelComponentAPIBus(
#                 bus.Broadcast, "GetComponentOfType", type_id
#             )
#             assert (
#                 get_component_of_type_outcome.IsSuccess()
#             ), f"Failure: Level does not have component:'{component.get_component_name()}'"
#             component.id = get_component_of_type_outcome.GetValue()
#             component_list.append(component)
#
#         return component_list
#
#     @staticmethod
#     def has_component(component_name: str) -> bool:
#         """
#         Used to verify if the level has the specified component
#         :param component_name: Name of component to check for
#         :return: True, if level has specified component. Else, False
#         """
#         type_ids = EditorComponent.get_type_ids([component_name], EditorEntityType.LEVEL)
#         return editor.EditorLevelComponentAPIBus(bus.Broadcast, "HasComponentOfType", type_ids[0])
#
#     @staticmethod
#     def count_components_of_type(component_name: str) -> int:
#         """
#         Used to get a count of the specified level component attached to the level
#         :param component_name: Name of component to check for
#         :return: integer count of occurences of level component attached to level or zero if none are present
#         """
#         type_ids = EditorComponent.get_type_ids([component_name], EditorEntityType.LEVEL)
#         return editor.EditorLevelComponentAPIBus(bus.Broadcast, "CountComponentsOfType", type_ids[0])
#
#
#













# =====================================================================================================================
# if you hit the question mark in the scripting panel it has a searchable help feature that gives some hints!
# ] azlmbr.legacy.general.find_game_entity('Mesh')
# [4294967295]
# ] import azlmbr.entity as entity
# ] entity.EntityId(4294967295)
# [4294967295]

# =====================================================================================================================
# coding:utf-8
# !/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# """
#     Apply a hard-coded material to all meshes in the level
# """
#
# import os
#
# import azlmbr.asset as asset
# import azlmbr.bus as bus
# import azlmbr.entity as entity
# import azlmbr.legacy.general as general
# import azlmbr.math as math
# import azlmbr.editor as editor
# import azlmbr.render as render
#
# ###########################################################################
# # Main Code Block, runs this script as main (testing)
# # -------------------------------------------------------------------------
# if __name__ == '__main__':
#     """Run this file as main"""
#
#     # Get the material we want to assign
#     material_asset_path = os.path.join("materials", "special", "debugvertexstreams.azmaterial")
#     material_asset_id = asset.AssetCatalogRequestBus(
#         bus.Broadcast, "GetAssetIdByPath", material_asset_path, math.Uuid(), False)
#     general.log(f"Assigning {material_asset_path} to all mesh and actor components in the level.")
#
#     type_ids_list = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType',
#                                                  ["Mesh", "Actor", "Material"], 0)
#
#     mesh_component_type_id = type_ids_list[0]
#     actor_component_type_id = type_ids_list[1]
#     material_component_type_id = type_ids_list[2]
#
#     # For each entity
#     search_filter = entity.SearchFilter()
#     all_entities = entity.SearchBus(bus.Broadcast, "SearchEntities", search_filter)
#
#     for current_entity_id in all_entities:
#         # If entity has Mesh or Actor component
#         has_mesh_component = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', current_entity_id,
#                                                           mesh_component_type_id)
#         has_actor_component = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', current_entity_id,
#                                                            actor_component_type_id)
#         if has_mesh_component or has_actor_component:
#             # If entity does not have Material component
#             has_material_component = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType',
#                                                                   current_entity_id, material_component_type_id)
#             if not has_material_component:
#                 # Add material component
#                 editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', current_entity_id,
#                                              [material_component_type_id])
#
#             # Clear all material overrides
#             render.MaterialComponentRequestBus(bus.Event, 'ClearMaterialMap', current_entity_id)
#
#             # Set the default material to the one we want, which will apply it to every slot since we've cleared all overrides
#             render.MaterialComponentRequestBus(bus.Event, 'SetMaterialAssetIdOnDefaultSlot', current_entity_id,
#                                                material_asset_id)
#
# =====================================================================================================================


"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
#
# import azlmbr
# from shiboken2 import wrapInstance, getCppPointer
# from PySide2 import QtCore, QtWidgets, QtGui
# from PySide2.QtCore import QEvent, Qt
# from PySide2.QtWidgets import QAction, QDialog, QHeaderView, QLabel, QLineEdit, QPushButton, QSplitter, QTreeWidget, \
#     QTreeWidgetItem, QWidget, QAbstractButton
#
#
# class OverlayWidget(QWidget):
#     def __init__(self, parent=None):
#         super(OverlayWidget, self).__init__(parent)
#
#         self.setWindowFlags(
#             QtCore.Qt.FramelessWindowHint | QtCore.Qt.Tool | QtCore.Qt.WindowTransparentForInput | QtCore.Qt.WindowDoesNotAcceptFocus)
#         self.setAttribute(QtCore.Qt.WA_TransparentForMouseEvents)
#
#         object_name = "OverlayWidget"
#         self.setObjectName(object_name)
#         self.setStyleSheet("#{name} {{ background-color: rgba(100, 65, 164, 128); }}".format(name=object_name))
#
#     def update_widget(self, hovered_widget):
#         self.resize(hovered_widget.size())
#         self.window().move(hovered_widget.mapToGlobal(QtCore.QPoint(0, 0)))
#
#
# class InspectPopup(QWidget):
#     def __init__(self, parent=None):
#         super(InspectPopup, self).__init__(parent)
#
#         self.setWindowFlags(Qt.Popup)
#
#         object_name = "InspectPopup"
#         self.setObjectName(object_name)
#         self.setStyleSheet("#{name} {{ background-color: #6441A4; }}".format(name=object_name))
#         self.setContentsMargins(10, 10, 10, 10)
#
#         layout = QtWidgets.QGridLayout()
#         self.name_label = QLabel("Name:")
#         self.name_value = QLabel("")
#         layout.addWidget(self.name_label, 0, 0)
#         layout.addWidget(self.name_value, 0, 1)
#
#         self.type_label = QLabel("Type:")
#         self.type_value = QLabel("")
#         layout.addWidget(self.type_label, 1, 0)
#         layout.addWidget(self.type_value, 1, 1)
#
#         self.geometry_label = QLabel("Geometry:")
#         self.geometry_value = QLabel("")
#         layout.addWidget(self.geometry_label, 2, 0)
#         layout.addWidget(self.geometry_value, 2, 1)
#
#         self.setLayout(layout)
#
#     def update_widget(self, new_widget):
#         name = "(None)"
#         type_str = "(Unknown)"
#         geometry_str = "(Unknown)"
#         if new_widget:
#             type_str = str(type(new_widget))
#
#             geometry_rect = new_widget.geometry()
#             geometry_str = "x: {x}, y: {y}, width: {width}, height: {height}".format(x=geometry_rect.x(),
#                                                                                      y=geometry_rect.y(),
#                                                                                      width=geometry_rect.width(),
#                                                                                      height=geometry_rect.height())
#
#             # Not all of our widgets have their objectName set
#             if new_widget.objectName():
#                 name = new_widget.objectName()
#
#         self.name_value.setText(name)
#         self.type_value.setText(type_str)
#         self.geometry_value.setText(geometry_str)
#
#
# class ObjectTreeDialog(QDialog):
#     def __init__(self, parent=None, root_object=None):
#         super(ObjectTreeDialog, self).__init__(parent)
#         self.setWindowTitle("Object Tree")
#
#         layout = QtWidgets.QVBoxLayout()
#
#         # Tree widget for displaying our object hierarchy
#         self.tree_widget = QTreeWidget()
#         self.tree_widget_columns = [
#             "TYPE",
#             "OBJECT NAME",
#             "TEXT",
#             "ICONTEXT",
#             "TITLE",
#             "WINDOW_TITLE",
#             "CLASSES",
#             "POINTER_ADDRESS",
#             "GEOMETRY"
#         ]
#         self.tree_widget.setColumnCount(len(self.tree_widget_columns))
#         self.tree_widget.setHeaderLabels(self.tree_widget_columns)
#
#         # Only show our type and object name columns.  The others we only use to store data so that
#         # we can use the built-in QTreeWidget.findItems to query.
#         for column_name in self.tree_widget_columns:
#             if column_name == "TYPE" or column_name == "OBJECT NAME":
#                 continue
#
#             column_index = self.tree_widget_columns.index(column_name)
#             self.tree_widget.setColumnHidden(column_index, True)
#
#         header = self.tree_widget.header()
#         header.setSectionResizeMode(0, QHeaderView.ResizeToContents)
#         header.setSectionResizeMode(1, QHeaderView.ResizeToContents)
#
#         # Populate our object tree widget
#         # If a root object wasn't specified, then use the Editor main window
#         if not root_object:
#             params = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, "GetQtBootstrapParameters")
#             editor_id = QtWidgets.QWidget.find(params.mainWindowId)
#             editor_main_window = wrapInstance(int(getCppPointer(editor_id)[0]), QtWidgets.QMainWindow)
#             root_object = editor_main_window
#         self.build_tree(root_object, self.tree_widget)
#
#         # Listen for when the tree widget selection changes so we can update
#         # selected item properties
#         self.tree_widget.itemSelectionChanged.connect(self.on_tree_widget_selection_changed)
#
#         # Split our tree widget with a properties view for showing more information about
#         # a selected item. We also use a stacked layout for the properties view so that
#         # when nothing has been selected yet, we can show a message informing the user
#         # that something needs to be selected.
#         splitter = QSplitter()
#         splitter.addWidget(self.tree_widget)
#         self.widget_properties = QWidget(self)
#         self.stacked_layout = QtWidgets.QStackedLayout()
#         self.widget_info = QWidget()
#         form_layout = QtWidgets.QFormLayout()
#         self.name_value = QLineEdit("")
#         self.name_value.setReadOnly(True)
#         self.type_value = QLabel("")
#         self.geometry_value = QLabel("")
#         self.text_value = QLabel("")
#         self.icon_text_value = QLabel("")
#         self.title_value = QLabel("")
#         self.window_title_value = QLabel("")
#         self.classes_value = QLabel("")
#         form_layout.addRow("Name:", self.name_value)
#         form_layout.addRow("Type:", self.type_value)
#         form_layout.addRow("Geometry:", self.geometry_value)
#         form_layout.addRow("Text:", self.text_value)
#         form_layout.addRow("Icon Text:", self.icon_text_value)
#         form_layout.addRow("Title:", self.title_value)
#         form_layout.addRow("Window Title:", self.window_title_value)
#         form_layout.addRow("Classes:", self.classes_value)
#         self.widget_info.setLayout(form_layout)
#
#         self.widget_properties.setLayout(self.stacked_layout)
#         self.stacked_layout.addWidget(QLabel("Select an object to view its properties"))
#         self.stacked_layout.addWidget(self.widget_info)
#         splitter.addWidget(self.widget_properties)
#
#         # Give our splitter stretch factor of 1 so it will expand to take more room over
#         # the footer
#         layout.addWidget(splitter, 1)
#
#         # Create our popup widget for showing information when hovering over widgets
#         self.hovered_widget = None
#         self.inspect_mode = False
#         self.inspect_popup = InspectPopup()
#         self.inspect_popup.resize(100, 50)
#         self.inspect_popup.hide()
#
#         # Create a widget to highlight the extent of the hovered widget
#         self.hover_extent_widget = OverlayWidget()
#         self.hover_extent_widget.hide()
#
#         # Add a footer with a button to switch to widget inspect mode
#         self.footer = QWidget()
#         footer_layout = QtWidgets.QHBoxLayout()
#         self.inspect_button = QPushButton("Pick widget to inspect")
#         self.inspect_button.clicked.connect(self.on_inspect_clicked)
#         footer_layout.addStretch(1)
#         footer_layout.addWidget(self.inspect_button)
#         self.footer.setLayout(footer_layout)
#         layout.addWidget(self.footer)
#
#         self.setLayout(layout)
#
#         # Delete ourselves when the dialog is closed, so that we don't stay living in the background
#         # since we install an event filter on the application
#         self.setAttribute(Qt.WA_DeleteOnClose, True)
#
#         # Listen to events at the application level so we can know when the mouse is moving
#         app = QtWidgets.QApplication.instance()
#         app.installEventFilter(self)
#
#     def eventFilter(self, obj, event):
#         # Look for mouse movement events so we can see what widget the mouse is hovered over
#         event_type = event.type()
#         if event_type == QEvent.MouseMove:
#             global_pos = event.globalPos()
#
#             # Make our popup follow the mouse, but we need to offset it by 1, 1 otherwise
#             # the QApplication.widgetAt will always return our popup instead of the Editor
#             # widget since it is on top
#             self.inspect_popup.move(global_pos + QtCore.QPoint(1, 1))
#
#             # Find out which widget is under our current mouse position
#             hovered_widget = QtWidgets.QApplication.widgetAt(global_pos)
#             if self.hovered_widget:
#                 # Bail out, this is the same widget we are already hovered on
#                 if self.hovered_widget is hovered_widget:
#                     return False
#
#             # Update our hovered widget and label
#             self.hovered_widget = hovered_widget
#             self.update_hovered_widget_popup()
#         elif event_type == QEvent.KeyRelease:
#             if event.key() == Qt.Key_Escape:
#                 # Cancel the inspect mode if the Escape key is pressed
#                 # We don't need to actually hide the inspect popup here because
#                 # it will be hidden already by the Escape action
#                 self.inspect_mode = False
#         elif event_type == QEvent.MouseButtonPress or event_type == QEvent.MouseButtonRelease:
#             # Trigger inspecting the currently hovered widget when the left mouse button is clicked
#             # Don't continue processing this event
#             if self.inspect_mode and event.button() == Qt.LeftButton:
#                 # Only trigger the inspect on the click release, but we want to also eat the press
#                 # event so that the widget we clicked on isn't stuck in a weird state (e.g. thinks its being dragged)
#                 # Also hide the inspect popup since it won't be hidden automatically by the mouse click since we are
#                 # consuming the event
#                 if event_type == event_type == QEvent.MouseButtonRelease:
#                     self.inspect_popup.hide()
#                     self.hover_extent_widget.hide()
#                     self.inspect_widget()
#                 return True
#
#         # Pass every event through
#         return False
#
#     def build_tree(self, obj, parent_tree):
#         if len(obj.children()) == 0:
#             return
#         for child in obj.children():
#             object_type = type(child).__name__
#             if child.metaObject().className() != object_type:
#                 object_type = f"{child.metaObject().className()} ({object_type})"
#             object_name = child.objectName()
#             text = icon_text = title = window_title = geometry_str = classes = "(N/A)"
#             if isinstance(child, QtGui.QWindow):
#                 title = child.title()
#             if isinstance(child, QAction):
#                 text = child.text()
#                 icon_text = child.iconText()
#             if isinstance(child, QWidget):
#                 window_title = child.windowTitle()
#                 if not (child.property("class") == ""):
#                     classes = child.property("class")
#             if isinstance(child, QAbstractButton):
#                 text = child.text()
#
#             # Keep track of the pointer address for this object so we can search for it later
#             pointer_address = str(int(getCppPointer(child)[0]))
#
#             # Some objects might not have a geometry (e.g. actions, generic qobjects)
#             if hasattr(child, 'geometry'):
#                 geometry_rect = child.geometry()
#                 geometry_str = "x: {x}, y: {y}, width: {width}, height: {height}".format(x=geometry_rect.x(),
#                                                                                          y=geometry_rect.y(),
#                                                                                          width=geometry_rect.width(),
#                                                                                          height=geometry_rect.height())
#
#             child_tree = QTreeWidgetItem(
#                 [object_type, object_name, text, icon_text, title, window_title, classes, pointer_address,
#                  geometry_str])
#             if isinstance(parent_tree, QTreeWidget):
#                 parent_tree.addTopLevelItem(child_tree)
#             else:
#                 parent_tree.addChild(child_tree)
#             self.build_tree(child, child_tree)
#
#     def update_hovered_widget_popup(self):
#         if self.inspect_mode and self.hovered_widget:
#             if not self.inspect_popup.isVisible():
#                 self.hover_extent_widget.show()
#                 self.inspect_popup.show()
#
#             self.hover_extent_widget.update_widget(self.hovered_widget)
#             self.inspect_popup.update_widget(self.hovered_widget)
#         else:
#             self.inspect_popup.hide()
#             self.hover_extent_widget.hide()
#
#     def on_inspect_clicked(self):
#         self.inspect_mode = True
#         self.update_hovered_widget_popup()
#
#     def on_tree_widget_selection_changed(self):
#         selected_items = self.tree_widget.selectedItems()
#
#         # If nothing is selected, then switch the stacked layout back to 0
#         # to show the message
#         if not selected_items:
#             self.stacked_layout.setCurrentIndex(0)
#             return
#
#         # Update the selected widget properties and switch to the 1 index in
#         # the stacked layout so that all the rows will be visible
#         item = selected_items[0]
#         self.name_value.setText(item.text(self.tree_widget_columns.index("OBJECT NAME")))
#         self.type_value.setText(item.text(self.tree_widget_columns.index("TYPE")))
#         self.geometry_value.setText(item.text(self.tree_widget_columns.index("GEOMETRY")))
#         self.text_value.setText(item.text(self.tree_widget_columns.index("TEXT")))
#         self.icon_text_value.setText(item.text(self.tree_widget_columns.index("ICONTEXT")))
#         self.title_value.setText(item.text(self.tree_widget_columns.index("TITLE")))
#         self.window_title_value.setText(item.text(self.tree_widget_columns.index("WINDOW_TITLE")))
#         self.classes_value.setText(item.text(self.tree_widget_columns.index("CLASSES")))
#         self.stacked_layout.setCurrentIndex(1)
#
#     def inspect_widget(self):
#         self.inspect_mode = False
#
#         # Find the tree widget item that matches our hovered widget, and then set it as the current item
#         # so that the tree widget will scroll to it, expand it, and select it
#         widget_pointer_address = str(int(getCppPointer(self.hovered_widget)[0]))
#         pointer_address_column = self.tree_widget_columns.index("POINTER_ADDRESS")
#         items = self.tree_widget.findItems(widget_pointer_address, Qt.MatchFixedString | Qt.MatchRecursive,
#                                            pointer_address_column)
#         if items:
#             item = items[0]
#             self.tree_widget.clearSelection()
#             self.tree_widget.setCurrentItem(item)
#         else:
#             print("Unable to find widget")
#
#
# def get_object_tree(parent, obj=None):
#     """
#     Returns the parent/child hierarchy for the given obj (QObject)
#     parent: Parent for the dialog that is created
#     obj: Root object for the tree to be built.
#     returns: QTreeWidget object starting with the root element obj.
#     """
#     w = ObjectTreeDialog(parent, obj)
#     w.resize(1000, 500)
#
#     return w
#
#
# if __name__ == "__main__":
#     # Get our Editor main window
#     params = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, "GetQtBootstrapParameters")
#     editor_id = QtWidgets.QWidget.find(params.mainWindowId)
#     editor_main_window = wrapInstance(int(getCppPointer(editor_id)[0]), QtWidgets.QMainWindow)
#     dock_main_window = editor_main_window.findChild(QtWidgets.QMainWindow)
#
#     # Show our object tree visualizer
#     object_tree = get_object_tree(dock_main_window)
#     object_tree.show()

# material_asset_id = azasset.AssetCatalogRequestBus(
#     bus.Broadcast, "GetAssetIdByPath", target_asset_path, math.Uuid(), False)
# print(f'ContainerID: {container_id}')
# label = container_id.GetLabel()
# print(f'Label: {label}')
# entity_id = self.get_entity_by_name(object_name)
# print(f'EntityID: {entity_id}')
# azlmbr.render.MaterialComponentRequestBus(azlmbr.bus.Event, 'SetMaterialAssetId', entity_id, asset.id)
# asset = Asset.find_asset_by_path(target_asset_path, False)

# entity_component.update_container_item('Model Materials', key=0, value=container_id)

# e = self.get_entity_by_name(object_name)
# print(f"TargetEntity: {target_entity}  GetByName: {e}")
# ids = self.getIds(e)
# print(f'IDs: {ids}')
# base_component = new_entity.add_component(object_component)
# component_path = 'Controller|Configuration|Model Asset'
# asset = Asset.find_asset_by_path()
# base_component.set_component_property_value(component_path, asset.id)
#
# material_component = new_entity.add_component('Material')
# # component_path = 'Default Material|Material Asset'
# component_path = 'Model Materials'
# asset = Asset.find_asset_by_path('test_scene/Assets/Objects/pSphere3/pSphere3_standardSurface1.azmaterial')
# material_component.set_component_property_value(component_path, asset.id)

# try:
        #     entity_name = "test_entity"
        #     entity = self.create_new_entity(entity_name, "Mesh")
        #     # entity = self.get_entity(entity_name)
        #     # entity = EditorEntity.find_editor_entity(entity_name)
        #     # print(f'Entity: {entity}')
        #     # entity_components = entity.components
        #     # print(f'PropertyList: {entity_components}')
        #     # kids = entity.get_children()
        #     # print(f'EntityChildren: {kids}')
        #
        #
        # except Exception as e:
        #     print(f'Failed to create: {e}')

        # create a new entity with a camera component
        # camera_entity_1 = EditorEntity.create_editor_entity(name="CameraEntity1")
        # camera_entity_1.add_component("Camera")
        # scene_elements = self.maya_scene_info['result']['result']['scene_data']

        # for key, values in scene_elements.items():
        #     print(f'Key: {key}   Values: {values}')
        #     for object_name, object_data in values.items():
        #         id = self.create_new_entity(object_name)


# def add_material(self, entity_id, material_location):
#     editor_material_component = azlmbr.entity.EntityUtilityBus(
#         azlmbr.bus.Broadcast,
#         "GetOrAddComponentByTypeName",
#         entity_id,
#         "EditorMaterialComponent"
#     )
#
#     result = azlmbr.entity.EntityUtilityBus(azlmbr.bus.Broadcast, "UpdateComponentForEntity", entity_id,
#                                             editor_material_component, material_location)
#
#     if not result:
#         raise RuntimeError("UpdateComponentForEntity for editor_material_component failed")
