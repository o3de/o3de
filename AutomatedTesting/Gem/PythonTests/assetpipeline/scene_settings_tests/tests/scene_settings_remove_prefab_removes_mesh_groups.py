"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
    
def Scene_Settings_Tests_Remove_Prefab_Removes_Mesh_Groups():
    import pyside_utils

    @pyside_utils.wrap_async
    async def run_test():
        import asyncio
        from editor_python_test_tools.utils import Report
        import PySide2
        from PySide2 import QtWidgets
        import azlmbr.bus as bus
        import azlmbr.legacy.general as general
        import ly_test_tools.o3de.pipeline_utils as utils
        import scene_settings_test_messages as tm
        import scene_settings_test_helpers as scene_test_helpers
        general.idle_enable(True)
        
        path_to_manifest, widget_main_window, reflected_property_root = \
            scene_test_helpers.prepare_scene_ui_for_test(test_file_name="auto_test_fbx.fbx", manifest_should_exist=False, should_create_manifest=False)

        # First, make sure the mesh groups added by the procedural prefab exists
        name_mesh_labels = widget_main_window.findChildren(QtWidgets.QFrame, "Name mesh")

        editable_mesh_group_name = "auto_test_fbx"
        # The mesh groups generated should be the same every time, otherwise references would break
        # when a scene manifest file didn't exist yet.
        expected_mesh_names = [
            editable_mesh_group_name,
            "default_auto_test_fbx_F0FEDC3F_1BDC_546F_93A9_8DD78321B7B0_",
            "default_auto_test_fbx_2076A10D_F9B1_5F4F_8100_DCEF0AFFB901_",
            "default_auto_test_fbx_C93B7FD9_93B8_5FBD_864E_B63FCBE1803A_"
        ]

        found_mesh_names = []

        for name_mesh_label in name_mesh_labels:
            mesh_name_line_edit = name_mesh_label.findChild(QtWidgets.QLineEdit, "")
            mesh_name = mesh_name_line_edit.text()
            found_mesh_names.append(mesh_name)
            if mesh_name == editable_mesh_group_name:
                Report.critical_result(tm.Test_Messages.scene_settings_base_mesh_group_enabled, mesh_name_line_edit.isEnabled())
            else:
                # All other mesh groups should be read-only
                Report.critical_result(tm.Test_Messages.scene_settings_prefab_mesh_group_disabled, not mesh_name_line_edit.isEnabled())

        expected_diff, found_diff = utils.get_differences_between_lists(expected_mesh_names, found_mesh_names)

        Report.critical_result(tm.Test_Messages.scene_settings_expected_mesh_groups_found, len(expected_diff) == 0 and len(found_diff) == 0)
        
        # Next, go to the prefab tab
        tab_bar = widget_main_window.findChild(QtWidgets.QTabBar,"")
        PREFAB_TAB_INDEX = 3
        tab_bar.setCurrentIndex(PREFAB_TAB_INDEX)

        # Find the default prefab group and delete it
        ui_headers = widget_main_window.findChildren(QtWidgets.QWidget, "AZ__SceneAPI__UI__HeaderWidget")
        for ui_header in ui_headers:
            name_label = ui_header.findChild(QtWidgets.QLabel, "m_nameLabel")
            if name_label.text() == "Prefab group":
                delete_button = ui_header.findChild(QtWidgets.QToolButton, "m_deleteButton")
                delete_button.click()
                break
                
        # Go back to the first tab
        MESH_TAB_INDEX = 0
        tab_bar.setCurrentIndex(MESH_TAB_INDEX)

        # Briefly pause so all events get posted
        general.idle_wait_frames(30)
        
        # Verify the groups are gone now
        
        # Verify the prefab created groups are gone now and the remaining mesh group is the expected non-prefab one.
        # Qt keeps objects around and can throw off searches. To work around that, examine the contents of the labels directly.        
        new_found_mesh_names = []
        name_mesh_labels = widget_main_window.findChildren(QtWidgets.QFrame, "Name mesh")
        for name_mesh_label in name_mesh_labels:
            mesh_name_line_edit = name_mesh_label.findChild(QtWidgets.QLineEdit, "")
            if mesh_name_line_edit != None:
                new_found_mesh_names.append(mesh_name_line_edit.text())
                Report.critical_result(tm.Test_Messages.scene_settings_base_mesh_group_enabled, mesh_name_line_edit.isEnabled())

        Report.critical_result(("", f"new: {new_found_mesh_names}"), len(new_found_mesh_names) == 1)
        Report.critical_result(tm.Test_Messages.scene_settings_expected_mesh_groups_removed, len(new_found_mesh_names) == 1)

        Report.critical_result(tm.Test_Messages.scene_settings_expected_mesh_group_found, editable_mesh_group_name == new_found_mesh_names[0])
        
        scene_test_helpers.save_and_verify_manifest(path_to_manifest, widget_main_window)
        widget_main_window.close()

    run_test()

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Scene_Settings_Tests_Remove_Prefab_Removes_Mesh_Groups)
