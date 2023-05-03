"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
    
# This is a regression test for verifying that toggling certain UI elements properly marks the scene manifest dirty 
def Scene_Settings_Tests_In_Editor_Change_Manifest_Vector_Widget_Child_Marks_File_Dirty():
    import pyside_utils

    @pyside_utils.wrap_async
    async def run_test():
        import asyncio
        from editor_python_test_tools.utils import Report
        import PySide2
        from PySide2 import QtWidgets
        import azlmbr.bus as bus
        import scene_settings_test_messages as tm
        import scene_settings_test_helpers as scene_test_helpers

        path_to_manifest, widget_main_window, reflected_property_root = \
            scene_test_helpers.prepare_scene_ui_for_test( \
                test_file_name="Jack_Death_Fall_Back_ZUp.fbx", \
                manifest_should_exist=True, should_create_manifest=True)
        
        # Find a toggle that used to not properly mark the UI as dirty because it was parented to a manifest vector widget.
        y_axis = reflected_property_root.findChild(QtWidgets.QWidget,"Ignore Y-Axis transition")
        Report.critical_result(tm.Test_Messages.scene_settings_y_axis_row_found, y_axis is not None)

        check_boxes = y_axis.findChildren(QtWidgets.QCheckBox,"")
        Report.critical_result(tm.Test_Messages.scene_settings_y_axis_check_box_found, check_boxes is not None)
        Report.critical_result(tm.Test_Messages.scene_settings_y_axis_check_box_len_one, len(check_boxes) == 1)

        # Click twice so the file saved to disk isn't changed, to minimize processing time.
        check_boxes[0].click()
        check_boxes[0].click()
        
        # Verify the scene is actually marked as dirty
        has_unsaved_changes = azlmbr.qt.SceneSettingsRootDisplayScriptRequestBus(azlmbr.bus.Broadcast, "HasUnsavedChanges")
        Report.critical_result(tm.Test_Messages.scene_settings_update_disabled_on_launch, has_unsaved_changes)
    
        # Save to prevent the unsaved changes popup from occuring
        scene_test_helpers.save_and_verify_manifest(path_to_manifest, widget_main_window)

        widget_main_window.close()
        

    run_test()

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Scene_Settings_Tests_In_Editor_Change_Manifest_Vector_Widget_Child_Marks_File_Dirty)
