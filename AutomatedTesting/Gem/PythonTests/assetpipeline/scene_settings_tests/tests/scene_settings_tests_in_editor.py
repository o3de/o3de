"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def Scene_Settings_Tests_In_Editor_Create_And_Verify_Scene_Settings_File():
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
            scene_test_helpers.prepare_scene_ui_for_test(test_file_name="auto_test_fbx.fbx", manifest_should_exist=False, should_create_manifest=True)

        update_materials_row = reflected_property_root.findChild(QtWidgets.QWidget,"Update materials")
        Report.critical_result(tm.Test_Messages.scene_settings_found_update_materials_row, update_materials_row is not None)

        check_boxes = update_materials_row.findChildren(QtWidgets.QCheckBox,"")
        Report.critical_result(tm.Test_Messages.scene_settings_found_expected_interface_checkbox, check_boxes is not None)
        Report.critical_result(tm.Test_Messages.scene_settings_found_only_one_checkbox, len(check_boxes) == 1)

        update_material_checkbox = check_boxes[0]

        # Click the toggle twice to make it so nothing has functionally changed, but this scene settings is now considered dirty
        # and can be saved to disk.
        update_material_checkbox.click()
        update_material_checkbox.click()
        
        # Verify the scene is actually marked as dirty
        has_unsaved_changes = azlmbr.qt.SceneSettingsRootDisplayScriptRequestBus(azlmbr.bus.Broadcast, "HasUnsavedChanges")
        Report.critical_result(tm.Test_Messages.scene_settings_update_disabled_on_launch, has_unsaved_changes)
    
        scene_test_helpers.save_and_verify_manifest(path_to_manifest, widget_main_window)

        widget_main_window.close()

    run_test()

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Scene_Settings_Tests_In_Editor_Create_And_Verify_Scene_Settings_File)
