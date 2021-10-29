"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from __future__ import annotations
from collections import Counter
from collections import deque
from os import path
from pathlib import Path

from PySide2 import QtWidgets

import azlmbr.legacy.general as general
from azlmbr.entity import EntityId
from azlmbr.math import Vector3
from editor_python_test_tools.editor_entity_utils import EditorEntity
from editor_python_test_tools.utils import Report

import azlmbr.entity as entity
import azlmbr.bus as bus
import azlmbr.components as components
import azlmbr.editor as editor
import azlmbr.globals
import azlmbr.math as math
import azlmbr.prefab as prefab
import editor_python_test_tools.pyside_utils as pyside_utils


def get_prefab_file_path(prefab_path):
    if not path.isabs(prefab_path):
        prefab_path = path.join(general.get_file_alias("@projectroot@"), prefab_path)
    
    # Append prefab if it doesn't contain .prefab on it
    name, ext = path.splitext(prefab_path)
    if ext != ".prefab":
        prefab_path = name + ".prefab"
    return prefab_path


def get_all_entity_ids():
    return entity.SearchBus(bus.Broadcast, 'SearchEntities', entity.SearchFilter())

def wait_for_propagation():
    general.idle_wait_frames(1)

# This is a helper class which contains some of the useful information about a prefab instance.
class PrefabInstance:

    def __init__(self, prefab_file_name: str = None, container_entity: EditorEntity = None):
        self.prefab_file_name: str = prefab_file_name
        self.container_entity: EditorEntity = container_entity

    def __eq__(self, other):
        return other and self.container_entity.id == other.container_entity.id

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash(self.container_entity.id)

    def is_valid(self) -> bool:
        """
        See if this instance is valid to be used with other prefab operations. 
        :return: Whether the target instance is valid or not.
        """
        return self.container_entity.id.IsValid() and self.prefab_file_name in Prefab.existing_prefabs

    def has_editor_prefab_component(self) -> bool:
        """
        Check if the instance's container entity contains EditorPrefabComponent. 
        :return: Whether the container entity of target instance has EditorPrefabComponent in it or not.
        """
        return editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", self.container_entity.id, azlmbr.globals.property.EditorPrefabComponentTypeId)

    def is_at_position(self, expected_position):
        """
        Check if the instance's container entity is at expected position given. 
        :return: Whether the container entity of target instance is at expected position or not.
        """
        actual_position = components.TransformBus(bus.Event, "GetWorldTranslation", self.container_entity.id)
        is_at_position = actual_position.IsClose(expected_position)

        if not is_at_position:
            Report.info(f"Prefab Instance Container Entity '{self.container_entity.id.ToString()}'\'s expected position: {expected_position.ToString()}, actual position: {actual_position.ToString()}")
        
        return is_at_position

    async def ui_reparent_prefab_instance(self, parent_entity_id: EntityId):
        """
        Reparent this instance to target parent entity. 
        The function will also check pop up dialog ui in editor to see if there's prefab cyclical dependency error while reparenting prefabs.
        :param parent_entity_id: The id of the entity this instance should be a child of in the transform hierarchy next.
        """
        container_entity_id_before_reparent = self.container_entity.id

        original_parent = EditorEntity(self.container_entity.get_parent_id())
        original_parent_before_reparent_children_ids = {child_id.ToString(): child_id for child_id in original_parent.get_children_ids()}

        new_parent = EditorEntity(parent_entity_id)
        new_parent_before_reparent_children_ids = {child_id.ToString(): child_id for child_id in new_parent.get_children_ids()}

        pyside_utils.run_soon(lambda: self.container_entity.set_parent_entity(parent_entity_id))
        pyside_utils.run_soon(lambda: wait_for_propagation())

        try:
            active_modal_widget = await pyside_utils.wait_for_modal_widget()
            error_message_box = active_modal_widget.findChild(QtWidgets.QMessageBox)
            ok_button = error_message_box.button(QtWidgets.QMessageBox.Ok)
            ok_button.click()
            assert False, "Cyclical dependency detected while reparenting prefab"
        except pyside_utils.EventLoopTimeoutException:
            pass

        original_parent_after_reparent_children_ids = {child_id.ToString(): child_id for child_id in original_parent.get_children_ids()}
        assert len(original_parent_after_reparent_children_ids) == len(original_parent_before_reparent_children_ids) - 1, \
            "The children count of the Prefab Instance's original parent should be decreased by 1."
        assert not container_entity_id_before_reparent in original_parent_after_reparent_children_ids, \
            "This Prefab Instance is still a child entity of its original parent entity."
       
        new_parent_after_reparent_children_ids = {child_id.ToString(): child_id for child_id in new_parent.get_children_ids()}
        assert len(new_parent_after_reparent_children_ids) == len(new_parent_before_reparent_children_ids) + 1, \
            "The children count of the Prefab Instance's new parent should be increased by 1."

        after_before_diff = set(new_parent_after_reparent_children_ids.keys()).difference(set(new_parent_before_reparent_children_ids.keys()))
        container_entity_id_after_reparent = new_parent_after_reparent_children_ids[after_before_diff.pop()]
        reparented_container_entity = EditorEntity(container_entity_id_after_reparent)
        reparented_container_entity_parent_id = reparented_container_entity.get_parent_id()
        has_correct_parent = reparented_container_entity_parent_id.ToString() == parent_entity_id.ToString()
        assert has_correct_parent, "Prefab Instance reparented is *not* under the expected parent entity"

        current_instance_prefab = Prefab.get_prefab(self.prefab_file_name)
        current_instance_prefab.instances.remove(self)

        self.container_entity = reparented_container_entity
        current_instance_prefab.instances.add(self)

    def get_direct_child_entities(self):
        """
        Returns the entities only contained in the current prefab instance.
        This function does not return entities contained in other child instances
        """
        return self.container_entity.get_children()


