"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    chromatic_aberration_creation = (
        "Chromatic Aberration Entity successfully created",
        "P0: Chromatic Aberration Entity failed to be created")
    chromatic_aberration_component = (
        "Entity has a Chromatic Aberration component",
        "P0: Entity failed to find Chromatic Aberration component")
    chromatic_aberration_component_removal = (
        "Chromatic Aberration component successfully removed",
        "P0: Chromatic Aberration component failed to be removed")
    removal_undo = (
        "UNDO Chromatic Aberration component removal success",
        "P0: UNDO Chromatic Aberration component removal failed")
    chromatic_aberration_disabled = (
        "Chromatic Aberration component disabled",
        "P0: Chromatic Aberration component was not disabled")
    postfx_layer_component = (
        "Entity has a PostFX Layer component",
        "P0: Entity did not have an PostFX Layer component")
    chromatic_aberration_enabled = (
        "Chromatic Aberration component enabled",
        "P0: Chromatic Aberration component was not enabled")
    enable_chromatic_aberration_parameter_enabled = (
        "Enable Chromatic Aberration parameter enabled",
        "P0: Enable Chromatic Aberration parameter was not enabled")
    enable_chromatic_aberration_parameter_disabled = (
        "Enable Chromatic Aberration parameter disabled",
        "P1: Enable Chromatic Aberration parameter was not disabled")
    enabled_override_parameter_enabled = (
        "Enabled Override parameter enabled",
        "P1: Enabled Override parameter was not enabled")
    enabled_override_parameter_disabled = (
        "Enabled Override parameter disabled",
        "P1: Enabled Override parameter was not disabled")
    strength_override_min = (
        "Strength Override parameter set to minimum value",
        "P1: Strength Override parameter failed to be set to minimum value")
    strength_override_max = (
        "Strength Override parameter set to maximum value",
        "P1: Strength Override parameter failed to be set to maximum value")
    blend_override_min = (
        "Blend Override parameter set to minimum value",
        "P1: Blend Override parameter failed to be set to minimum value")
    blend_override_max = (
        "Blend Override parameter set to maximum value",
        "P1: Blend Override parameter failed to be set to maximum value")
    strength_min = (
        "Strength parameter set to minimum value",
        "P1: Strength parameter failed to be set to minimum value")
    strength_max = (
        "Strength parameter set to a maximum value",
        "P1: Strength parameter failed to be set to maximum value")
    blend_min = (
        "Blend parameter set to minimum value",
        "P1: Blend parameter failed to be set to minimum value")
    blend_max = (
        "Blend parameter set to maximum value",
        "P1: Blend parameter failed to be set to maximum value")
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


