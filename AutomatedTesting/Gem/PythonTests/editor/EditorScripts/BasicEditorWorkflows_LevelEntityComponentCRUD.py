"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    level_created = (
        "New level created successfully",
        "Failed to create new level"
    )
    new_entity_created = (
        "New entity created successfully",
        "Failed to create a new entity"
    )
    child_entity_created = (
        "New child entity created successfully",
        "Failed to create new child entity"
    )
    component_added = (
        "Component added to entity successfully",
        "Failed to add component to entity"
    )
    component_updated = (
        "Component property updated successfully",
        "Failed to update component property"
    )
    component_removed = (
        "Component removed from entity successfully",
        "Failed to remove component from entity"
    )
    saved_and_exported = (
        "Level saved and exported successfully",
        "Failed to save/export level"
    )


def BasicEditorWorkflows_LevelEntityComponentCRUD():

    import pyside_utils

    @pyside_utils.wrap_async
    async def run_test():
        """
        Summary:
        Open O3DE editor and check if basic Editor workflows are completable.

        Expected Behavior:
        - A new level can be created
        - A new entity can be created
        - Entity hierarchy can be adjusted
        - Components can be added/removed/updated
        - Level can be saved/exported

        Note:
        - This test file must be called from the O3DE Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        import os
        from PySide2 import QtWidgets

        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.entity as entity
        import azlmbr.legacy.general as general
        import azlmbr.math as math
        import azlmbr.paths as paths

        import editor_python_test_tools.hydra_editor_utils as hydra
        from editor_python_test_tools.utils import Report

        def find_entity_by_name(entity_name):
            search_filter = entity.SearchFilter()
            search_filter.names = [entity_name]
            results = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
            if len(results) > 0:
                return results[0]
            return None

        # 1) Create a new level
        lvl_name = "tmp_level"
        editor_window = pyside_utils.get_editor_main_window()

        # The action manager doesn't register the menus until the next system tick, so need to wait
        # until the menu bar has been populated
        general.idle_enable(True)
        await pyside_utils.wait_for_condition(lambda: len(editor_window.menuBar().actions()) > 1)

        new_level_action = pyside_utils.get_action_for_menu_path(editor_window, "File", "New Level")
        pyside_utils.trigger_action_async(new_level_action)
        active_modal_widget = await pyside_utils.wait_for_modal_widget()
        new_level_dlg = active_modal_widget.findChild(QtWidgets.QWidget, "CNewLevelDialog")
        if new_level_dlg:
            if new_level_dlg.windowTitle() == "New Level":
                Report.info("New Level dialog opened")
            grp_box = new_level_dlg.findChild(QtWidgets.QGroupBox, "STATIC_GROUP1")
            level_name = grp_box.findChild(QtWidgets.QLineEdit, "LEVEL")
            level_name.setText(lvl_name)
            button_box = new_level_dlg.findChild(QtWidgets.QDialogButtonBox, "buttonBox")
            button_box.button(QtWidgets.QDialogButtonBox.Ok).click()

        # Verify new level was created successfully
        level_create_success = await pyside_utils.wait_for_condition(lambda: editor.EditorToolsApplicationRequestBus(
            bus.Broadcast, "GetCurrentLevelName") == lvl_name, 5.0)
        Report.critical_result(Tests.level_created, level_create_success)

        # 2) Delete existing entities, and create and manipulate new entities via Entity Inspector
        search_filter = azlmbr.entity.SearchFilter()
        all_entities = entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", search_filter)
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", all_entities)
        entity_outliner_widget = editor_window.findChild(QtWidgets.QWidget, "EntityOutlinerWidgetUI")
        outliner_object_list = entity_outliner_widget.findChild(QtWidgets.QWidget, "m_objectList_Contents")
        outliner_tree = outliner_object_list.findChild(QtWidgets.QWidget, "m_objectTree")
        outliner_viewport = outliner_tree.findChild(QtWidgets.QWidget, "qt_scrollarea_viewport")
        await pyside_utils.trigger_context_menu_entry(outliner_viewport, "Create entity")

        # Find the new entity
        parent_entity_id = find_entity_by_name("Entity1")
        parent_entity_success = await pyside_utils.wait_for_condition(lambda: parent_entity_id is not None, 5.0)
        Report.critical_result(Tests.new_entity_created, parent_entity_success)

        # TODO: Replace Hydra call to creates child entity and add components with context menu triggering - LYN-3951
        # Create a new child entity
        child_entity = hydra.Entity("Child")
        entity_position = math.Vector3(0.0, 0.0, 0.0)
        components_to_add = []
        child_entity.create_entity(entity_position, components_to_add, parent_entity_id)

        # Verify entity hierarchy
        child_entity.get_parent_info()
        Report.result(Tests.child_entity_created, child_entity.parent_id == parent_entity_id)

        # 3) Add/configure a component on an entity
        # Add component and verify success
        child_entity.add_component("Box Shape")
        component_add_success = await pyside_utils.wait_for_condition(lambda: hydra.has_components(child_entity.id,
                                                                                             ["Box Shape"]), 5.0)
        Report.result(Tests.component_added, component_add_success)

        # Update the component
        dimensions_to_set = math.Vector3(16.0, 16.0, 16.0)
        child_entity.get_set_test(0, "Box Shape|Box Configuration|Dimensions", dimensions_to_set)
        box_shape_dimensions = hydra.get_component_property_value(child_entity.components[0],
                                                                  "Box Shape|Box Configuration|Dimensions")
        Report.result(Tests.component_updated, box_shape_dimensions == dimensions_to_set)

        # Remove the component
        child_entity.remove_component("Box Shape")
        component_rem_success = await pyside_utils.wait_for_condition(lambda: not hydra.has_components(child_entity.id,
                                                                                                    ["Box Shape"]), 5.0)
        Report.result(Tests.component_removed, component_rem_success)

        # 4) Save the level
        save_level_action = pyside_utils.get_action_for_menu_path(editor_window, "File", "Save")
        pyside_utils.trigger_action_async(save_level_action)

        # 5) Verify the save/export of the level
        level_prefab_path = os.path.join(paths.products, "levels", lvl_name, f"{lvl_name}.spawnable")
        success = await pyside_utils.wait_for_condition(lambda: os.path.exists(level_prefab_path), 10.0)
        Report.result(Tests.saved_and_exported, success)

    run_test()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(BasicEditorWorkflows_LevelEntityComponentCRUD)
