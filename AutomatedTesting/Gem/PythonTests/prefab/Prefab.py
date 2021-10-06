"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from __future__ import annotations
from collections import Counter
from collections import deque
from os import path

from PySide2 import QtWidgets

from azlmbr.entity import EntityId
from azlmbr.math import Vector3
from editor_python_test_tools.editor_entity_utils import EditorEntity
from editor_python_test_tools.utils import Report

import azlmbr.bus as bus
import azlmbr.prefab as prefab
import editor_python_test_tools.pyside_utils as pyside_utils
import prefab.Prefab_Test_Utils as prefab_test_utils

# This is a helper class which contains some of the useful information about a prefab instance.
class PrefabInstance:

    def __init__(self, prefab_file_name: str=None, container_entity: EditorEntity=EntityId()):
        self.prefab_file_name: str = prefab_file_name
        self.container_entity: EditorEntity = container_entity

    def __eq__(self, other):
        return other and self.container_entity.id == other.container_entity.id

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash(self.container_entity.id)

    """
    See if this instance is valid to be used with other prefab operations. 
    :return: Whether the target instance is valid or not.
    """
    def is_valid() -> bool:
        return self.container_entity.id.IsValid() and self.prefab_file_name in Prefab.existing_prefabs

    """
    Reparent this instance to target parent entity. 
    The function will also check pop up dialog ui in editor to see if there's prefab cyclical dependency error while reparenting prefabs.
    :param parent_entity_id: The id of the entity this instance should be a child of in the transform hierarchy next.
    """
    async def ui_reparent_prefab_instance(self, parent_entity_id: EntityId):
        container_entity_id_before_reparent = self.container_entity.id

        original_parent = EditorEntity(self.container_entity.get_parent_id())
        original_parent_before_reparent_children_ids = set(original_parent.get_children_ids())

        new_parent = EditorEntity(parent_entity_id)
        new_parent_before_reparent_children_ids = set(new_parent.get_children_ids())

        pyside_utils.run_soon(lambda: self.container_entity.set_parent_entity(parent_entity_id))
        pyside_utils.run_soon(lambda: prefab_test_utils.wait_for_propagation())

        try:
            active_modal_widget = await pyside_utils.wait_for_modal_widget()
            error_message_box = active_modal_widget.findChild(QtWidgets.QMessageBox)
            ok_button = error_message_box.button(QtWidgets.QMessageBox.Ok)
            ok_button.click()
            assert False, "Cyclical dependency detected while reparenting prefab"
        except pyside_utils.EventLoopTimeoutException:
            pass

        original_parent_after_reparent_children_ids = set(original_parent.get_children_ids())
        assert len(original_parent_after_reparent_children_ids) == len(original_parent_before_reparent_children_ids) - 1, \
            "The children count of the Prefab Instance's original parent should be decreased by 1."
        assert not container_entity_id_before_reparent in original_parent_after_reparent_children_ids, \
            "This Prefab Instance is still a child entity of its original parent entity."
       
        new_parent_after_reparent_children_ids = set(new_parent.get_children_ids())
        assert len(new_parent_after_reparent_children_ids) == len(new_parent_before_reparent_children_ids) + 1, \
            "The children count of the Prefab Instance's new parent should be increased by 1."

        container_entity_id_after_reparent = set(new_parent_after_reparent_children_ids).difference(new_parent_before_reparent_children_ids).pop()
        reparented_container_entity = EditorEntity(container_entity_id_after_reparent)
        reparented_container_entity_parent_id = reparented_container_entity.get_parent_id()
        has_correct_parent = reparented_container_entity_parent_id.ToString() == parent_entity_id.ToString()
        assert has_correct_parent, "Prefab Instance reparented is *not* under the expected parent entity"

        self.container_entity = reparented_container_entity