def AtomEditorComponents_ChromaticAberration_AddedToEntity():
    """
    Summary:
    Tests the Chromatic Aberration component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an Chromatic Aberration entity with no components.
    2) Add Chromatic Aberration component to Chromatic Aberration entity.
    3) Remove the Chromatic Aberration component.
    4) Undo Chromatic Aberration component removal.
    5) Verify Chromatic Aberration component not enabled.
    6) Add PostFX Layer component since it is required by the Chromatic Aberration component.
    7) Verify Chromatic Aberration component is enabled.
    8) Enable/Disable the "Enable Chromatic Aberration" parameter.
    9) Enable/Disable the Enabled Override parameter.
    10) Update the Strength Override parameter to min/max values.
    11) Update the Blend Override parameter to min/max values.
    12) Update the Strength parameter to min/max values.
    13) Update the Blend parameter to min/max values.
    14) Enter/Exit game mode.
    15) Test IsHidden.
    16) Test IsVisible.
    17) Delete Chromatic Aberration entity.
    18) UNDO deletion.
    19) REDO deletion.
    20) Look for errors and asserts.

    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.wait_utils import PrefabWaiter
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create an Chromatic Aberration entity with no components.
        chromatic_aberration_entity = EditorEntity.create_editor_entity(AtomComponentProperties.chromatic_aberration())
        Report.critical_result(Tests.chromatic_aberration_creation, chromatic_aberration_entity.exists())

        # 2. Add Chromatic Aberration component to Chromatic Aberration entity.
        chromatic_aberration_component = chromatic_aberration_entity.add_component(AtomComponentProperties.chromatic_aberration())
        Report.critical_result(Tests.chromatic_aberration_component,
                               chromatic_aberration_entity.has_component(AtomComponentProperties.chromatic_aberration()))

        # 3. Remove the Chromatic Aberration component.
        chromatic_aberration_component.remove()
        general.idle_wait_frames(1)
        Report.critical_result(Tests.chromatic_aberration_component_removal,
                               not chromatic_aberration_entity.has_component(AtomComponentProperties.chromatic_aberration()))

        # 4. Undo Chromatic Aberration component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.removal_undo,
                      chromatic_aberration_entity.has_component(AtomComponentProperties.chromatic_aberration()))

        # 5. Verify Chromatic Aberration component not enabled.
        Report.result(Tests.chromatic_aberration_disabled, not chromatic_aberration_component.is_enabled())

        # 6. Add PostFX Layer component since it is required by the Chromatic Aberration component.
        chromatic_aberration_entity.add_component(AtomComponentProperties.postfx_layer())
        general.idle_wait_frames(1)
        Report.result(Tests.postfx_layer_component,
                      chromatic_aberration_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify Chromatic Aberration component is enabled.
        Report.result(Tests.chromatic_aberration_enabled, chromatic_aberration_component.is_enabled())

        # 8. Enable/Disable the "Enable Chromatic Aberration" parameter.
        # Enable the "Enable Chromatic Aberration" parameter.
        chromatic_aberration_component.set_component_property_value(
            AtomComponentProperties.chromatic_aberration('Enable Chromatic Aberration'), True)
        Report.result(
            Tests.enable_chromatic_aberration_parameter_enabled,
            chromatic_aberration_component.get_component_property_value(
                AtomComponentProperties.chromatic_aberration('Enable Chromatic Aberration')) is True)

        # Disable the "Enable Chromatic Aberration" parameter.
        chromatic_aberration_component.set_component_property_value(
            AtomComponentProperties.chromatic_aberration('Enable Chromatic Aberration'), False)
        Report.result(
            Tests.enable_chromatic_aberration_parameter_disabled,
            chromatic_aberration_component.get_component_property_value(
                AtomComponentProperties.chromatic_aberration('Enable Chromatic Aberration')) is False)

        # Re-enable the "Enable Chromatic Aberration" parameter for game mode verification.
        chromatic_aberration_component.set_component_property_value(
            AtomComponentProperties.chromatic_aberration('Enable Chromatic Aberration'), True)
        general.idle_wait_frames(1)

        # 9. Enable/Disable the Enabled Override parameter.
        # Disable the Enabled Override parameter.
        chromatic_aberration_component.set_component_property_value(
            AtomComponentProperties.chromatic_aberration('Enabled Override'), False)
        Report.result(
            Tests.enabled_override_parameter_disabled,
            chromatic_aberration_component.get_component_property_value(
                AtomComponentProperties.chromatic_aberration('Enabled Override')) is False)

        # Enable the Enabled Override parameter.
        chromatic_aberration_component.set_component_property_value(
            AtomComponentProperties.chromatic_aberration('Enabled Override'), True)
        Report.result(
            Tests.enabled_override_parameter_enabled,
            chromatic_aberration_component.get_component_property_value(
                AtomComponentProperties.chromatic_aberration('Enabled Override')) is True)

        # 10. Update the Strength Override parameter to min/max values.
        # Update the Strength Override parameter to minimum value.
        chromatic_aberration_component.set_component_property_value(
            AtomComponentProperties.chromatic_aberration('Strength Override'), 0.0)
        Report.result(
            Tests.strength_override_min,
            chromatic_aberration_component.get_component_property_value(
                AtomComponentProperties.chromatic_aberration('Strength Override')) == 0.0)

        # Update the Strength Override parameter to maximum value.
        chromatic_aberration_component.set_component_property_value(
            AtomComponentProperties.chromatic_aberration('Strength Override'), 1.0)
        Report.result(
            Tests.strength_override_max,
            chromatic_aberration_component.get_component_property_value(
                AtomComponentProperties.chromatic_aberration('Strength Override')) == 1.0)

        # 11. Update the Blend Override parameter to min/max values.
        # Update the Blend Override parameter to minimum value.
        chromatic_aberration_component.set_component_property_value(
            AtomComponentProperties.chromatic_aberration('Blend Override'), 0.0)
        Report.result(
            Tests.blend_override_min,
            chromatic_aberration_component.get_component_property_value(
                AtomComponentProperties.chromatic_aberration('Blend Override')) == 0.0)

        # Update the Blend Override parameter to maximum value.
        chromatic_aberration_component.set_component_property_value(
            AtomComponentProperties.chromatic_aberration('Blend Override'), 1.0)
        Report.result(
            Tests.blend_override_max,
            chromatic_aberration_component.get_component_property_value(
                AtomComponentProperties.chromatic_aberration('Blend Override')) == 1.0)

        general.idle_wait_frames(1)

        # 12. Update the Strength parameter to min/max values.
        # Update the Strength parameter to minimum value.
        chromatic_aberration_component.set_component_property_value(
            AtomComponentProperties.chromatic_aberration('Strength'), 0.0)
        Report.result(
            Tests.strength_min,
            chromatic_aberration_component.get_component_property_value(
                AtomComponentProperties.chromatic_aberration('Strength')) == 0.0)

        # Update the Strength parameter to a max value.
        chromatic_aberration_component.set_component_property_value(
            AtomComponentProperties.chromatic_aberration('Strength'), 1.0)
        Report.result(
            Tests.strength_max,
            chromatic_aberration_component.get_component_property_value(
                AtomComponentProperties.chromatic_aberration('Strength')) == 1.0)

        # 13. Update the Blend parameter to min/max value.
        # Update the Blend parameter to minimum value.
        chromatic_aberration_component.set_component_property_value(
            AtomComponentProperties.chromatic_aberration('Blend'), 0.0)
        Report.result(
            Tests.blend_min,
            chromatic_aberration_component.get_component_property_value(
                AtomComponentProperties.chromatic_aberration('Blend')) == 0.0)

        # Update the Blend parameter to maximum value.
        chromatic_aberration_component.set_component_property_value(
            AtomComponentProperties.chromatic_aberration('Blend'), 1.0)
        Report.result(
            Tests.blend_max,
            chromatic_aberration_component.get_component_property_value(
                AtomComponentProperties.chromatic_aberration('Blend')) == 1.0)

        # 14. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 15. Test IsHidden.
        chromatic_aberration_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, chromatic_aberration_entity.is_hidden() is True)

        # 16. Test IsVisible.
        chromatic_aberration_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, chromatic_aberration_entity.is_visible() is True)

        # 17. Delete Chromatic Aberration entity.
        chromatic_aberration_entity.delete()
        PrefabWaiter.wait_for_propagation()
        Report.result(Tests.entity_deleted, not chromatic_aberration_entity.exists())

        # 18. UNDO deletion.
        general.undo()
        PrefabWaiter.wait_for_propagation()
        Report.result(Tests.deletion_undo, chromatic_aberration_entity.exists())

        # 19. REDO deletion.
        general.redo()
        PrefabWaiter.wait_for_propagation()
        Report.result(Tests.deletion_redo, not chromatic_aberration_entity.exists())

        # 20. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_ChromaticAberration_AddedToEntity)
