"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    load_level = (
        "Level loaded successfully",
        "Failed to load the level"
    )
    create_entity = (
        "Parent entity created successfully",
        "Failed to create a parent entity"
    )
    set_entity_name = (
        "Entity name set successfully",
        "Failed to set entity name"
    )
    delete_entity = (
        "Parent Entity deleted successfully",
        "Failed to delete parent entity"
    )
    create_child_entity = (
        "Child entity created successfully",
        "Failed to create a child entity"
    )
    delete_child_entity = (
        "Child entity deleted successfully",
        "Failed to delete child entity"
    )
    add_mesh_component = (
        "Mesh component added successfully",
        "Failed to add mesh component"
    )
    found_component_typeId = (
        "Found component typeId",
        "Unable to find component TypeId"
    )
    remove_mesh_component = (
        "Mesh component removed successfully",
        "Failed to remove mesh component"
    )


def BasicEditorWorkflows_ExistingLevel_EntityComponentCRUD():
    """
        Performing basic test in editor
            01. Load exiting level
            02. create parent entity and set name
            03. create child entity and set a name
            04. delete child entity
            05. add mesh component to parent entity
            06. remove mesh component
            07. delete parent entity
            Close editor without saving
    """
    import os
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import editor_python_test_tools.hydra_editor_utils as hydra

    import azlmbr.math as math
    import azlmbr.asset as asset
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.entity as entity
    import azlmbr.legacy.general as general
    import azlmbr.object

    def search_entity(entity_to_search, entity_name):
        """

        :param entity_to_search: entity to be searched
        :param entity_name: name of the entity used in the set command
        :return: True if entity id exists in the entity_list
                 False if entity id does not exist in the entity_list
        """
        entity_list = []
        entity_search_filter = entity.SearchFilter()
        entity_search_filter.names = entity_name
        entity_list = entity.SearchBus(bus.Broadcast, 'SearchEntities', entity_search_filter)
        if entity_list:
            if entity_to_search in entity_list:
                return True
            return False
        return False

    # 01. load an existing level

    test_level = 'Simple'
    general.open_level_no_prompt(test_level)
    Report.result(Tests.load_level, general.get_current_level_name() == test_level)

    # 02. create parent entity and set name
    # Delete any exiting entity and Create a new Entity at the root level
    search_filter = azlmbr.entity.SearchFilter()
    all_entities = entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", search_filter)
    editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", all_entities)
    parent_entity = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", entity.EntityId())
    Report.result(Tests.create_entity, parent_entity.IsValid())

    # Setting a new name
    parent_entity_name = "Parent_1"
    editor.EditorEntityAPIBus(bus.Event, 'SetName', parent_entity, parent_entity_name)
    Report.result(Tests.set_entity_name,
                  editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', parent_entity) == parent_entity_name)

    # 03. Create child Entity to above created parent entity and set a name
    child_1_entity = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', parent_entity)
    Report.result(Tests.create_child_entity, child_1_entity.IsValid())
    child_entity_name = "Child_1"
    editor.EditorEntityAPIBus(bus.Event, 'SetName', child_1_entity, child_entity_name)
    Report.result(Tests.set_entity_name,
                  editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', child_1_entity) == child_entity_name)

    # 04. delete_Child_entity
    editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntityById', child_1_entity)
    Report.result(Tests.delete_child_entity, search_entity(child_1_entity, "Child_1") == False)

    # 05. add mesh component to parent entity
    type_id_list = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Mesh"],
                                                entity.EntityType().Game)
    if type_id_list is not None:
        component_outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', parent_entity,
                                                         type_id_list)
        Report.result(Tests.add_mesh_component, component_outcome.IsSuccess())
    else:
        Report.result(Tests.found_component_typeId, type_id_list is not None)

    # 06. remove mesh component
    outcome_get_component = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', parent_entity,
                                                         type_id_list[0])
    if outcome_get_component.IsSuccess():
        component_entity_pair = outcome_get_component.GetValue()
        editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [component_entity_pair])
        component_exists = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', parent_entity,
                                                        type_id_list[0])
        mesh_test = True
        if component_exists:
            mesh_test = False
        Report.result(Tests.remove_mesh_component, mesh_test)
    else:
        Report.result(Tests.found_component_typeId, outcome_get_component.IsSuccess())

    # 7. delete parent entity
    editor.ToolsApplicationRequestBus(azlmbr.bus.Broadcast, 'DeleteEntityById', parent_entity)
    Report.result(Tests.delete_entity, search_entity(parent_entity, "Parent_1") == False)

    # Close editor without saving
    editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(BasicEditorWorkflows_ExistingLevel_EntityComponentCRUD)
