"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    look_modification_creation = (
        "Look Modification Entity successfully created",
        "P0: Look Modification Entity failed to be created")
    look_modification_component = (
        "Entity has a Look Modification component",
        "P0: Entity failed to find Look Modification component")
    look_modification_component_removal = (
        "Look Modification component successfully removed",
        "P1: Look Modification component failed to be removed")
    removal_undo = (
        "UNDO Look Modification component removal success",
        "P1: UNDO Look Modification component removal failed")
    look_modification_disabled = (
        "Look Modification component disabled",
        "P0: Look Modification component was not disabled")
    postfx_layer_component = (
        "Entity has a PostFX Layer component",
        "P0: Entity did not have an PostFX Layer component")
    look_modification_enabled = (
        "Look Modification component enabled",
        "P0: Look Modification component was not enabled")
    toggle_enable_parameter_on = (
        "Enable look modification parameter enabled",
        "P0: Enable look modification parameter was not enabled")
    toggle_enable_parameter_off = (
        "Enable look modification parameter disabled",
        "P1: Enable look modification parameter was not disabled")
    color_grading_lut_set = (
        "Entity has the Color Grading LUT set",
        "P0: Color Grading LUT failed to be set")
    lut_intensity_min_value = (
        "Lut Intensity set to minimum value",
        "P1: Lut Intensity failed to be set to minimum value")
    lut_intensity_max_value = (
        "Lut Intensity set to maximum value",
        "P1: Lut Intensity failed to be set to maximum value")
    lut_override_min_value = (
        "Lut Override set to minimum value",
        "P1: Lut Override failed to be set to minimum value")
    lut_override_max_value = (
        "Lut Override set to maximum value",
        "P1: Lut Override failed to be set to maximum value")
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


