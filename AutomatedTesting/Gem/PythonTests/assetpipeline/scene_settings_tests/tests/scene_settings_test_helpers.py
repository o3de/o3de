"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def prepare_scene_ui_for_test(test_file_name, manifest_should_exist, should_create_manifest):
    import asyncio
    from editor_python_test_tools.utils import Report
    import os
    import PySide2
    from PySide2 import QtWidgets
    import azlmbr.bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.qt
    import scene_settings_test_messages as tm
    project_root_folder = editor.EditorToolsApplicationRequestBus(azlmbr.bus.Broadcast, 'GetGameFolder')
    path_to_test_asset = os.path.join(project_root_folder, test_file_name)
    path_to_manifest = path_to_test_asset + ".assetinfo"
    
    # Make sure the test asset exists
    Report.critical_result(tm.Test_Messages.scene_settings_test_asset_exists, os.path.exists(path_to_test_asset))
    
    # Make sure the scene settings file doesn't yet exist
    manifest_expected_error_message = tm.Test_Messages.scene_settings_scene_settings_not_created_yet
    if manifest_should_exist:
        manifest_expected_error_message = tm.Test_Messages.scene_settings_test_asset_manifest_exists
    Report.critical_result(manifest_expected_error_message, os.path.exists(path_to_manifest) == manifest_should_exist)
    window_id = azlmbr.qt.SceneSettingsAssetImporterForScriptRequestBus(azlmbr.bus.Broadcast, "EditImportSettings", path_to_test_asset)
    
    # This test needs to pause between some operations to let asynchronous operations finish.
    general.idle_enable(True)
    
    # The window doesn't immediately populate when opened, so wait a few frames for it to
    # generate the interface using the reflected property system.
    general.idle_wait_frames(30)
    widget_main_window = QtWidgets.QWidget.find(window_id)

    # When the window is first opened, even if it's to a new scene settings file not yet saved to disk, it shouldn't be marked with unsaved changes.
    if should_create_manifest:
        has_unsaved_changes = azlmbr.qt.SceneSettingsRootDisplayScriptRequestBus(azlmbr.bus.Broadcast, "HasUnsavedChanges")
        Report.critical_result(tm.Test_Messages.scene_settings_update_disabled_on_launch, not has_unsaved_changes)

        card_layout_area = widget_main_window.findChild(QtWidgets.QWidget, "m_cardAreaLayoutWidget")
    
        # On initial launch of the Scene Settings UI, it will generate one processing event to load the requested file.
        # Find the card for this processing event and close it.
        first_card_push_buttons = card_layout_area.findChildren(QtWidgets.QPushButton,"")
        Report.critical_result(tm.Test_Messages.scene_settings_only_one_card_in_layout, len(first_card_push_buttons) == 1)
        Report.critical_result(tm.Test_Messages.scene_settings_status_card_can_be_clicked, first_card_push_buttons[0].isEnabled())

        # Click the button
        first_card_push_buttons[0].click()
        # Wait a brief period of time after clicking to make sure the status card is removed.
        general.idle_wait_frames(30)

        # Verify the button no longer exists
        first_card_push_buttons = card_layout_area.findChildren(QtWidgets.QPushButton,"")
        Report.critical_result(tm.Test_Messages.scene_setting_card_dismissed_on_click, len(first_card_push_buttons) == 0)
    
    reflected_property_root = widget_main_window.findChild(QtWidgets.QWidget, "m_rootWidget")

    return path_to_manifest, widget_main_window, reflected_property_root


def save_and_verify_manifest(path_to_manifest, widget_main_window):
    import asyncio
    from editor_python_test_tools.utils import Report
    import os
    import PySide2
    from PySide2 import QtWidgets
    import azlmbr.bus
    import azlmbr.legacy.general as general
    import azlmbr.qt
    import scene_settings_test_messages as tm
    # This test needs to pause between some operations to let asynchronous operations finish.
    general.idle_enable(True)

    save_button = widget_main_window.findChild(QtWidgets.QPushButton, "m_saveButton")
    save_button.click()
    
    # All other assets should already be processed, and this is a simple update, so it should be processed quickly.
    # Processing is done when the scene settings card with the save status can be closed.
    time_out_frames_on_scene_settings_updated = 3000
    frames_to_wait_between_checks = 60
    saving_completed = False

    # Wait for the scene file to finish processing.

    # Even though the outer test wrapper running this has a built in timeout,
    # this is done with a custom timeout because it makes it more clear what failed.
    card_layout_area = widget_main_window.findChild(QtWidgets.QWidget, "m_cardAreaLayoutWidget")
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
    
    Report.critical_result(tm.Test_Messages.scene_settings_saved_successfully, saving_completed)

    # Make sure that the scene settings UI has updated this file to no longer be considered dirty.
    has_unsaved_changes = azlmbr.qt.SceneSettingsRootDisplayScriptRequestBus(azlmbr.bus.Broadcast, "HasUnsavedChanges")
    Report.critical_result(tm.Test_Messages.scene_settings_file_update_disabled_after_save, not has_unsaved_changes)
        
    # Make sure the manifest was actually created
    Report.critical_result(tm.Test_Messages.scene_settings_file_created, os.path.exists(path_to_manifest))
