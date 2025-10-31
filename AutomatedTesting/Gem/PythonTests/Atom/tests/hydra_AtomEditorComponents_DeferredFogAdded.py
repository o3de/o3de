"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    deferred_fog_creation = (
        "Deferred Fog Entity successfully created",
        "P0: Deferred Fog Entity failed to be created")
    deferred_fog_component = (
        "Entity has a Deferred Fog component",
        "P0: Entity failed to find Deferred Fog component")
    deferred_fog_component_removal = (
        "Deferred Fog component successfully removed",
        "P0: Deferred Fog component failed to be removed")
    removal_undo = (
        "UNDO Deferred Fog component removal success",
        "P0: UNDO Deferred Fog component removal failed")
    deferred_fog_disabled = (
        "Deferred Fog component disabled",
        "P0: Deferred Fog component was not disabled")
    postfx_layer_component = (
        "Entity has a PostFX Layer component",
        "P0: Entity did not have an PostFX Layer component")
    deferred_fog_enabled = (
        "Deferred Fog component enabled",
        "P0: Deferred Fog component was not enabled")
    enable_deferred_fog_parameter_enabled = (
        "Enable Deferred Fog parameter enabled",
        "P0: Enable Deferred Fog parameter was not enabled")
    enable_deferred_fog_parameter_disabled = (
        "Enable Deferred Fog parameter disabled",
        "P0: Enable Deferred Fog parameter was not disabled")
    enable_turbulence_properties_parameter_enabled = (
        "Enable Turbulence Properties parameter enabled",
        "P1: Enable Turbulence Properties parameter was not enabled")
    enable_turbulence_properties_parameter_disabled = (
        "Enable Turbulence Properties parameter disabled",
        "P1: Enable Turbulence Properties parameter was not disabled")
    enable_fog_layer_parameter_enabled = (
        "Enable Fog Layer parameter enabled",
        "P1: Enable Fog Layer parameter was not enabled")
    enable_fog_layer_parameter_disabled = (
        "Enable Fog Layer parameter disabled",
        "P1: Enable Fog Layer parameter was not disabled")
    edit_fog_color = (
        "Fog Color parameter updated",
        "P1: Fog Color parameter failed to update")
    fog_start_min = (
        "Fog Start Distance set to minimum value",
        "P1: Fog Start Distance failed to be set to minimum value")
    fog_start_max = (
        "Fog Start Distance set to maximum value",
        "P1: Fog Start Distance failed to be set to maximum value")
    fog_end_min = (
        "Fog End Distance set to minimum value",
        "P1: Fog End Distance failed to be set to minimum value")
    fog_end_max = (
        "Fog End Distance set to maximum value",
        "P1: Fog End Distance failed to be set to maximum value")
    fog_bottom_height_min = (
        "Fog Bottom Height set to minimum value",
        "P1: Fog Bottom Height failed to be set to minimum value")
    fog_bottom_height_max = (
        "Fog Bottom Height set to maximum value",
        "P1: Fog Bottom Height failed to be set to maximum value")
    fog_max_height_min = (
        "Fog Max Height set to minimum value",
        "P1: Fog Max Height failed to be set to minimum value")
    fog_max_height_max = (
        "Fog Max Height set to maximum value",
        "P1: Fog Max Height failed to be set to maximum value")
    first_octave_scale = (
        "Noise Texture First Octave Scale updated",
        "P1: Noise Texture First Octave Scale failed to be updated")
    first_octave_velocity = (
        "Noise Texture First Octave Velocity updated",
        "P1: Noise Texture First Octave Velocity failed to be updated")
    second_octave_scale = (
        "Noise Texture Second Octave Scale updated",
        "P1: Noise Texture Second Octave Scale failed to be updated")
    second_octave_velocity = (
        "Noise Texture Second Octave Velocity updated",
        "P1: Noise Texture Second Octave Velocity failed to be updated")
    octaves_blend_factor_min = (
        "Octaves Blend Factor set to minimum value",
        "P1: Octaves Blend Factort failed to be set to minimum value")
    octaves_blend_factor_max = (
        "Octaves Blend Factor set to maximum value",
        "P1: Octaves Blend Factort failed to be set to maximum value")
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
    edit_fog_mode = (
        "Fog mode updated",
        "P1: Failed to set the fog mode")
    fog_density_min = (
        "Fog Density set to minimum value",
        "P1: Fog Density failed to be set to minimum value")
    fog_density_clamp_min = (
        "Fog Density Clamp set to minimum value",
        "P1: Fog Density Clamp failed to be set to minimum value")
    fog_density_max = (
        "Fog Density set to maximum value",
        "P1: Fog Density failed to be set to maximum value")
    fog_density_clamp_max = (
        "Fog Density Clamp set to maximum value",
        "P1: Fog Density Clamp failed to be set to maximum value")
    