def AtomEditorComponents_LookModification_AddedToEntity():
    """
    Summary:
    Tests the Look Modification component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an Look Modification entity with no components.
    2) Add Look Modification component to Look Modification entity.
    3) Remove the Look Modification component.
    4) Undo Look Modification component removal.
    5) Verify Look Modification component not enabled.
    6) Add PostFX Layer component since it is required by the Look Modification component.
    7) Verify Look Modification component is enabled.
    8) Toggle the "Enable look modification" parameter (default False).
    9) Add LUT asset to the Color Grading LUT parameter.
    10) Set the Shaper Type parameter (from atom_constants.py SHAPER_TYPE, default none)
    11) LUT Intensity (float range 0.0 to 1.0, default 1.0)
    12) LUT Override (float range 0.0 to 1.0, default 1.0)
    13) Enter/Exit game mode.
    14) Test IsHidden.
    15) Test IsVisible.
    16) Delete Look Modification entity.
    17) UNDO deletion.
    18) REDO deletion.
    19) Look for errors.

    :return: None
    """

    import os

    import azlmbr.legacy.general as general

    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties, SHAPER_TYPE

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create an Look Modification entity with no components.
        look_modification_entity = EditorEntity.create_editor_entity(AtomComponentProperties.look_modification())
        Report.critical_result(Tests.look_modification_creation, look_modification_entity.exists())

        # 2. Add Look Modification component to Look Modification entity.
        look_modification_component = look_modification_entity.add_component(
            AtomComponentProperties.look_modification())
        Report.critical_result(
            Tests.look_modification_component,
            look_modification_entity.has_component(AtomComponentProperties.look_modification()))

        # 3. Remove the Look Modification component.
        look_modification_component.remove()
        general.idle_wait_frames(1)
        Report.result(Tests.look_modification_component_removal,
                      not look_modification_entity.has_component(AtomComponentProperties.look_modification()))

        # 4. Undo Look Modification component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.removal_undo,
                      look_modification_entity.has_component(AtomComponentProperties.look_modification()))

        # 5. Verify Look Modification component not enabled.
        Report.result(Tests.look_modification_disabled, not look_modification_component.is_enabled())

        # 6. Add PostFX Layer component since it is required by the Look Modification component.
        look_modification_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(
            Tests.postfx_layer_component,
            look_modification_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify Look Modification component is enabled.
        Report.result(Tests.look_modification_enabled, look_modification_component.is_enabled())

        # 8. Toggle the "Enable look modification" parameter (default False).
        # Toggle the "Enable look modification" parameter on.
        look_modification_component.set_component_property_value(
            AtomComponentProperties.look_modification('Enable look modification'), True)
        Report.result(Tests.toggle_enable_parameter_on,
                      look_modification_component.get_component_property_value(
                          AtomComponentProperties.look_modification('Enable look modification')) is True)

        # Toggle the "Enable look modification" parameter off.
        look_modification_component.set_component_property_value(
            AtomComponentProperties.look_modification('Enable look modification'), False)
        Report.result(Tests.toggle_enable_parameter_off,
                      look_modification_component.get_component_property_value(
                          AtomComponentProperties.look_modification('Enable look modification')) is False)

        # Toggle the "Enable look modification" parameter back on for testing.
        look_modification_component.set_component_property_value(
            AtomComponentProperties.look_modification('Enable look modification'), True)

        # 9. Set the Color Grading LUT asset on the Look Modification entity.
        color_grading_lut_path = os.path.join("colorgrading", "testdata", "photoshop", "inv-Log2-48nits",
                                              "test_3dl_32_lut.azasset")
        color_grading_lut_asset = Asset.find_asset_by_path(color_grading_lut_path, False)
        look_modification_component.set_component_property_value(
            AtomComponentProperties.look_modification('Color Grading LUT'), color_grading_lut_asset.id)
        Report.result(
            Tests.color_grading_lut_set,
            color_grading_lut_asset.id == look_modification_component.get_component_property_value(
                                          AtomComponentProperties.look_modification('Color Grading LUT')))

        # Cycle through options in the Shaper Type parameter.
        for shaper_type in SHAPER_TYPE.keys():

            # 10. Set the Shaper Type parameter (from atom_constants.py SHAPER_TYPE, default none)
            look_modification_component.set_component_property_value(
                AtomComponentProperties.look_modification('Shaper Type'), SHAPER_TYPE[shaper_type])
            test_shaper_type = (
                f"Set Shaper Type to: {shaper_type}",
                f"P1: Shaper Type failed to be set to {shaper_type} ")
            Report.result(test_shaper_type, look_modification_component.get_component_property_value(
                AtomComponentProperties.look_modification('Shaper Type')) == SHAPER_TYPE[shaper_type])

        # 11. LUT Intensity (float range 0.0 to 1.0, default 1.0)
        # Set LUT Intensity to its minimum value.
        look_modification_component.set_component_property_value(
            AtomComponentProperties.look_modification('LUT Intensity'), 0.0)
        Report.result(Tests.lut_intensity_min_value,
                      look_modification_component.get_component_property_value(
                          AtomComponentProperties.look_modification('LUT Intensity')) == 0.0)

        # Set LUT Intensity to its maximum value.
        look_modification_component.set_component_property_value(
            AtomComponentProperties.look_modification('LUT Intensity'), 1.0)
        Report.result(Tests.lut_intensity_max_value,
                      look_modification_component.get_component_property_value(
                          AtomComponentProperties.look_modification('LUT Intensity')) == 1.0)

        # 12. LUT Override (float range 0.0 to 1.0, default 1.0)
        # Set LUT Override to its minimum value.
        look_modification_component.set_component_property_value(
            AtomComponentProperties.look_modification('LUT Override'), 0.0)
        Report.result(Tests.lut_override_min_value,
                      look_modification_component.get_component_property_value(
                          AtomComponentProperties.look_modification('LUT Override')) == 0.0)

        # Set LUT Override to its maximum value.
        look_modification_component.set_component_property_value(
            AtomComponentProperties.look_modification('LUT Override'), 1.0)
        Report.result(Tests.lut_override_max_value,
                      look_modification_component.get_component_property_value(
                          AtomComponentProperties.look_modification('LUT Override')) == 1.0)

        # 13. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 14. Test IsHidden.
        look_modification_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, look_modification_entity.is_hidden() is True)

        # 15. Test IsVisible.
        look_modification_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, look_modification_entity.is_visible() is True)

        # 16. Delete Look Modification entity.
        look_modification_entity.delete()
        Report.result(Tests.entity_deleted, not look_modification_entity.exists())

        # 17. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, look_modification_entity.exists())

        # 18. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not look_modification_entity.exists())

        # 19. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_LookModification_AddedToEntity)
