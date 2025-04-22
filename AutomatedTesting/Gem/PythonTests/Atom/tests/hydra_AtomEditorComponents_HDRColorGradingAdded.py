"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    hdr_color_grading_creation = (
        "HDR Color Grading Entity successfully created",
        "P0: HDR Color Grading Entity failed to be created")
    hdr_color_grading_component = (
        "Entity has an HDR Color Grading component",
        "P0: Entity failed to find HDR Color Grading component")
    hdr_color_grading_component_removal = (
        "HDR Color Grading component successfully removed",
        "P0: HDR Color Grading component failed to be removed")
    removal_undo = (
        "UNDO HDR Color Grading component removal success",
        "P0: UNDO HDR Color Grading component removal failed")
    hdr_color_grading_disabled = (
        "HDR Color Grading component disabled",
        "P0: HDR Color Grading component was not disabled")
    postfx_layer_component = (
        "Entity has a PostFX Layer component",
        "P0: Entity did not have an PostFX Layer component")
    hdr_color_grading_enabled = (
        "HDR Color Grading component enabled",
        "P0: HDR Color Grading component was not enabled")
    toggle_enable_parameter_on = (
        "Enable HDR Color Grading parameter enabled",
        "P0: Enable HDR Color Grading parameter was not enabled")
    toggle_enable_parameter_off = (
        "Enable HDR Color Grading parameter disabled",
        "P0: Enable HDR Color Grading parameter was not disabled")
    color_adjustment_weight_min_value = (
        "Color Adjustment Weight set to minimum value",
        "P1: Color Adjustment Weight failed to be set to minimum value")
    color_adjustment_weight_max_value = (
        "Color Adjustment Weight set to maximum value",
        "P1: Color Adjustment Weight failed to be set to maximum value")
    exposure_low_value = (
        "Exposure set to a low value",
        "P1: Exposure failed to be set to a low value")
    exposure_high_value = (
        "Exposure set to a high value",
        "P1: Exposure failed to be set to a high value")
    contrast_min_value = (
        "Contrast set to minimum value",
        "P1: Contrast failed to be set to minimum value")
    contrast_max_value = (
        "Contrast set to maximum value",
        "P1: Contrast failed to be set to maximum value")
    pre_saturation_min_value = (
        "Pre Saturation set to minimum value",
        "P1: Pre Saturation failed to be set to minimum value")
    pre_saturation_max_value = (
        "Pre Saturation set to maximum value",
        "P1: Pre Saturation failed to be set to maximum value")
    filter_intensity_low_value = (
        "Filter Intensity set to a low value",
        "P1: Filter Intensity failed to be set to a low value")
    filter_intensity_high_value = (
        "Filter Intensity set to a high value",
        "P1: Filter Intensity failed to be set to a high value")
    filter_multiply_min_value = (
        "Filter Multiply set to minimum value",
        "P1: Filter Multiply failed to be set to minimum value")
    filter_multiply_max_value = (
        "Filter Multiply set to maximum value",
        "P1: Filter Multiply failed to be set to maximum value")
    edit_filter_swatch_color = (
        "Filter Swatch parameter updated",
        "P1: Filter Swatch parameter failed to update")
    white_balance_weight_max_value = (
        "White Balance Weight set to maximum value",
        "P1: White Balance Weight failed to be set to maximum value")
    temperature_min_value = (
        "Temperature set to minimum value",
        "P1: Temperature failed to be set to minimum value")
    temperature_max_value = (
        "Temperature set to maximum value",
        "P1: Temperature failed to be set to maximum value")
    tint_min_value = (
        "Tint set to minimum value",
        "P1: Tint failed to be set to minimum value")
    tint_max_value = (
        "Tint set to maximum value",
        "P1: Tint failed to be set to maximum value")
    luminance_preservation_min_value = (
        "Luminance Preservation set to minimum value",
        "P1: Luminance Preservation failed to be set to minimum value")
    luminance_preservation_max_value = (
        "Luminance Preservation set to maximum value",
        "P1: Luminance Preservation failed to be set to maximum value")
    split_toning_weight_max_value = (
        "Split Toning Weight set to maximum value",
        "P1: Split Toning Weight failed to be set to maximum value")
    balance_min_value = (
        "Balance set to minimum value",
        "P1: Balance failed to be set to minimum value")
    balance_max_value = (
        "Balance set to maximum value",
        "P1: Balance maximum to be set to minimum value")
    edit_split_toning_shadow_color = (
        "Split Toning Shadows Color parameter updated",
        "P1: Split Toning Shadows Color parameter failed to update")
    edit_split_toning_highlights_color = (
        "Split Toning Highlights Color parameter updated",
        "P1: Split Toning Highlights Color parameter failed to update")
    smh_weight_max_value = (
        "SMH Weight set to maximum value",
        "P1: SMH Weight failed to be set to maximum value")
    shadows_start_min_value = (
        "Shadows Start set to minimum value",
        "P1: Shadows Start failed to be set to minimum value")
    shadows_start_max_value = (
        "Shadows Start set to maximum value",
        "P1: Shadows Start failed to be set to maximum value")
    shadows_end_min_value = (
        "Shadows End set to minimum value",
        "P1: Shadows End failed to be set to minimum value")
    shadows_end_max_value = (
        "Shadows End set to maximum value",
        "P1: Shadows End failed to be set to maximum value")
    highlights_start_min_value = (
        "Highlights Start set to minimum value",
        "P1: Highlights Start failed to be set to minimum value")
    highlights_start_max_value = (
        "Highlights Start set to maximum value",
        "P1: Highlights Start failed to be set to maximum value")
    highlights_end_min_value = (
        "Highlights End set to minimum value",
        "P1: Highlights End failed to be set to minimum value")
    highlights_end_max_value = (
        "Highlights End set to maximum value",
        "P1: Highlights End failed to be set to maximum value")
    edit_smh_shadows_color = (
        "SMH Shadows Color parameter updated",
        "P1: SMH Shadows Color parameter failed to update")
    edit_smh_midtones_color = (
        "SMH Midtones Color parameter updated",
        "P1: SMH Midtones Color parameter failed to update")
    edit_smh_highlights_color = (
        "SMH Highlights Color parameter updated",
        "P1: SMH Highlights Color parameter failed to update")
    edit_channel_mixing_red = (
        "Channel Mixing Red parameter updated",
        "P1: Channel Mixing Red parameter failed to update")
    edit_channel_mixing_green = (
        "Channel Mixing Green parameter updated",
        "P1: Channel Mixing Green parameter failed to update")
    edit_channel_mixing_blue = (
        "Channel Mixing Blue parameter updated",
        "P1: Channel Mixing Blue parameter failed to update")
    final_adjustment_weight_min_value = (
        "Final Adjustment Weight set to minimum value",
        "P1: Final Adjustment Weight failed to be set to minimum value")
    final_adjustment_weight_max_value = (
        "Final Adjustment Weight set to maximum value",
        "P1: Final Adjustment Weight failed to be set to maximum value")
    post_saturation_min_value = (
        "Post Saturation set to minimum value",
        "P1: Post Saturation failed to be set to minimum value")
    post_saturation_max_value = (
        "Post Saturation set to maximum value",
        "P1: Post Saturation failed to be set to maximum value")
    hue_shift_max_value = (
        "Hue Shift set to maximum value",
        "P1: Hue Shift failed to be set to maximum value")
    enter_game_mode = (
        "Entered game mode",
        "Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "REDO deletion failed")


def AtomEditorComponents_HDRColorGrading_AddedToEntity():
    """
    Summary:
    Tests the HDR Color Grading component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an HDR Color Grading entity with no components.
    2) Add HDR Color Grading component to HDR Color Grading entity.
    3) Remove the Color Grading component.
    4) Undo Color Grading component removal.
    5) Verify HDR Color Grading component not enabled.
    6) Add PostFX Layer component since it is required by the HDR Color Grading component.
    7) Verify HDR Color Grading component is enabled.
    8) Toggle "Enable HDR Color Grading" (default False)
    9) Color Adjustment|Weight (float range 0.0 to 1.0, default 1.0)
    10) Exposure (soft cap float range -20.0 to 20.0, actual range -INF to INF)
    11) Contrast (float range -100.0 to 100.0)
    12) Pre Saturation (float range -100.0 to 100.0)
    13) Filter Intensity (soft cap float range -1.0 to 1.0, actual range -INF to INF)
    14) Filter Multiply (float range -1.0 to 1.0)
    15) Filter Swatch (Vector3 range 0.0 to 1.0)
    16) White Balance|Weight (float range 0.0 to 1.0, default 0.0)
    17) Temperature (float range 1000.0 to 40000.0)
    18) Tint (float range -100.0 to 100.0)
    19) Luminance Preservation (float range -1.0 to 1.0)
    20) Split Toning|Weight (float range 0.0 to 1.0, default 0.0)
    21) Balance (float range -1.0 to 1.0)
    22) Split Toning|Shadows Color (Vector3 range 0.0 to 1.0)
    23) Split Toning|Highlights Color (Vector3 range 0.0 to 1.0)
    24) Update the SMH Weight parameter (float range 0.0 to 1.0, default 0.0)
    25) Shadows Start (float range 0.0 to 16.0)
    26) Shadows End (float range 0.0 to 16.0)
    27) Highlights Start (float range 0.0 to 16.0)
    28) Highlights End (float range 0.0 to 16.0)
    29) Shadows Midtones Highlights|Shadows Color (Vector3 range 0.0 to 1.0)
    30) Shadows Midtones Highlights|Midtones Color (Vector3 range 0.0 to 1.0)
    31) Shadows Midtones Highlights|Highlights Color (Vector3 range 0.0 to 1.0)
    32) Channel Mixing Red (Vector3 range 0.0 to 1.0)
    33) Channel Mixing Green (Vector3 range 0.0 to 1.0)
    34) Channel Mixing Blue (Vector3 range 0.0 to 1.0)
    35) Final Adjustment|Weight (float range 0.0 to 1.0, default 1.0)
    36) Post Saturation (float range -100.0 to 100.0)
    37) Hue Shift (float range 0.0 to 1.0)
    38) LUT Resolution (from atom_constants.py LUT_RESOLUTION, default 16x16x16)
    39) Set the Shaper Type parameter (from atom_constants.py SHAPER_TYPE, default none)
    40) Enter/Exit game mode.
    41) Test IsHidden.
    42) Test IsVisible.
    43) Delete HDR Color Grading entity.
    44) UNDO deletion.
    45) REDO deletion.
    46) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties, LUT_RESOLUTION, SHAPER_TYPE

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create an HDR Color Grading entity with no components.
        hdr_color_grading_entity = EditorEntity.create_editor_entity(AtomComponentProperties.hdr_color_grading())
        Report.critical_result(Tests.hdr_color_grading_creation, hdr_color_grading_entity.exists())

        # 2. Add HDR Color Grading component to HDR Color Grading entity.
        hdr_color_grading_component = hdr_color_grading_entity.add_component(
            AtomComponentProperties.hdr_color_grading())
        Report.critical_result(
            Tests.hdr_color_grading_component,
            hdr_color_grading_entity.has_component(AtomComponentProperties.hdr_color_grading()))

        # 3. Remove the HDR Color Grading component.
        hdr_color_grading_component.remove()
        general.idle_wait_frames(1)
        Report.result(Tests.hdr_color_grading_component_removal,
                      not hdr_color_grading_entity.has_component(AtomComponentProperties.hdr_color_grading()))

        # 4. Undo HDR Color Grading component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.removal_undo,
                      hdr_color_grading_entity.has_component(AtomComponentProperties.hdr_color_grading()))

        # 5. Verify HDR Color Grading component not enabled.
        Report.result(Tests.hdr_color_grading_disabled, not hdr_color_grading_component.is_enabled())

        # 6. Add PostFX Layer component since it is required by the HDR Color Grading component.
        hdr_color_grading_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(
            Tests.postfx_layer_component,
            hdr_color_grading_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify HDR Color Grading component is enabled.
        Report.result(Tests.hdr_color_grading_enabled, hdr_color_grading_component.is_enabled())

        # 8. Toggle "Enable HDR Color Grading".
        # Toggle "Enable HDR Color Grading" on.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Enable HDR color grading'), True)
        Report.result(Tests.toggle_enable_parameter_on,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Enable HDR color grading')) is True)

        # Toggle "Enable HDR Color Grading" off.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Enable HDR color grading'), False)
        Report.result(Tests.toggle_enable_parameter_off,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Enable HDR color grading')) is False)

        # Toggle "Enable HDR Color Grading" back on for testing.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Enable HDR color grading'), True)


        # 9. Color Adjustment|Weight (float range 0.0 to 1.0, default 1.0)
        # Set Color Adjustment|Weight to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Color Adjustment Weight'), 0.0)
        Report.result(Tests.color_adjustment_weight_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Color Adjustment Weight')) == 0.0)

        # Set Color Adjustment|Weight to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Color Adjustment Weight'), 1.0)
        Report.result(Tests.color_adjustment_weight_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Color Adjustment Weight')) == 1.0)

        # 10. Exposure (soft cap float range -20.0 to 20.0, actual range -INF to INF)
        # Set Exposure to a low value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Exposure'), -20.0)
        Report.result(Tests.exposure_low_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Exposure')) == -20.0)

        # Set Exposure to a high value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Exposure'), 20.0)
        Report.result(Tests.exposure_high_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Exposure')) == 20.0)

        # 11. Contrast (float range -100.0 to 100.0)
        # Set Contrast to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Contrast'), -100.0)
        Report.result(Tests.contrast_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Contrast')) == -100.0)

        # Set Contrast to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Contrast'), 100.0)
        Report.result(Tests.contrast_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Contrast')) == 100.0)

        # 12. Pre Saturation (float range -100.0 to 100.0)
        # Set Pre Saturation to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Pre Saturation'), -100.0)
        Report.result(Tests.pre_saturation_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Pre Saturation')) == -100.0)

        # Set Pre Saturation to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Pre Saturation'), 100.0)
        Report.result(Tests.pre_saturation_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Pre Saturation')) == 100.0)

        # 13. Filter Intensity (soft cap float range -1.0 to 1.0, actual range -INF to INF)
        # Set Filter Intensity to a low value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Filter Intensity'), -1.0)
        Report.result(Tests.filter_intensity_low_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Filter Intensity')) == -1.0)

        # Set Filter Intensity to a high value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Filter Intensity'), 1.0)
        Report.result(Tests.filter_intensity_high_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Filter Intensity')) == 1.0)

        # 14. Filter Multiply (float range -1.0 to 1.0)
        # Set Filter Multiply to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Filter Multiply'), -1.0)
        Report.result(Tests.filter_multiply_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Filter Multiply')) == -1.0)

        # Set Filter Multiply to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Filter Multiply'), 1.0)
        Report.result(Tests.filter_multiply_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Filter Multiply')) == 1.0)

        # 15. Filter Swatch (Vector3 range 0.0 to 1.0)
        # Adjust Filter Swatch color.
        violet_color = math.Vector3(0.498, 0.0, 1.0)
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Filter Swatch'), violet_color)
        filter_color_value = hdr_color_grading_component.get_component_property_value(
            AtomComponentProperties.hdr_color_grading('Filter Swatch'))
        Report.result(Tests.edit_filter_swatch_color, filter_color_value.IsClose(violet_color))

        # 16. White Balance|Weight (float range 0.0 to 1.0, default 0.0)
        # Set White Balance to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('White Balance Weight'), 1.0)
        Report.result(Tests.white_balance_weight_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('White Balance Weight')) == 1.0)

        # 17. Temperature (float range 1000.0 to 40000.0)
        # Set Temperature to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Temperature'), 1000.0)
        Report.result(Tests.temperature_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Temperature')) == 1000.0)

        # Set Temperature to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Temperature'), 40000.0)
        Report.result(Tests.temperature_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Temperature')) == 40000.0)

        # 18. Tint (float range -100.0 to 100.0)
        # Set Tint to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Tint'), -100.0)
        Report.result(Tests.tint_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Tint')) == -100.0)

        # Set Tint to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Tint'), 100.0)
        Report.result(Tests.tint_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Tint')) == 100.0)

        # 19. Luminance Preservation (float range -1.0 to 1.0)
        # Set Luminance Preservation to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Luminance Preservation'), -1.0)
        Report.result(Tests.luminance_preservation_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Luminance Preservation')) == -1.0)

        # Set Luminance Preservation to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Luminance Preservation'), 1.0)
        Report.result(Tests.luminance_preservation_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Luminance Preservation')) == 1.0)

        # 20. Split Toning|Weight (float range 0.0 to 1.0, default 0.0)
        # Set Split Toning|Weight to it's maximum value to enable the parameter.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Split Toning Weight'), 1.0)
        Report.result(Tests.split_toning_weight_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Split Toning Weight')) == 1.0)

        # 21. Balance (float range -1.0 to 1.0)
        # Set Balance to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Balance'), -1.0)
        Report.result(Tests.balance_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Balance')) == -1.0)

        # Set Balance to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Balance'), 1.0)
        Report.result(Tests.balance_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Balance')) == 1.0)

        # 22. Split Toning|Shadows Color (Vector3 range 0.0 to 1.0)
        # Adjust Split Toning Shadows Color.
        st_shadows_color = math.Vector3(0.25, 0.0, 1.0)
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Split Toning Shadows Color'), st_shadows_color)
        st_shadows_color_value = hdr_color_grading_component.get_component_property_value(
            AtomComponentProperties.hdr_color_grading('Split Toning Shadows Color'))
        Report.result(Tests.edit_split_toning_shadow_color, st_shadows_color_value.IsClose(st_shadows_color))

        # 23. Split Toning|Highlights Color (Vector3 range 0.0 to 1.0)
        # Adjust Split Toning Highlights Color.
        st_highlights_color = math.Vector3(0.0, 1.0, 0.25)
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Split Toning Highlights Color'), st_highlights_color)
        st_highlights_color_value = hdr_color_grading_component.get_component_property_value(
            AtomComponentProperties.hdr_color_grading('Split Toning Highlights Color'))
        Report.result(Tests.edit_split_toning_highlights_color, st_highlights_color_value.IsClose(st_highlights_color))

        # 24. Update the Shadows Midtones Highlights|Weight parameter (float range 0.0 to 1.0)
        # Set SMH|Weight to its maximum value to enable the parameter.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('SMH Weight'), 1.0)
        Report.result(Tests.smh_weight_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('SMH Weight')) == 1.0)

        # 25. Shadows Start (float range 0.0 to 16.0, default 0.0)
        # Set Shadows Start to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Shadows Start'), 16.0)
        Report.result(Tests.shadows_start_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Shadows Start')) == 16.0)

        # Set Shadows Start to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Shadows Start'), 0.0)
        Report.result(Tests.shadows_start_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Shadows Start')) == 0.0)

        # 26. Shadows End (float range 0.0 to 16.0, default 0.3)
        # Set Shadows End to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Shadows End'), 0.0)
        Report.result(Tests.shadows_end_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Shadows End')) == 0.0)

        # Set Shadows End to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Shadows End'), 16.0)
        Report.result(Tests.shadows_end_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Shadows End')) == 16.0)

        # Return Shadows End to a low value for Game Mode.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Shadows End'), 0.3)

        # 27. Highlights Start (float range 0.0 to 16.0, default value 0.55)
        # Set Highlights Start to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Highlights Start'), 0.0)
        Report.result(Tests.highlights_start_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Highlights Start')) == 0.0)

        # Set Highlights Start to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Highlights Start'), 16.0)
        Report.result(Tests.highlights_start_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Highlights Start')) == 16.0)

        # Return Highlights Start to a low value for Game Mode.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Highlights Start'), 0.55)

        # 28. Highlights End (float range 0.0 to 16.0, default value 1.0)
        # Set Highlights End to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Highlights End'), 0.0)
        Report.result(Tests.highlights_end_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Highlights End')) == 0.0)

        # Set Highlights End to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Highlights End'), 16.0)
        Report.result(Tests.highlights_end_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Highlights End')) == 16.0)

        # 29. Shadows Midtones Highlights|Shadows Color (Vector3 range 0.0 to 1.0)
        # Adjust SMH Shadows color.
        smh_shadows_color = math.Vector3(0.0, 0.0, 1.0)
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('SMH Shadows Color'), smh_shadows_color)
        smh_shadows_color_value = hdr_color_grading_component.get_component_property_value(
            AtomComponentProperties.hdr_color_grading('SMH Shadows Color'))
        Report.result(Tests.edit_smh_shadows_color, smh_shadows_color_value.IsClose(smh_shadows_color))

        # 30. Shadows Midtones Highlights|Midtones Color (Vector3 range 0.0 to 1.0)
        # Adjust SMH Midtones color.
        smh_midtones_color = math.Vector3(1.0, 0.0, 0.0)
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('SMH Midtones Color'), smh_midtones_color)
        smh_midtones_color_value = hdr_color_grading_component.get_component_property_value(
            AtomComponentProperties.hdr_color_grading('SMH Midtones Color'))
        Report.result(Tests.edit_smh_midtones_color, smh_midtones_color_value.IsClose(smh_midtones_color))

        # 31. Shadows Midtones Highlights|Highlights Color (Vector3 range 0.0 to 1.0)
        # Adjust SMH Highlights color.
        smh_highlights_color = math.Vector3(0.0, 1.0, 0.0)
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('SMH Highlights Color'), smh_highlights_color)
        smh_highlights_color_value = hdr_color_grading_component.get_component_property_value(
            AtomComponentProperties.hdr_color_grading('SMH Highlights Color'))
        Report.result(Tests.edit_smh_highlights_color, smh_highlights_color_value.IsClose(smh_highlights_color))

        # 32. Channel Mixing Red (Vector3 range 0.0 to 1.0, default: x:1.0,y:0.0,z:0.0)
        # Adjust Channel Mixing Red.
        channel_red_color = math.Vector3(0.0, 0.0, 1.0)
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Channel Mixing Red'), channel_red_color)
        channel_red_color_value = hdr_color_grading_component.get_component_property_value(
            AtomComponentProperties.hdr_color_grading('Channel Mixing Red'))
        Report.result(Tests.edit_channel_mixing_red, channel_red_color_value.IsClose(channel_red_color))

        # 33. Channel Mixing Green (Vector3 range 0.0 to 1.0, default: x:0.0,y:1.0,z:0.0)
        # Adjust Channel Mixing Green.
        channel_green_color = math.Vector3(1.0, 0.0, 0.0)
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Channel Mixing Green'), channel_green_color)
        channel_green_color_value = hdr_color_grading_component.get_component_property_value(
            AtomComponentProperties.hdr_color_grading('Channel Mixing Green'))
        Report.result(Tests.edit_channel_mixing_green, channel_green_color_value.IsClose(channel_green_color))

        # 34. Channel Mixing Blue (Vector3 range 0.0 to 1.0, default: x:0.0,y:0.0,z:1.0)
        # Adjust Channel Mixing Blue.
        channel_blue_color = math.Vector3(0.0, 1.0, 0.0)
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Channel Mixing Blue'), channel_blue_color)
        channel_blue_color_value = hdr_color_grading_component.get_component_property_value(
            AtomComponentProperties.hdr_color_grading('Channel Mixing Blue'))
        Report.result(Tests.edit_channel_mixing_blue, channel_blue_color_value.IsClose(channel_blue_color))

        # 35. Final Adjustment|Weight (float range 0.0 to 1.0, default 1.0)
        # Set Final Adjustment|Weight to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Final Adjustment Weight'), 0.0)
        Report.result(Tests.final_adjustment_weight_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Final Adjustment Weight')) == 0.0)

        # Set Final Adjustment|Weight to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Final Adjustment Weight'), 1.0)
        Report.result(Tests.final_adjustment_weight_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Final Adjustment Weight')) == 1.0)

        # 36. Post Saturation (float range -100.0 to 100.0, default 0)
        # Set Post Saturation to its minimum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Post Saturation'), -100.0)
        Report.result(Tests.post_saturation_min_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Post Saturation')) == -100.0)

        # Set Post Saturation to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Post Saturation'), 100.0)
        Report.result(Tests.post_saturation_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Post Saturation')) == 100.0)

        # 37. Hue Shift (float range 0.0 to 1.0, default 0.0)
        # Set Hue Shift to its maximum value.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Hue Shift'), 1.0)
        Report.result(Tests.hue_shift_max_value,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Hue Shift')) == 1.0)

        # Cycle through LUT Resolutions.
        for lut_resolution in LUT_RESOLUTION.keys():

            # 38. LUT Resolution (from atom_constants.py LUT_RESOLUTION, default 16x16x16)
            hdr_color_grading_component.set_component_property_value(
                AtomComponentProperties.hdr_color_grading('LUT Resolution'), LUT_RESOLUTION[lut_resolution])
            test_lut_resolution = (
                f"Set Lut Resolution to: {lut_resolution}",
                f"P1: Lut Resolution failed to be set to {lut_resolution} ")
            Report.result(test_lut_resolution, hdr_color_grading_component.get_component_property_value(
                AtomComponentProperties.hdr_color_grading('LUT Resolution')) == LUT_RESOLUTION[lut_resolution])

            # Cycle through Shaper Types per LUT Resolution.
            for shaper_type in SHAPER_TYPE.keys():

                # 39. Set the Shaper Type parameter (from atom_constants.py SHAPER_TYPE, default none)
                hdr_color_grading_component.set_component_property_value(
                    AtomComponentProperties.hdr_color_grading('Shaper Type'), SHAPER_TYPE[shaper_type])
                test_shaper_type = (
                    f"Set Shaper Type to: {shaper_type}",
                    f"P1: Shaper Type failed to be set to {shaper_type} ")
                Report.result(test_shaper_type, hdr_color_grading_component.get_component_property_value(
                    AtomComponentProperties.hdr_color_grading('Shaper Type')) == SHAPER_TYPE[shaper_type])

        # 40. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 41. Test IsHidden.
        hdr_color_grading_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, hdr_color_grading_entity.is_hidden() is True)

        # 42. Test IsVisible.
        hdr_color_grading_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, hdr_color_grading_entity.is_visible() is True)

        # 43. Delete HDR Color Grading entity.
        hdr_color_grading_entity.delete()
        Report.result(Tests.entity_deleted, not hdr_color_grading_entity.exists())

        # 44. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, hdr_color_grading_entity.exists())

        # 45. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not hdr_color_grading_entity.exists())

        # 46. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_HDRColorGrading_AddedToEntity)