def AtomEditorComponents_DeferredFog_AddedToEntity():
    """
    Summary:
    Tests the Deferred Fog component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an Deferred Fog entity with no components.
    2) Add Deferred Fog component to Deferred Fog entity.
    3) Remove the Deferred Fog component.
    4) Undo Bloom component removal.
    5) Verify Deferred Fog component not enabled.
    6) Add PostFX Layer component since it is required by the Deferred Fog component.
    7) Verify Deferred Fog component is enabled.
    8) Enable/Disable the "Enable Deferred Fog" parameter.
    9) Enable/Disable the Enable Turbulence Properties parameter.
    10) Enable/Disable the Enable Fog Layer parameter.
    11) Edit the Fog Color parameter.
    12) Update the Fog Start Distance parameter to min/max values.
    13) Update the Fog End Distance parameter to min/max values.
    14) Update the Fog Bottom Height parameter to min/max values.
    15) Update the Fog Max Height parameter to min/max values.
    16) Edit the Noise Texture parameter.
    17) Update the Noise Texture First Octave Scale parameter to low/high values.
    18) Update the Noise Texture First Octave Velocity parameter to low/high values.
    19) Update the Noise Texture Second Octave Scale parameter to low/high values.
    20) Update the Noise Texture Second Octave Velocity parameter to low/high values.
    21) Update the Octaves Blend Factor parameter to min/max values.
    22 Enter/Exit game mode.
    23) Test IsHidden.
    24) Test IsVisible.
    25) Delete Deferred Fog entity.
    26) UNDO deletion.
    27) REDO deletion.
    28) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import (AtomComponentProperties, FOG_MODES)

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create an Deferred Fog entity with no components.
        deferred_fog_entity = EditorEntity.create_editor_entity(AtomComponentProperties.deferred_fog())
        Report.critical_result(Tests.deferred_fog_creation, deferred_fog_entity.exists())

        # 2. Add Deferred Fog component to Deferred Fog entity.
        deferred_fog_component = deferred_fog_entity.add_component(
            AtomComponentProperties.deferred_fog())
        Report.critical_result(
            Tests.deferred_fog_component,
            deferred_fog_entity.has_component(AtomComponentProperties.deferred_fog()))

        # 3. Remove the Deferred Fog component.
        deferred_fog_component.remove()
        general.idle_wait_frames(1)
        Report.critical_result(Tests.deferred_fog_component_removal,
                               not deferred_fog_entity.has_component(AtomComponentProperties.deferred_fog()))

        # 4. Undo Bloom component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.removal_undo, deferred_fog_entity.has_component(AtomComponentProperties.deferred_fog()))

        # 5. Verify Deferred Fog component not enabled.
        Report.result(Tests.deferred_fog_disabled, not deferred_fog_component.is_enabled())

        # 6. Add PostFX Layer component since it is required by the Deferred Fog component.
        deferred_fog_entity.add_component(AtomComponentProperties.postfx_layer())
        general.idle_wait_frames(1)
        Report.result(
            Tests.postfx_layer_component,
            deferred_fog_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify Deferred Fog component is enabled.
        Report.result(Tests.deferred_fog_enabled, deferred_fog_component.is_enabled())

        # 8. Enable/Disable the Enable Deferred Fog parameter.
        # Enable the Enable Deferred Fog parameter.
        deferred_fog_component.set_component_property_value(
            AtomComponentProperties.deferred_fog('Enable Deferred Fog'), True)
        Report.result(Tests.enable_deferred_fog_parameter_enabled,
                      deferred_fog_component.get_component_property_value(
                          AtomComponentProperties.deferred_fog('Enable Deferred Fog')) is True)

        # Disable the Enable Deferred Fog parameter.
        deferred_fog_component.set_component_property_value(
            AtomComponentProperties.deferred_fog('Enable Deferred Fog'), False)
        Report.result(Tests.enable_deferred_fog_parameter_disabled,
                      deferred_fog_component.get_component_property_value(
                          AtomComponentProperties.deferred_fog('Enable Deferred Fog')) is False)

        # Re-enable the Enable Deferred Fog parameter for game mode verification.
        deferred_fog_component.set_component_property_value(
            AtomComponentProperties.deferred_fog('Enable Deferred Fog'), True)
        general.idle_wait_frames(1)

        # Cycle through the fog modes
        for fog_mode in FOG_MODES.keys():
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Fog Mode'), FOG_MODES[fog_mode])

            general.idle_wait_frames(1)
            Report.result(Tests.edit_fog_mode,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Fog Mode')) == FOG_MODES[fog_mode])

            # 9. Enable/Disable the Enable Turbulence Properties parameter.
            # Enable the Enable Turbulence Properties parameter.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Enable Turbulence Properties'), True)
            Report.result(Tests.enable_turbulence_properties_parameter_enabled,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Enable Turbulence Properties')) is True)

            # Disable the Enable Turbulence Properties parameter.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Enable Turbulence Properties'), False)
            Report.result(Tests.enable_turbulence_properties_parameter_disabled,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Enable Turbulence Properties')) is False)

            # Re-enable the Enable Turbulence Properties parameter for game mode verification.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Enable Turbulence Properties'), True)

            # 10. Enable/Disable the Enable Fog Layer parameter.
            # Enable the Enable Fog Layer parameter.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Enable Fog Layer'), True)
            Report.result(Tests.enable_fog_layer_parameter_enabled,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Enable Fog Layer')) is True)

            # Disable the Enable Fog Layer parameter.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Enable Fog Layer'), False)
            Report.result(Tests.enable_fog_layer_parameter_disabled,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Enable Fog Layer')) is False)

            # Re-enable the Enable Fog Layer parameter for game mode verification.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Enable Fog Layer'), True)
            general.idle_wait_frames(1)

            # 11. Edit the Fog Color parameter.
            violet_color_value = math.Vector3(0.498, 0.0, 1.0)
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Fog Color'), violet_color_value)
            fog_color_value = deferred_fog_component.get_component_property_value(
                AtomComponentProperties.deferred_fog('Fog Color'))
            Report.result(Tests.edit_fog_color, fog_color_value.IsClose(violet_color_value))

            # 12. Update the Fog Start Distance parameter to min/max values.
            # Update the Fog Start Distance parameter to its minimum value.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Fog Start Distance'), 0.0)
            Report.result(Tests.fog_start_min,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Fog Start Distance')) == 0.0)

            # Update the Fog Start Distance parameter to its maximum value.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Fog Start Distance'), 5000.0)
            Report.result(Tests.fog_start_max,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Fog Start Distance')) == 5000.0)

            if FOG_MODES[fog_mode] == FOG_MODES['Linear']:
                # 13. Update the Fog End Distance parameter to min/max values.
                # Update the Fog End Distance parameter to its maximum value.
                deferred_fog_component.set_component_property_value(
                    AtomComponentProperties.deferred_fog('Fog End Distance'), 5000.0)
                Report.result(Tests.fog_end_max,
                              deferred_fog_component.get_component_property_value(
                                  AtomComponentProperties.deferred_fog('Fog End Distance')) == 5000.0)

                # Update the Fog End Distance parameter to its minimum value.
                deferred_fog_component.set_component_property_value(
                    AtomComponentProperties.deferred_fog('Fog End Distance'), 0.0)
                Report.result(Tests.fog_end_min,
                              deferred_fog_component.get_component_property_value(
                                  AtomComponentProperties.deferred_fog('Fog End Distance')) == 0.0)

            # 14. Update the Fog Bottom Height parameter to min/max values.
            # Update the Fog Bottom Height parameter to its maximum value.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Fog Bottom Height'), 5000.0)
            Report.result(Tests.fog_bottom_height_max,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Fog Bottom Height')) == 5000.0)

            # Update the Fog Bottom Height parameter to its minimum value.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Fog Bottom Height'), -5000.0)
            Report.result(Tests.fog_bottom_height_min,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Fog Bottom Height')) == -5000.0)

            # 15. Update the Fog Max Height parameter to min/max values.
            # Update the Fog Max Height parameter to its minimum value.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Fog Max Height'), -5000.0)
            Report.result(Tests.fog_max_height_min,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Fog Max Height')) == -5000.0)

            # Update the Fog Max Height parameter to its maximum value.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Fog Max Height'), 5000.0)
            Report.result(Tests.fog_max_height_max,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Fog Max Height')) == 5000.0)

            # Update the Fog Density Clamp to it's min/max values
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Fog Density Clamp'), 1.0)
            Report.result(Tests.fog_density_clamp_max,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Fog Density Clamp')) == 1.0)

            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Fog Density Clamp'), 0.0)
            Report.result(Tests.fog_density_clamp_min,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Fog Density Clamp')) == 0.0)

            if FOG_MODES[fog_mode] in (FOG_MODES['Exponential'], FOG_MODES['ExponentialSquared']):
                # Update the Fog Density to it's min/max values
                deferred_fog_component.set_component_property_value(
                    AtomComponentProperties.deferred_fog('Fog Density'), 1.0)
                Report.result(Tests.fog_density_max,
                              deferred_fog_component.get_component_property_value(
                                  AtomComponentProperties.deferred_fog('Fog Density')) == 1.0)

                deferred_fog_component.set_component_property_value(
                    AtomComponentProperties.deferred_fog('Fog Density'), 0.0)
                Report.result(Tests.fog_density_min,
                              deferred_fog_component.get_component_property_value(
                                  AtomComponentProperties.deferred_fog('Fog Density')) == 0.0)

            general.idle_wait_frames(1)

            # 16. Edit the Noise Texture parameter.
            # This field cannot currently be edited. It will be fixed in a future sprint.

            # Store Noise Texture First/Second Scale & Velocity value:
            set_octave = math.Vector2(-100.0, 100.0)

            # 17. Update the Noise Texture First Octave Scale parameter to low/high values.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Noise Texture First Octave Scale'), set_octave)
            get_first_octave_scale = deferred_fog_component.get_component_property_value(
                AtomComponentProperties.deferred_fog('Noise Texture First Octave Scale'))
            Report.result(Tests.first_octave_scale, get_first_octave_scale.IsClose(set_octave))

            # 18. Update the Noise Texture First Octave Velocity parameter to low/high values.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Noise Texture First Octave Velocity'), set_octave)
            get_first_octave_velocity = deferred_fog_component.get_component_property_value(
                AtomComponentProperties.deferred_fog('Noise Texture First Octave Velocity'))
            Report.result(Tests.first_octave_velocity, get_first_octave_velocity.IsClose(set_octave))

            # 19. Update the Noise Texture Second Octave Scale parameter to low/high values.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Noise Texture Second Octave Scale'), set_octave)
            get_second_octave_scale = deferred_fog_component.get_component_property_value(
                AtomComponentProperties.deferred_fog('Noise Texture Second Octave Scale'))
            Report.result(Tests.second_octave_scale, get_second_octave_scale.IsClose(set_octave))

            # 20. Update the Noise Texture Second Octave Velocity parameter to low/high values.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Noise Texture Second Octave Velocity'), set_octave)
            get_second_octave_velocity = deferred_fog_component.get_component_property_value(
                AtomComponentProperties.deferred_fog('Noise Texture Second Octave Velocity'))
            Report.result(Tests.second_octave_velocity, get_second_octave_velocity.IsClose(set_octave))

            # 21. Update the Octaves Blend Factor parameter to min/max values.
            # Update the Octaves Blend Factor to its minimum value.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Octaves Blend Factor'), 0.0)
            Report.result(Tests.octaves_blend_factor_min,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Octaves Blend Factor')) == 0.0)

            # Update the Octave Blend Factor to its maximum value.
            deferred_fog_component.set_component_property_value(
                AtomComponentProperties.deferred_fog('Octaves Blend Factor'), 1.0)
            Report.result(Tests.octaves_blend_factor_max,
                          deferred_fog_component.get_component_property_value(
                              AtomComponentProperties.deferred_fog('Octaves Blend Factor')) == 1.0)

            # 22. Enter/Exit game mode.
            TestHelper.enter_game_mode(Tests.enter_game_mode)
            general.idle_wait_frames(1)
            TestHelper.exit_game_mode(Tests.exit_game_mode)

            # 23. Test IsHidden.
            deferred_fog_entity.set_visibility_state(False)
            Report.result(Tests.is_hidden, deferred_fog_entity.is_hidden() is True)

            # 24. Test IsVisible.
            deferred_fog_entity.set_visibility_state(True)
            general.idle_wait_frames(1)
            Report.result(Tests.is_visible, deferred_fog_entity.is_visible() is True)

        # 25. Delete Deferred Fog entity.
        deferred_fog_entity.delete()
        Report.result(Tests.entity_deleted, not deferred_fog_entity.exists())

        # 26. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, deferred_fog_entity.exists())

        # 27. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not deferred_fog_entity.exists())

        # 28. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_DeferredFog_AddedToEntity)
