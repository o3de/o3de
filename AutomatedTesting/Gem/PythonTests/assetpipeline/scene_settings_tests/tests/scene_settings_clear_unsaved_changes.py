"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
    
def Scene_Settings_Clear_Unsaved_Changes():
    import pyside_utils

    @pyside_utils.wrap_async
    async def run_test():
        import asyncio
        from editor_python_test_tools.utils import Report
        import PySide2
        from PySide2 import QtWidgets
        import azlmbr.bus as bus
        import azlmbr.legacy.general as general
        import scene_settings_test_messages as tm
        import scene_settings_test_helpers as scene_test_helpers
        
        path_to_manifest, widget_main_window, reflected_property_root, window_id = \
            scene_test_helpers.prepare_scene_ui_for_test(test_file_name="auto_test_fbx.fbx", manifest_should_exist=False, should_create_manifest=False)

        # Pause a bit longer, the UI has to be rebuilt after clearing unsaved changes.
        PAUSE_TIME_IN_FRAMES = 3000

        # 1. Make a change - Change the update material toggle
        # 2. Verify the change.
        # 3. Revert unsaved changes.
        # 4. Verify the change was reverted.
        
        update_materials_row = reflected_property_root.findChild(QtWidgets.QWidget,"Update materials")
        Report.critical_result(tm.Test_Messages.scene_settings_found_update_materials_row, update_materials_row is not None)
        check_boxes = update_materials_row.findChildren(QtWidgets.QCheckBox,"")
        Report.critical_result(tm.Test_Messages.scene_settings_found_expected_interface_checkbox, check_boxes is not None)
        Report.critical_result(tm.Test_Messages.scene_settings_found_only_one_checkbox, len(check_boxes) == 1)
        
        update_material_checkbox = check_boxes[0]

        initial_check_state = update_material_checkbox.isChecked()
        Report.critical_result(("Initial state is false" , "Initial state is true"), initial_check_state == False)
        
        general.idle_wait_frames(PAUSE_TIME_IN_FRAMES)

        update_material_checkbox.click()
        updated_checkbox_state = update_material_checkbox.isChecked()

        Report.critical_result(tm.Test_Messages.scene_settings_unsaved_toggle_changed, updated_checkbox_state == True)
                
        general.idle_wait_frames(PAUSE_TIME_IN_FRAMES)

        clear_unsaved_changes_action = widget_main_window.findChild(QtWidgets.QAction, "m_actionClearUnsavedChanges")
        clear_unsaved_changes_action.trigger()
        
        # Clear the handles to old objects, so objects refresh correctly.
        clear_unsaved_changes_action = None
        update_material_checkbox = None
        check_boxes = None
        update_materials_row = None
        widget_main_window = None
        
        # Wait for the UI to finish rebuilding.
        general.idle_wait_frames(PAUSE_TIME_IN_FRAMES)
        PySide2.QtCore.QCoreApplication.instance().processEvents()
        
        # If the old widget handles are used, the old values will be returned, so refresh everything manually.
        widget_main_window2 = QtWidgets.QWidget.find(window_id)
        reflected_property_root2 = widget_main_window2.findChild(QtWidgets.QWidget, "m_rootWidget")
        update_materials_row2 = reflected_property_root2.findChild(QtWidgets.QWidget,"Update materials")
        check_boxes2 = update_materials_row2.findChildren(QtWidgets.QCheckBox,"")
        update_material_checkbox2 = check_boxes2[0]
        general.idle_wait_frames(PAUSE_TIME_IN_FRAMES)
        PySide2.QtCore.QCoreApplication.instance().processEvents()
        
        cleared_unsaved_changes_checkbox_state = update_material_checkbox2.isChecked()
        #Report.critical_result(tm.Test_Messages.scene_settings_unsaved_toggle_restored, initial_check_state == cleared_unsaved_changes_checkbox_state)

        Report.critical_result(("GOOD", f"Checked: {update_material_checkbox2.isChecked()}"), cleared_unsaved_changes_checkbox_state == False)

        widget_main_window.close()

    run_test()

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Scene_Settings_Clear_Unsaved_Changes)
