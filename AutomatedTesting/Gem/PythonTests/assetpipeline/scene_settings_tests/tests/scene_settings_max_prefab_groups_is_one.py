"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def Scene_Settings_Tests_Max_Prefab_Groups_Is_One():
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
            
        tab_bar = widget_main_window.findChild(QtWidgets.QTabBar,"")
        PREFAB_TAB_INDEX = 3
        tab_bar.setCurrentIndex(PREFAB_TAB_INDEX)

        add_buttons = widget_main_window.findChildren(QtWidgets.QPushButton, "m_addButton")
        PREFAB_BUTTON_INDEX = 3
        add_button = add_buttons[PREFAB_BUTTON_INDEX]

        Report.critical_result(tm.Test_Messages.scene_settings_add_button_initially_disabled, add_button.isEnabled() == False)

        # Find the default prefab group and delete it
        ui_headers = widget_main_window.findChildren(QtWidgets.QWidget, "AZ__SceneAPI__UI__HeaderWidget")
        for ui_header in ui_headers:
            name_label = ui_header.findChild(QtWidgets.QLabel, "m_nameLabel")
            if name_label.text() == "Prefab group":
                delete_button = ui_header.findChild(QtWidgets.QToolButton, "m_deleteButton")
                delete_button.click()
                break

        # Briefly pause so all events get posted
        PREFAB_BUTTON_WAIT_TIMEOUT = 30
        await pyside_utils.wait_for_condition(lambda: add_button.isEnabled(), timeout=PREFAB_BUTTON_WAIT_TIMEOUT)
        Report.critical_result(tm.Test_Messages.scene_settings_add_button_enabled, add_button.isEnabled() == True)

        add_button.click()
        # Briefly pause so all events get posted
        await pyside_utils.wait_for_condition(lambda: add_button.isEnabled() == False, timeout=PREFAB_BUTTON_WAIT_TIMEOUT)
        Report.critical_result(tm.Test_Messages.scene_settings_add_button_disabled_after_adding, add_button.isEnabled() == False)
        
        scene_test_helpers.save_and_verify_manifest(path_to_manifest, widget_main_window)
        widget_main_window.close()

    run_test()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Scene_Settings_Tests_Max_Prefab_Groups_Is_One)
