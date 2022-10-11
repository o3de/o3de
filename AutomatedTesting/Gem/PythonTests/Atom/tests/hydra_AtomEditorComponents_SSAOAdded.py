"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    remove_component = (
        "Component removal was successful",
        "P0: Component removal failed")
    ssao_creation = (
        "SSAO Entity successfully created",
        "P0: SSAO Entity failed to be created")
    ssao_component = (
        "Entity has a SSAO component",
        "P0: Entity failed to find SSAO component")
    ssao_component_disabled = (
        "SSAO component disabled",
        "P0: SSAO component was not disabled.")
    postfx_layer_component = (
        "Entity has a PostFX Layer component",
        "P0: Entity did not have an PostFX Layer component")
    ssao_component_enabled = (
        "SSAO component enabled",
        "P0: SSAO component was not enabled.")
    enter_game_mode = (
        "Entered game mode",
        "P0: Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "P0: Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "P0: Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "P0: Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "P0: Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "P0: UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "P0: REDO deletion failed")
    enable_ssao = (
        "Enable SSAO property set",
        "P1: Enable SSAO property failed to set correctly")
    enable_blur = (
        "Enable Blur property set",
        "P1: Enabled Blure property failed to set correctly")
    set_ssao_strength = (
        "SSAO Strength property set",
        "P1: SSAO Strength property failed to set correctly")
    set_sample_radius = (
        "Sample Radius property set",
        "P1: Sample Radius property failed to set correctly")
    set_blur_strength = (
        "Blur Strength property set",
        "P1: Blur Strength property failed to set correctly")
    set_blur_edge_threshold = (
        "Blur Edge Threshold property set",
        "P1: Blur Edge Threshold property failed to set correctly")
    set_blur_sharpness = (
        "Blur Sharpness property set",
        "P1: Blur Sharpness property failed to set correctly")
    enable_downsample = (
        "Enable Downsample property set",
        "P1: Enable Downsample property failed to set correctly")
    enable_override = (
        "Enable Override property set",
        "P1: Enable Override property failed to set correctly")
    set_strength_override = (
        "Strength Override property set",
        "P1: Strength Override property failed to set correctly")
    set_samplingradius_override = (
        "SamplingRadius Override property set",
        "P1: SamplingRadius Override property failed to set correctly")
    enable_blur_override = (
        "Enable Blur Override property set",
        "P1: Enable Blure Override property failed to set correctly")
    set_blur_const_falloff = (
        "Blur Const Falloff Override property set",
        "P1: Blur Const Falloff Override property failed to set correctly")
    set_blur_depth_falloff_strength_override = (
        "Blur Depth Falloff Strength Override property set",
        "P1: Blur Depth Falloff Strength Override property failed to set correctly")
    set_blur_depth_falloff_threshold_override = (
        "Blur Depth Falloff Threshold Override property set",
        "P1: Blur Depth Falloff Threshold Override property failed to set correctly")
    enable_downsample_override = (
        "Enable Downsample Override property set",
        "P1: Enable Downsample Override property failed to set correctly")


