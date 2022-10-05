"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def is_this_the_update_materials_checkbox(check_box):
    # The reflected property editor doesn't include names or unique identifiers. The labels are actually different objects.
    # The easiest way to find the button is to go up a few parents to the ancestor with the tooltip, and check that.
    # "Checking this box will accept changes made in the source file into the Open 3D Engine asset."
    check_box_parent_1 = check_box.parent()
    if not check_box_parent_1:
        return None
    check_box_parent_2 = check_box_parent_1.parent()
    if not check_box_parent_2:
        return None
    check_box_parent_3 = check_box_parent_2.parent()
    if not check_box_parent_3:
        return None

    if check_box_parent_3.toolTip() == "<b></b>Checking this box will accept changes made in the source file into the Open 3D Engine asset.":
        return check_box_parent_3
    return None


def Scene_Settings_Tests_In_Editor_Create_And_Verify_Scene_Settings_File():
    import pyside_utils

    @pyside_utils.wrap_async
    async def run_test():
        import asyncio
        from editor_python_test_tools.utils import Report
        import PySide2
        from PySide2 import QtWidgets
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.legacy.general as general
        import scene_settings_test_messages as tm
        import scene_settings_test_helpers as scene_test_helpers
        
        path_to_manifest, widget_main_window, reflected_property_root = \
            scene_test_helpers.prepare_scene_ui_for_test(test_file_name="auto_test_fbx.fbx", manifest_should_exist=False)

        # This test needs to make a change to a setting, so that the update button becomes enabled and the manifest can be saved.
        # The reflected property editor doesn't include names, text or other identifiers on the widgets.
        # The easiest way to verify that a widget is the expected widget, is to search for the widget's type, and then
        # go back up the ancestor list until the tooltip is found for that section.
        # This is searching for the "Update materials" toggle on the default scene settings.
        update_material_checkbox = None

        check_boxes = reflected_property_root.findChildren(QtWidgets.QCheckBox,"")

        # Pyside will automatically clean up the reference to update_material_checkbox if the ancestor 
        # goes out of scope, so hold onto it to keep it in scope, and keep update_material_checkbox active.
        ancestor_with_tooltip = None
        for check_box in check_boxes:
            ancestor_with_toolitip = is_this_the_update_materials_checkbox(check_box)
            if ancestor_with_toolitip is not None:
                update_material_checkbox = check_box
                break
                
        Report.critical_result(tm.Test_Messages.scene_settings_found_update_materials_checkbox, update_material_checkbox is not None)

        # Click the toggle twice to make it so nothing has functionally changed, but this scene settings is now considered dirty
        # and can be saved to disk.
        update_material_checkbox.click()
        update_material_checkbox.click()
        
        # Verify the scene is actually marked as dirty
        has_unsaved_changes = azlmbr.qt.SceneSettingsRootDisplayPythonRequestBus(azlmbr.bus.Broadcast, "HasUnsavedChanges")
        Report.critical_result(tm.Test_Messages.scene_settings_update_disabled_on_launch, has_unsaved_changes)
    
        scene_test_helpers.save_and_verify_manifest(path_to_manifest, widget_main_window)

        widget_main_window.close()

    run_test()
    

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Scene_Settings_Tests_In_Editor_Create_And_Verify_Scene_Settings_File)
