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
            06. delete parent entity
    """

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.editor_entity_utils import EditorEntity

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.entity as entity
    import azlmbr.legacy.general as general
    import azlmbr.object

    # 01. load an existing level
    test_level = 'Simple'
    general.open_level_no_prompt(test_level)
    Report.result(Tests.load_level, general.get_current_level_name() == test_level)

    # 02. create parent entity and set name
    # Delete any exiting entity and Create a new Entity at the root level
    search_filter = azlmbr.entity.SearchFilter()
    all_entities = entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", search_filter)
    editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", all_entities)
    parent_entity = EditorEntity.create_editor_entity("Parent_1")
    Report.result(Tests.create_entity, parent_entity.exists())

    # 03. Create child Entity to above created parent entity and set a name
    child_1_entity = EditorEntity.create_editor_entity("Child_1", parent_entity.id )
    Report.result(Tests.create_child_entity, child_1_entity.exists())

    # 04. delete_Child_entity
    child_1_entity.delete()
    Report.result(Tests.delete_child_entity, not child_1_entity.exists())

    # 05. add mesh component to parent entity
    parent_entity.add_component("Mesh")
    Report.result(Tests.add_mesh_component, parent_entity.has_component("Mesh"))

    # 06. delete parent entity
    parent_entity.delete()
    Report.result(Tests.delete_entity, not parent_entity.exists())


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(BasicEditorWorkflows_ExistingLevel_EntityComponentCRUD)