def AtomEditorComponents_SSAO_AddedToEntity():
    """
    Summary:
    Tests the SSAO component can be added to an entity and has the expected functionality.
    Screen Space Ambient Occlusion (SSAO) is a PostFX shadow lighting effect.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a SSAO entity with no components.
    2) Add SSAO component to SSAO entity.
    3) Remove SSAO component.
    4) UNDO component removal.
    5) Verify SSAO component not enabled.
    6) Add PostFX Layer component since it is required by the SSAO component.
    7) Verify SSAO component is enabled.
    8) Toggle Enable SSAO (default True)
    9) Toggle Enable Blur False (default True)
    10) Set SSAO Strength to high/low values then return to default
    11) Set Sample Radius to high/low values then return to default
    12) Set Blur Strength to high/low values then return to default
    13) Set Blur Edge Threshold to high/low values
    14) Set Blur Sharpness to high/low values then return to default
    15) Toggle Enable Downsample (default True)
    16) Toggle Enabled Override (default True)
    17) Set Strength Override to 0.0 then return to default 1.0
    18) Set SamplingRadius Override to 0.0 then return to default 1.0
    19) Toggle EnableBlur Override (default True)
    20) Set BlurConstFalloff Override to 0.0 then return to default 1.0
    21) Set BlurDepthFalloffStrength Override to 0.0 then return to default 1.0
    22) Set BlurDepthFalloffThreshold Override to 0.0 then return to default 1.0
    23) Toggle EnableDownsample Override (default true)
    24) Enter/Exit game mode.
    25) Test IsHidden.
    26) Test IsVisible.
    27) Delete SSAO entity.
    28) UNDO deletion.
    29) REDO deletion.
    30) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create a SSAO entity with no components.
        ssao_entity = EditorEntity.create_editor_entity(AtomComponentProperties.ssao())
        Report.critical_result(Tests.ssao_creation, ssao_entity.exists())

        # 2. Add SSAO component to SSAO entity.
        ssao_component = ssao_entity.add_component(AtomComponentProperties.ssao())
        Report.critical_result(
            Tests.ssao_component,
            ssao_entity.has_component(AtomComponentProperties.ssao()))

        # 3. Remove SSAO component.
        ssao_component.remove()
        general.idle_wait_frames(1)
        Report.result(Tests.remove_component,
                      not ssao_entity.has_component(AtomComponentProperties.ssao()))

        # 4. UNDO component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.critical_result(
            Tests.ssao_component,
            ssao_entity.has_component(AtomComponentProperties.ssao()))

        # 5. Verify SSAO component not enabled.
        Report.result(Tests.ssao_component_disabled, not ssao_component.is_enabled())

        # 6. Add PostFX Layer component since it is required by the SSAO component.
        ssao_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(
            Tests.postfx_layer_component,
            ssao_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify SSAO component is enabled.
        Report.result(Tests.ssao_component_enabled, ssao_component.is_enabled())

        # 8. Toggle Enable SSAO (default True)
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Enable SSAO'), False)
        Report.result(
            Tests.enable_ssao,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Enable SSAO')) is False)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Enable SSAO'), True)
        Report.result(
            Tests.enable_ssao,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Enable SSAO')) is True)

        # 9. Toggle Enable Blur False (default True)
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Enable Blur'), False)
        Report.result(
            Tests.enable_blur,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Enable Blur')) is False)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Enable Blur'), True)
        Report.result(
            Tests.enable_blur,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Enable Blur')) is True)

        # 10. Set SSAO Strength to high/low values then return to default
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('SSAO Strength'), 2.0)
        Report.result(
            Tests.set_ssao_strength,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('SSAO Strength')) == 2.0)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('SSAO Strength'), 0.0)
        Report.result(
            Tests.set_ssao_strength,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('SSAO Strength')) == 0.0)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('SSAO Strength'), 1.0)

        # 11. Set Sample Radius to high/low values then return to default
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Sampling Radius'), 0.25)
        Report.result(
            Tests.set_sample_radius,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Sampling Radius')) == 0.25)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Sampling Radius'), 0.0)
        Report.result(
            Tests.set_sample_radius,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Sampling Radius')) == 0.0)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Sampling Radius'), 0.05)

        # 12. Set Blur Strength to high/low values then return to default
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Blur Strength'), 1.0)
        Report.result(
            Tests.set_blur_strength,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Blur Strength')) == 1.0)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Blur Strength'), 0.0)
        Report.result(
            Tests.set_blur_strength,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Blur Strength')) == 0.0)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Blur Strength'), 0.85)

        # 13. Set Blur Edge Threshold to high/low values
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Blur Edge Threshold'), 1.0)
        Report.result(
            Tests.set_blur_edge_threshold,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Blur Edge Threshold')) == 1.0)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Blur Edge Threshold'), 0.0)
        Report.result(
            Tests.set_blur_edge_threshold,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Blur Edge Threshold')) == 0.0)

        # 14. Set Blur Sharpness to high/low values then return to default
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Blur Sharpness'), 400.0)
        Report.result(
            Tests.set_blur_sharpness,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Blur Sharpness')) == 400.0)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Blur Sharpness'), 0.0)
        Report.result(
            Tests.set_blur_sharpness,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Blur Sharpness')) == 0.0)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Blur Sharpness'), 200.0)

        # 15. Toggle Enable Downsample (default True)
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Enable Downsample'), False)
        Report.result(
            Tests.enable_downsample,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Enable Downsample')) is False)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Enable Downsample'), True)
        Report.result(
            Tests.enable_downsample,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Enable Downsample')) is True)

        # 16. Toggle Enabled Override (default True)
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Enabled Override'), False)
        Report.result(
            Tests.enable_downsample,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Enabled Override')) is False)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Enabled Override'), True)
        Report.result(
            Tests.enable_downsample,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Enabled Override')) is True)

        # 17. Set Strength Override to 0.0 then return to default 1.0
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Strength Override'), 0.0)
        Report.result(
            Tests.set_strength_override,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Strength Override')) == 0.0)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('Strength Override'), 1.0)
        Report.result(
            Tests.set_strength_override,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('Strength Override')) == 1.0)

        # 18. Set SamplingRadius Override to 0.0 then return to default 1.0
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('SamplingRadius Override'), 0.0)
        Report.result(
            Tests.set_samplingradius_override,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('SamplingRadius Override')) == 0.0)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('SamplingRadius Override'), 1.0)
        Report.result(
            Tests.set_samplingradius_override,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('SamplingRadius Override')) == 1.0)

        # 19. Toggle EnableBlur Override (default True)
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('EnableBlur Override'), False)
        Report.result(
            Tests.enable_blur_override,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('EnableBlur Override')) is False)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('EnableBlur Override'), True)
        Report.result(
            Tests.enable_blur_override,
            ssao_component.get_component_property_value(AtomComponentProperties.ssao('EnableBlur Override')) is True)

        # 20. Set BlurConstFalloff Override to 0.0 then return to default 1.0
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('BlurConstFalloff Override'), 0.0)
        Report.result(
            Tests.set_blur_const_falloff,
            ssao_component.get_component_property_value(
                AtomComponentProperties.ssao('BlurConstFalloff Override')) == 0.0)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('BlurConstFalloff Override'), 1.0)
        Report.result(
            Tests.set_blur_const_falloff,
            ssao_component.get_component_property_value(
                AtomComponentProperties.ssao('BlurConstFalloff Override')) == 1.0)

        # 21. Set BlurDepthFalloffStrength Override to 0.0 then return to default 1.0
        ssao_component.set_component_property_value(
            AtomComponentProperties.ssao('BlurDepthFalloffStrength Override'), 0.0)
        Report.result(
            Tests.set_blur_depth_falloff_strength_override,
            ssao_component.get_component_property_value(
                AtomComponentProperties.ssao('BlurDepthFalloffStrength Override')) == 0.0)

        ssao_component.set_component_property_value(
            AtomComponentProperties.ssao('BlurDepthFalloffStrength Override'), 1.0)
        Report.result(
            Tests.set_blur_depth_falloff_strength_override,
            ssao_component.get_component_property_value(
                AtomComponentProperties.ssao('BlurDepthFalloffStrength Override')) == 1.0)

        # 22. Set BlurDepthFalloffThreshold Override to 0.0 then return to default 1.0
        ssao_component.set_component_property_value(
            AtomComponentProperties.ssao('BlurDepthFalloffThreshold Override'), 0.0)
        Report.result(
            Tests.set_blur_depth_falloff_threshold_override,
            ssao_component.get_component_property_value(
                AtomComponentProperties.ssao('BlurDepthFalloffThreshold Override')) == 0.0)

        ssao_component.set_component_property_value(
            AtomComponentProperties.ssao('BlurDepthFalloffThreshold Override'), 1.0)
        Report.result(
            Tests.set_blur_depth_falloff_threshold_override,
            ssao_component.get_component_property_value(
                AtomComponentProperties.ssao('BlurDepthFalloffThreshold Override')) == 1.0)

        # 23. Toggle EnableDownsample Override (default true)
        ssao_component.set_component_property_value(AtomComponentProperties.ssao('EnableDownsample Override'), False)
        Report.result(
            Tests.enable_downsample_override,
            ssao_component.get_component_property_value(
                AtomComponentProperties.ssao('EnableDownsample Override')) is False)

        ssao_component.set_component_property_value(AtomComponentProperties.ssao('EnableDownsample Override'), True)
        Report.result(
            Tests.enable_downsample_override,
            ssao_component.get_component_property_value(
                AtomComponentProperties.ssao('EnableDownsample Override')) is True)

        # 24. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 25. Test IsHidden.
        ssao_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, ssao_entity.is_hidden() is True)

        # 26. Test IsVisible.
        ssao_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, ssao_entity.is_visible() is True)

        # 27. Delete SSAO entity.
        ssao_entity.delete()
        Report.result(Tests.entity_deleted, not ssao_entity.exists())

        # 28. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, ssao_entity.exists())

        # 29. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not ssao_entity.exists())

        # 30. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_SSAO_AddedToEntity)
