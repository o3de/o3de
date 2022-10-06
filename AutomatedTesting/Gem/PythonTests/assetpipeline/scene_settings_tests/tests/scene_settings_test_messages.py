"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Test_Messages:
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
    
    # Some tests use an existing manifest and modify it
    scene_settings_test_asset_manifest_exists = (
        "Verified the scene manifest for this test already exists.",
        "Could not find the expected scene manifest for this test."
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
    
    scene_settings_found_update_materials_row = (
        "Found Update materials root object.",
        "Failed to find Update materials root object."
    )

    scene_settings_found_update_materials_checkbox = (
        "Found the expected interface element in the scene settings UI.",
        "Unable to find the expected interface element in the scene settings UI."
    )
    
    scene_settings_found_only_one_update_materials_checkbox = (
        "Found single Update materials checkbox object.",
        "Update materials checkbox count is incorrect."
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

    scene_settings_y_axis_row_found = (
        "Found Ignore Y-Axis Transition root object.",
        "Failed to find Ignore Y-Axis Transition root object."
    )
    
    scene_settings_y_axis_check_box_found = (
        "Found Ignore Y-Axis Transition checkbox object.",
        "Failed to find Ignore Y-Axis Transition checkbox object."
    )
    
    scene_settings_y_axis_check_box_len_one = (
        "Found single Ignore Y-Axis Transition checkbox object.",
        "Ignore Y-Axis Transition checkbox count is incorrect."
    )
    