# This is a helper class which contains some of the useful information about a prefab template.
class Prefab:

    existing_prefabs = {}

    def __init__(self, file_path: str):
        self.file_path: str = get_prefab_file_path(file_path)
        self.instances: set[PrefabInstance] = set()

    @classmethod
    def is_prefab_loaded(cls, file_path: str) -> bool:
        """
        Check if a prefab is ready to be used to generate its instances.
        :param file_path: A unique file path of the target prefab.
        :return: Whether the target prefab is loaded or not.
        """
        return file_path in Prefab.existing_prefabs

    
    @classmethod
    def prefab_exists(cls, file_path: str) -> bool:
        """
        Check if a prefab exists in the directory for files of prefab tests.
        :param file_name: A unique file name of the target prefab.
        :return: Whether the target prefab exists or not.
        """
        return path.exists(get_prefab_file_path(file_path))

    @classmethod
    def get_prefab(cls, file_name: str) -> Prefab:
        """
        Return a prefab which can be used immediately.
        :param file_name: A unique file name of the target prefab.
        :return: The prefab with given file name.
        """
        assert file_name, "Received an empty file_name"
        if Prefab.is_prefab_loaded(file_name):
            return Prefab.existing_prefabs[file_name]
        else:
            assert Prefab.prefab_exists(file_name), f"Attempted to get a prefab \"{file_name}\" that doesn't exist"
            new_prefab = Prefab(file_name)
            Prefab.existing_prefabs[file_name] = Prefab(file_name)
            return new_prefab

    @classmethod
    def create_prefab(cls, entities: list[EditorEntity], file_name: str, prefab_instance_name: str=None) -> tuple(Prefab, PrefabInstance):
        """
        Create a prefab in memory and return it. The very first instance of this prefab will also be created.
        :param entities: The entities that should form the new prefab (along with their descendants).
        :param file_name: A unique file name of new prefab.
        :param prefab_instance_name: A name for the very first instance generated while prefab creation. The default instance name is the same as file_name.
        :return: Created Prefab object and the very first PrefabInstance object owned by the prefab.
        """
        assert not Prefab.is_prefab_loaded(file_name), f"Can't create Prefab '{file_name}' since the prefab already exists"

        new_prefab = Prefab(file_name)
        entity_ids = [entity.id for entity in entities]
        create_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'CreatePrefabInMemory', entity_ids, new_prefab.file_path)
        assert create_prefab_result.IsSuccess(), f"Prefab operation 'CreatePrefab' failed. Error: {create_prefab_result.GetError()}"

        container_entity_id = create_prefab_result.GetValue()
        container_entity = EditorEntity(container_entity_id)
        children_entity_ids = container_entity.get_children_ids()

        assert len(children_entity_ids) == len(entities), f"Entity count of created prefab instance does *not* match the count of given entities."

        if prefab_instance_name:
            container_entity.set_name(prefab_instance_name)

        wait_for_propagation()

        new_prefab_instance = PrefabInstance(file_name, EditorEntity(container_entity_id))
        new_prefab.instances.add(new_prefab_instance)
        Prefab.existing_prefabs[file_name] = new_prefab
        return new_prefab, new_prefab_instance

    @classmethod
    def remove_prefabs(cls, prefab_instances: list[PrefabInstance]):
        """
        Remove target prefab instances.
        :param prefab_instances: Instances to be removed.
        """
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

        wait_for_propagation()

        entity_ids_after_delete = set(get_all_entity_ids())
        
        for entity_id_removed in entity_ids_to_remove:
            if entity_id_removed in entity_ids_after_delete:
                assert False, "Not all entities and descendants in target prefabs are deleted."

        for instance in prefab_instances:
            instance_deleted_prefab = Prefab.get_prefab(instance.prefab_file_name)
            instance_deleted_prefab.instances.remove(instance)
            instance = PrefabInstance()

    @classmethod
    def duplicate_prefabs(cls, prefab_instances: list[PrefabInstance]):
        """
        Duplicate target prefab instances.
        :param prefab_instances: Instances to be duplicated.
        :return: PrefabInstance objects of given prefab instances' duplicates.
        """
        assert prefab_instances, "Input list of prefab instances should *not* be empty."

        common_parent = EditorEntity(prefab_instances[0].container_entity.get_parent_id())
        common_parent_children_ids_before_duplicate = set([child_id.ToString() for child_id in common_parent.get_children_ids()])

        container_entity_ids = [prefab_instance.container_entity.id for prefab_instance in prefab_instances]

        duplicate_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'DuplicateEntitiesInInstance', container_entity_ids)
        assert duplicate_prefab_result.IsSuccess(), f"Prefab operation 'DuplicateEntitiesInInstance' failed. Error: {duplicate_prefab_result.GetError()}"

        wait_for_propagation()

        duplicate_container_entity_ids = duplicate_prefab_result.GetValue()
        common_parent_children_ids_after_duplicate = set([child_id.ToString() for child_id in common_parent.get_children_ids()])

        assert set([container_entity_id.ToString() for container_entity_id in container_entity_ids]).issubset(common_parent_children_ids_after_duplicate), \
            "Provided prefab instances are *not* the children of their common parent anymore after duplication."
        assert common_parent_children_ids_before_duplicate.issubset(common_parent_children_ids_after_duplicate), \
            "Some children of provided entities' common parent before duplication are *not* the children of the common parent anymore after duplication."
        assert len(common_parent_children_ids_after_duplicate) == len(common_parent_children_ids_before_duplicate) + len(prefab_instances), \
            "The children count of the given prefab instances' common parent entity is *not* increased to the expected number."
        assert EditorEntity(duplicate_container_entity_ids[0]).get_parent_id().ToString() == common_parent.id.ToString(), \
            "Provided prefab instances' parent should be the same as duplicates' parent."

        duplicate_instances = []
        for duplicate_container_entity_id in duplicate_container_entity_ids:
            prefab_file_path = prefab.PrefabPublicRequestBus(bus.Broadcast, 'GetOwningInstancePrefabPath', duplicate_container_entity_id)
            assert prefab_file_path, "Returned file path should *not* be empty."

            prefab_file_name =  Path(prefab_file_path).stem
            duplicate_instance_prefab = Prefab.get_prefab(prefab_file_name)
            duplicate_instance = PrefabInstance(prefab_file_path, EditorEntity(duplicate_container_entity_id))
            duplicate_instance_prefab.instances.add(duplicate_instance)
            duplicate_instances.append(duplicate_instance)

        return duplicate_instances

    @classmethod
    def detach_prefab(cls, prefab_instance: PrefabInstance):
        """
        Detach target prefab instance.
        :param prefab_instances: Instance to be detached.
        """
        parent = EditorEntity(prefab_instance.container_entity.get_parent_id())
        parent_children_ids_before_detach = set([child_id.ToString() for child_id in parent.get_children_ids()])

        assert prefab_instance.has_editor_prefab_component(), f"Container entity should have EditorPrefabComponent before detachment."

        detach_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'DetachPrefab', prefab_instance.container_entity.id)
        assert detach_prefab_result.IsSuccess(), f"Prefab operation 'DetachPrefab' failed. Error: {detach_prefab_result.GetError()}"

        assert not prefab_instance.has_editor_prefab_component(), f"Container entity should *not* have EditorPrefabComponent after detachment."

        parent_children_ids_after_detach = set([child_id.ToString() for child_id in parent.get_children_ids()])

        assert prefab_instance.container_entity.id.ToString() in parent_children_ids_after_detach, \
            "Target prefab instance's container entity id should still exists after the detachment and before the propagation."

        assert len(parent_children_ids_after_detach) == len(parent_children_ids_before_detach), \
            "Parent entity should still keep the same amount of children entities."

        wait_for_propagation()

        instance_owner_prefab = Prefab.get_prefab(prefab_instance.prefab_file_name)
        instance_owner_prefab.instances.remove(prefab_instance)
        prefab_instance = PrefabInstance()

    def instantiate(self, parent_entity: EditorEntity=None, name: str=None, prefab_position: Vector3=Vector3()) -> PrefabInstance:
        """
        Instantiate an instance of this prefab.
        :param parent_entity: The entity the prefab should be a child of in the transform hierarchy.
        :param name: A name for newly instantiated prefab instance. The default instance name is the same as this prefab's file name.
        :param prefab_position: The position in world space the prefab should be instantiated in.
        :return: Instantiated PrefabInstance object owned by this prefab.
        """
        parent_entity_id = parent_entity.id if parent_entity is not None else EntityId()

        instantiate_prefab_result = prefab.PrefabPublicRequestBus(
            bus.Broadcast, 'InstantiatePrefab', self.file_path, parent_entity_id, prefab_position)

        assert instantiate_prefab_result.IsSuccess(), f"Prefab operation 'InstantiatePrefab' failed. Error: {instantiate_prefab_result.GetError()}"

        container_entity_id = instantiate_prefab_result.GetValue()
        container_entity = EditorEntity(container_entity_id)

        if name:
            container_entity.set_name(name)

        wait_for_propagation()

        new_prefab_instance = PrefabInstance(self.file_path, EditorEntity(container_entity_id))
        assert not new_prefab_instance in self.instances, "This prefab instance is already existed before this instantiation."
        self.instances.add(new_prefab_instance)

        assert new_prefab_instance.is_at_position(prefab_position), "This prefab instance is *not* at expected position."

        return new_prefab_instance
