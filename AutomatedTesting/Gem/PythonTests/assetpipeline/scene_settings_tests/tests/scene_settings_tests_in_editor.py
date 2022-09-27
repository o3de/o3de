"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    # Simple sanity check - make sure the test asset actually exists.
    scene_settings_test_asset_exists = (
        "Found the test asset used for this test.",
        "Could not find the test asset needed for this test."
    )
    
    # This test verifies that saving the scene settings creates a new manifest,
    # so it has to first make sure the manifest doesn't exist.
    scene_settings_scene_settings_not_created_yet = (
        "Verified the scene manifest does not yet exist.",
        "The scene manifest for the test asset already exists, when it should not. Make sure the test is running in a clean environment."
    )

    # By default, the scene settings UI disables the button to save changes.
    # The button only becomes enabled when a change is made, even if the change causes no functional difference to the data.
    scene_settings_update_disabled_on_launch = (
        "Scene settings UI update button is disabled on initial load, because the settings haven't been changed yet.",
        "Scene settings UI update button is unexpectedly enabled on launch, when it should be disabled because nothing has changed yet."
    )

    scene_settings_only_one_card_in_layout = (
        "Found the expected scene settings status card for the initial load of the scene settings.",
        "Could not find the status card on launching the scene settings, or found too many status cards."
    )

    # The status card is dismissed after launch to test the logic for closing status cards, and to
    # make sure there aren't any existing status cards for a later step in the test.
    scene_settings_status_card_can_be_clicked = (
        "The scene settings status card is enabled and can be clicked.",
        "The scene settings status card is not enabled, and cannot be clicked."
    )

    scene_setting_card_dismissed_on_click = (
        "Scene settings card was dismissed on click.",
        "Scene settings card was not dismissed on click."
    )

    # This test uses an arbitrary interface element that can be toggled to test saving operations.
    scene_settings_found_update_materials_checkbox = (
        "Found the expected interface element in the scene settings UI.",
        "Unable to find the expected interface element in the scene settings UI."
    )
    
    # Make sure that the save button becomes enabled after making a change to the scene settings.
    scene_settings_file_update_enabled_on_toggle = (
        "Scene settings UI update button is correctly enabled after changing a setting that marks the settings dirty.",
        "Scene settings UI update button is incorrectly not enabled after changing a setting that should have marked these settings dirty."
    )

    # Make sure the UI became responsive after the save finished.
    scene_settings_saved_successfully = (
        "Scene settings UI has refreshed after attempting to save, and is again interactable.",
        "Scene settings UI did not successfully save the file, the UI never became available and is not interactable."
    )

    # Saving the file should mark it as no longer dirty, and the save button should become disabled again.
    scene_settings_file_update_disabled_after_save = (
        "Scene settings UI update button has correctly become disabled after finishing the save operation.",
        "Scene settings UI update button did not become disabled after finishing the save operation."
    )

    # Finally, make sure the scene settings file was actually created on disk.
    scene_settings_file_created = (
        "Scene settings file successfully created.",
        "Failed to create scene settings file."
    )
    

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
    
        project_root_folder = editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'GetGameFolder')
        path_to_test_asset = os.path.join(project_root_folder, "auto_test_fbx.fbx")
        path_to_manifest = path_to_test_asset + ".assetinfo"

        # Make sure the test asset exists
        Report.critical_result(Tests.scene_settings_test_asset_exists, os.path.exists(path_to_test_asset))

        # Make sure the scene settings file doesn't yet exist
        Report.critical_result(Tests.scene_settings_scene_settings_not_created_yet, not os.path.exists(path_to_manifest))

        window_id = azlmbr.qt.SceneSettingsAssetImporterForPythonRequestBus(azlmbr.bus.Broadcast, "EditImportSettings", path_to_test_asset)

        # This test needs to pause between some operations to let asynchronous operations finish.
        general.idle_enable(True)

        # The window doesn't immediately populate when opened, so wait a few frames for it to
        # generate the interface using the reflected property system.
        general.idle_wait_frames(30)

        widget_main_window = QtWidgets.QWidget.find(window_id)
    
        update_button = widget_main_window.findChild(QtWidgets.QPushButton, "m_updateButton")
        
        Report.critical_result(Tests.scene_settings_update_disabled_on_launch, not update_button.isEnabled())
        
        # Close the card generated on opening the scene file, this both tests the close button, and
        # makes it easier to find the processed card later.
        card_layout_area = widget_main_window.findChild(QtWidgets.QWidget, "m_cardAreaLayoutWidget")
        
        # On initial launch of the Scene Settings UI, it will generate one processing event to load the requested file.
        # Find the card for this processing event and close it.
        first_card_push_buttons = card_layout_area.findChildren(QtWidgets.QPushButton,"")
        Report.critical_result(Tests.scene_settings_only_one_card_in_layout, len(first_card_push_buttons) == 1)
        Report.critical_result(Tests.scene_settings_status_card_can_be_clicked, first_card_push_buttons[0].isEnabled())

        # Click the button
        first_card_push_buttons[0].click()
        # Wait a brief period of time after clicking to make sure the status card is removed.
        general.idle_wait_frames(30)
        
        # Verify the button no longer exists
        first_card_push_buttons = card_layout_area.findChildren(QtWidgets.QPushButton,"")
        Report.critical_result(Tests.scene_setting_card_dismissed_on_click, len(first_card_push_buttons) == 0)

        reflected_property_root = widget_main_window.findChild(QtWidgets.QWidget, "m_rootWidget")
    
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
                
        Report.critical_result(Tests.scene_settings_found_update_materials_checkbox, update_material_checkbox is not None)

        # Click the toggle twice to make it so nothing has functionally changed, but this scene settings is now considered dirty
        # and can be saved to disk.
        update_material_checkbox.click()
        update_material_checkbox.click()
        
        Report.critical_result(Tests.scene_settings_file_update_enabled_on_toggle, update_button.isEnabled())
    
        update_button.click()
    
        # All other assets should already be processed, and this is a simple update, so it should be processed quickly.
        # Processing is done when the scene settings card with the save status can be closed.
        time_out_frames_on_scene_settings_updated = 3000
        frames_to_wait_between_checks = 60
        saving_completed = False

        # Wait for the scene file to finish processing.

        # Even though the outer test wrapper running this has a built in timeout,
        # this is done with a custom timeout because it makes it more clear what failed.
        while saving_completed == False and time_out_frames_on_scene_settings_updated > 0:
            first_card_push_buttons = card_layout_area.findChildren(QtWidgets.QPushButton,"")
            if len(first_card_push_buttons) > 0:
                # Once the button exists, and is enabled, the scene settings status card can be dismissed,
                # which means the scene has finished saving.
                # There should only be one card in the list, but on the chance there isn't, the most recent card
                # that this system is waiting on will be at the end of the list.
                if first_card_push_buttons[-1].isEnabled():
                    saving_completed = True
                    break
            time_out_frames_on_scene_settings_updated = time_out_frames_on_scene_settings_updated - frames_to_wait_between_checks
            # idle_wait_frames is used instead of idle_wait because idle_wait was not functional for this test.
            general.idle_wait_frames(frames_to_wait_between_checks)
    
        Report.critical_result(Tests.scene_settings_saved_successfully, saving_completed)
        Report.critical_result(Tests.scene_settings_file_update_disabled_after_save, not update_button.isEnabled())
        
        # Make sure the manifest was actually created
        Report.critical_result(Tests.scene_settings_file_created, os.path.exists(path_to_manifest))

        widget_main_window.close()

    run_test()
    

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Scene_Settings_Tests_In_Editor_Create_And_Verify_Scene_Settings_File)
