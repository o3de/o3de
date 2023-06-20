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
    
    scene_settings_found_advanced_settings_row = (
        "Found 'use advanced settings' root object.",
        "Failed to find 'use advanced settings' root object."
    )

    scene_settings_found_expected_interface_checkbox = (
        "Found the expected interface element in the scene settings UI.",
        "Unable to find the expected interface element in the scene settings UI."
    )
            
    scene_settings_found_only_one_checkbox = (
        "Found single checkbox object.",
        "Checkbox count is incorrect."
    )
    
    scene_settings_read_only_checked = (
        "Verified checkbox was checked.",
        "Checkbox was not checked."
    )
    
    scene_settings_read_only_disabled = (
        "Verified checkbox was not enabled.",
        "Checkbox was enabled when it was expected to be read-only."
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

    scene_settings_add_button_initially_disabled = (
        "Add procedural prefab group button initially disabled due to existing prefab group.",
        "Add procedural prefab group button not initially disabled."
    )
    
    scene_settings_add_button_enabled = (
        "Add procedural prefab group button enabled correctly when group removed.",
        "Add procedural prefab group button not enabled when group removed."
    )

    scene_settings_add_button_disabled_after_adding = (
        "Add procedural prefab group button disabled correctly when group added.",
        "Add procedural prefab group button not disabled when group added."
    )

    scene_settings_base_mesh_group_enabled = (
        "The base mesh group is enabled.",
        "The base mesh group is disabled, but should be enabled."
    )

    scene_settings_prefab_mesh_group_disabled = (
        "The prefab generated mesh group is disabled.",
        "The prefab generated mesh group is enabled, but should be disabled."
    )
    
    scene_settings_expected_mesh_groups_found = (
        "The expected mesh groups were found.",
        "The expected mesh groups were not found."
    )
    
    scene_settings_expected_mesh_groups_removed = (
        "The expected mesh groups were removed.",
        "The expected mesh groups were not removed."
    )

    scene_settings_expected_mesh_group_found = (
        "The expected mesh group was found.",
        "The expected mesh group was not found."
    )
    
    scene_settings_has_unsaved_changes = (
        "The scene has the expected unsaved changes.",
        "The scene is not registered as having unsaved changes."
    )

    scene_settings_unsaved_changes_cleared = (
        "Clearing unsaved changes worked correctly.",
        "Unsaved changes did not clear correctly as expected, scene appears to still have unsaved changes."
    )
    