# This is a helper class which contains some of the useful information about a prefab template.
class Prefab:

    existing_prefabs = {}

    def __init__(self, file_name: str):
        self.file_name:str = file_name
        self.file_path: str = prefab_test_utils.get_prefab_file_path(file_name)
        self.instances: set[PrefabInstance] = set()

    """
    Check if a prefab is ready to be used to generate its instances.
    :param file_name: A unique file name of the target prefab.
    :return: Whether the target prefab is loaded or not.
    """
    @classmethod
    def is_prefab_loaded(cls, file_name: str) -> bool:
        return file_name in Prefab.existing_prefabs

    """
    Check if a prefab exists in the directory for files of prefab tests.
    :param file_name: A unique file name of the target prefab.
    :return: Whether the target prefab exists or not.
    """
    @classmethod
    def prefab_exists(cls, file_name: str) -> bool:
        file_path = prefab_test_utils.get_prefab_file_path(file_name)
        return path.exists(file_path)

    """
    Return a prefab which can be used immediately.
    :param file_name: A unique file name of the target prefab.
    :return: The prefab with given file name.
    """
    @classmethod
    def get_prefab(cls, file_name: str) -> Prefab:
        if Prefab.is_prefab_loaded(file_name):
            return Prefab.existing_prefabs[file_name]
        else:
            assert Prefab.prefab_exists(file_name), f"Attempted to get a prefab {file_name} that doesn't exist"
            new_prefab = Prefab(file_name)
            Prefab.existing_prefabs[file_name] = Prefab(file_name)
            return new_prefab

    """
    Create a prefab in memory and return it. The very first instance of this prefab will also be created.
    :param entities: The entities that should form the new prefab (along with their descendants).
    :param file_name: A unique file name of new prefab.
    :param prefab_instance_name: A name for the very first instance generated while prefab creation. The default instance name is the same as file_name.
    :return: Created Prefab object and the very first PrefabInstance object owned by the prefab.
    """
    @classmethod
    def create_prefab(cls, entities: list[EditorEntity], file_name: str, prefab_instance_name: str=None) -> (Prefab, PrefabInstance):
        assert not Prefab.is_prefab_loaded(file_name), f"Can't create Prefab '{file_name}' since the prefab already exists"

        new_prefab = Prefab(file_name)
        entity_ids = [entity.id for entity in entities]
        create_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'CreatePrefabInMemory', entity_ids, new_prefab.file_path)
        assert create_prefab_result.IsSuccess(), f"Prefab operation 'CreatePrefab' failed. Error: {create_prefab_result.GetError()}"

        container_entity_id = create_prefab_result.GetValue()
        container_entity = EditorEntity(container_entity_id)

        if prefab_instance_name:
            container_entity.set_name(prefab_instance_name)

        prefab_test_utils.wait_for_propagation()

        new_prefab_instance = PrefabInstance(file_name, EditorEntity(container_entity_id))
        new_prefab.instances.add(new_prefab_instance)
        Prefab.existing_prefabs[file_name] = new_prefab
        return new_prefab, new_prefab_instance

    """
    Remove target prefab instances.
    :param prefab_instances: Instances to be removed.
    """
    @classmethod
    def remove_prefabs(cls, prefab_instances: list[PrefabInstance]):
        entity_ids_to_remove = []
        entity_id_queue = [prefab_instance.container_entity for prefab_instance in prefab_instances]
        while entity_id_queue:
            entity = entity_id_queue.pop(0)
            children_entity_ids = entity.get_children_ids()
            for child_entity_id in children_entity_ids:
                entity_id_queue.append(EditorEntity(child_entity_id))

            entity_ids_to_remove.append(entity.id)

        container_entity_ids = [prefab_instance.container_entity.id for prefab_instance in prefab_instances]
        delete_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'DeleteEntitiesAndAllDescendantsInInstance', container_entity_ids)
        assert delete_prefab_result.IsSuccess(), f"Prefab operation 'DeleteEntitiesAndAllDescendantsInInstance' failed. Error: {delete_prefab_result.GetError()}"

        prefab_test_utils.wait_for_propagation()

        entity_ids_after_delete = set(prefab_test_utils.get_all_entities())
        for entity_id_removed in entity_ids_to_remove:
            if entity_id_removed in entity_ids_after_delete:
                assert prefab_entities_deleted, "Not all entities and descendants in target prefabs are deleted."

        for instance in prefab_instances:
            instance_deleted_prefab = Prefab.get_prefab(instance.prefab_file_name)
            instance_deleted_prefab.instances.remove(instance)
            instance = PrefabInstance()
        
    """
    Instantiate an instance of this prefab.
    :param parent_entity: The entity the prefab should be a child of in the transform hierarchy.
    :param name: A name for newly instantiated prefab instance. The default instance name is the same as this prefab's file name.
    :param prefab_position: The position in world space the prefab should be instantiated in.
    :return: Instantiated PrefabInstance object owned by this prefab.
    """
    def instantiate(self, parent_entity: EditorEntity=None, name: str=None, prefab_position: Vector3=Vector3()) -> PrefabInstance:
        parent_entity_id = parent_entity.id if parent_entity is not None else EntityId()

        instantiate_prefab_result = prefab.PrefabPublicRequestBus(
            bus.Broadcast, 'InstantiatePrefab', self.file_path, parent_entity_id, prefab_position)

        assert instantiate_prefab_result.IsSuccess(), f"Prefab operation 'InstantiatePrefab' failed. Error: {instantiate_prefab_result.GetError()}"

        container_entity_id = instantiate_prefab_result.GetValue()
        container_entity = EditorEntity(container_entity_id)

        if name:
            container_entity.set_name(name)

        prefab_test_utils.wait_for_propagation()

        new_prefab_instance = PrefabInstance(self.file_name, EditorEntity(container_entity_id))
        assert not new_prefab_instance in self.instances, "This prefab instance is already existed before this instantiation."
        self.instances.add(new_prefab_instance)

        prefab_test_utils.check_entity_at_position(container_entity_id, prefab_position)

        return new_prefab_instance
