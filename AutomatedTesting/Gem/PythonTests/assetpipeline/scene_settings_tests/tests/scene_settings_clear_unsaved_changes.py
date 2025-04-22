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
        
        path_to_manifest, widget_main_window, reflected_property_root = \
            scene_test_helpers.prepare_scene_ui_for_test(test_file_name="auto_test_fbx.fbx", manifest_should_exist=False, should_create_manifest=False)

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

        update_material_checkbox.click()
        
        has_unsaved_changes = azlmbr.qt.SceneSettingsRootDisplayScriptRequestBus(azlmbr.bus.Broadcast, "HasUnsavedChanges")

        Report.critical_result(tm.Test_Messages.scene_settings_has_unsaved_changes, has_unsaved_changes == True)
                
        clear_unsaved_changes_action = widget_main_window.findChild(QtWidgets.QAction, "m_actionClearUnsavedChanges")
        clear_unsaved_changes_action.trigger()

        # Wait for the UI to finish rebuilding.
        PAUSE_TIME_IN_FRAMES = 30
        general.idle_wait_frames(PAUSE_TIME_IN_FRAMES)
        
        has_unsaved_changes = azlmbr.qt.SceneSettingsRootDisplayScriptRequestBus(azlmbr.bus.Broadcast, "HasUnsavedChanges")

        Report.critical_result(tm.Test_Messages.scene_settings_unsaved_changes_cleared, has_unsaved_changes == False)

        widget_main_window.close()

    run_test()

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Scene_Settings_Clear_Unsaved_Changes)
