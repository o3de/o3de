"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
    
def Scene_Settings_Tests_In_Editor_ReadOnly_Rule_Works():
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

        # The default prefab adds mesh groups with coordinate rules, that have advanced settings.
        # Find that toggle.
        use_advanced_settings_row = reflected_property_root.findChild(QtWidgets.QWidget,"Use Advanced Settings")
        Report.critical_result(tm.Test_Messages.scene_settings_found_advanced_settings_row, use_advanced_settings_row is not None)

        check_boxes = use_advanced_settings_row.findChildren(QtWidgets.QCheckBox,"")
        Report.critical_result(tm.Test_Messages.scene_settings_found_expected_interface_checkbox, check_boxes is not None)
        Report.critical_result(tm.Test_Messages.scene_settings_found_only_one_checkbox, len(check_boxes) == 1)

        use_advanced_settings_checkbox = check_boxes[0]
        
        # Verify that the toggle is already checked, and is disabled, meaning it is read only.
        Report.critical_result(tm.Test_Messages.scene_settings_read_only_checked, use_advanced_settings_checkbox.isChecked())
        Report.critical_result(tm.Test_Messages.scene_settings_read_only_disabled, not use_advanced_settings_checkbox.isEnabled())
        widget_main_window.close()

    run_test()

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Scene_Settings_Tests_In_Editor_ReadOnly_Rule_Works)
