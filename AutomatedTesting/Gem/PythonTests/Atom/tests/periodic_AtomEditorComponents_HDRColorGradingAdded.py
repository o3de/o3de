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
    generate_lut = (
        "Lut Generated",
        "P0: Lut failed to generate.")
    has_look_modification_component = (
        "Entity has Look Modification component",
        "P0: Entity does not have expected Look Modification component")
    look_modification_is_enabled = (
        "Look Modification component is enabled",
        "P0: Look Modification component failed to enable")
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
    5) Verify HDR Color Grading component is not enabled.
    6) Add PostFX Layer component since it is required by the HDR Color Grading component.
    7) Verify HDR Color Grading component is enabled.
    8) Toggle "Enable HDR Color Grading" (default False)
    9) LUT Resolution (from atom_constants.py LUT_RESOLUTION, default 16x16x16)
    10) Set the Shaper Type parameter (from atom_constants.py SHAPER_TYPE, default none)
    11) Generate LUT
    12) Activate LUT
    13) Enter/Exit game mode.
    14) Test IsHidden.
    15) Test IsVisible.
    16) Delete HDR Color Grading entity.
    17) UNDO deletion.
    18) REDO deletion.
    19) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.render as render

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

        # 5. Verify HDR Color Grading component is not enabled.
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

        # Cycle through LUT Resolutions.
        for lut_resolution in LUT_RESOLUTION.keys():

            # 9. LUT Resolution (from atom_constants.py LUT_RESOLUTION, default 16x16x16)
            hdr_color_grading_component.set_component_property_value(
                AtomComponentProperties.hdr_color_grading('LUT Resolution'), LUT_RESOLUTION[lut_resolution])
            test_lut_resolution = (
                f"Set Lut Resolution to: {lut_resolution}",
                f"P1: Lut Resolution failed to be set to {lut_resolution} ")
            Report.result(test_lut_resolution, hdr_color_grading_component.get_component_property_value(
                AtomComponentProperties.hdr_color_grading('LUT Resolution')) == LUT_RESOLUTION[lut_resolution])

            # Cycle through Shaper Types per LUT Resolution.
            for shaper_type in SHAPER_TYPE.keys():

                # 10. Set the Shaper Type parameter (from atom_constants.py SHAPER_TYPE, default none)
                hdr_color_grading_component.set_component_property_value(
                    AtomComponentProperties.hdr_color_grading('Shaper Type'), SHAPER_TYPE[shaper_type])
                test_shaper_type = (
                    f"Set Shaper Type to: {shaper_type}",
                    f"P1: Shaper Type failed to be set to {shaper_type} ")
                Report.result(test_shaper_type, hdr_color_grading_component.get_component_property_value(
                    AtomComponentProperties.hdr_color_grading('Shaper Type')) == SHAPER_TYPE[shaper_type])

        # 11. Generate LUT
        render.EditorHDRColorGradingRequestBus(azlmbr.bus.Event, "GenerateLutAsync", hdr_color_grading_entity.id)
        Report.critical_result(
            Tests.generate_lut,
            TestHelper.wait_for_condition(
                lambda: hdr_color_grading_component.get_component_property_value(
                    AtomComponentProperties.hdr_color_grading('Generated LUT Path')) != '',
                timeout_in_seconds=20))

        # 12. Activate LUT
        render.EditorHDRColorGradingRequestBus(azlmbr.bus.Event, "ActivateLutAsync", hdr_color_grading_entity.id)
        general.idle_wait_frames(10)
        Report.result(
            Tests.hdr_color_grading_disabled,
            not hdr_color_grading_component.is_enabled())
        Report.result(
            Tests.has_look_modification_component,
            hdr_color_grading_entity.has_component(AtomComponentProperties.look_modification()))

        look_modification_component = hdr_color_grading_entity.get_components_of_type(
                                                                  [AtomComponentProperties.look_modification()])[0]
        Report.result(
            Tests.look_modification_is_enabled,
            look_modification_component.is_enabled())

        # 13. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 14. Test IsHidden.
        hdr_color_grading_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, hdr_color_grading_entity.is_hidden() is True)

        # 15. Test IsVisible.
        hdr_color_grading_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, hdr_color_grading_entity.is_visible() is True)

        # 16. Delete HDR Color Grading entity.
        hdr_color_grading_entity.delete()
        Report.result(Tests.entity_deleted, not hdr_color_grading_entity.exists())

        # 17. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, hdr_color_grading_entity.exists())

        # 18. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not hdr_color_grading_entity.exists())

        # 19. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_HDRColorGrading_AddedToEntity)
