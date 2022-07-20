"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Shape components associated with specific light types.
LIGHT_SHAPES = {
    'sphere': 'Sphere Shape',
    'spot_disk': 'Disk Shape',
    'capsule': 'Capsule Shape',
    'quad': 'Quad Shape',
    'polygon': 'Polygon Prism Shape',
}


class Tests:
    light_creation = (
        "Light Entity successfully created",
        "P0: Light Entity failed to be created")
    light_component_removal = (
        "Light component successfully removed",
        "P1: Light component failed to be removed")
    light_component = (
        "Entity has a Light component",
        "P0: Entity failed to find Light component")
    removal_undo = (
        "UNDO Light component removal success",
        "P0: UNDO Light component removal failed")
    edit_light_color = (
        f"Light color updated",
        f"P1: Light color failed to update")
    edit_intensity_value = (
        f"Intensity updated",
        f"P1: Intensity failed to update")
    edit_attenuation_radius = (
        f"Attenuation radius Radius updated",
        f"P1: Attenuation radius Radius failed to update")
    enable_shadows = (
        f"Shadows enabled",
        f"P1: Shadows failed to be enabled")
    disable_shadows = (
        f"Shadows disabled",
        f"P1: Shadows failed to be disabled")
    edit_shadow_bias = (
        f"Shadow Bias updated",
        f"P1: Shadow Bias failed to be updated")
    edit_normal_bias = (
        f"Normal shadow bias updated",
        f"P1: Normal shadow bias failed to update")
    edit_filtering_sample_count = (
        f"Filtering sample count updated",
        f"P1: Filtering sample count failed to update")
    edit_esm_exponent = (
        f"ESM exponent updated",
        f"P1: ESM exponent failed to update")
    edit_inner_angle = (
        f"Shutters Inner angle updated",
        f"P1: Inner angle failed to update")
    edit_outer_angle = (
        f"Shutters Outer angle updated",
        f"P1: Outer angle failed to update")
    disable_shutters = (
        f"Shutters disabled",
        f"P1: Shutters failed to be disabled")
    enable_shutters = (
        f"Shutters enabled",
        f"P1: Shutters failed to be enabled")
    enable_both_directions = (
        f"Both directions enabled",
        f"P1: Both directions failed to be enabled")
    disable_both_directions = (
        f"Both directions disabled",
        f"P1: Both directions failed to be disabled")
    enable_fast_approximation = (
        f"Fast approximation enabled",
        f"P1: Fast approximation failed to be enabled")
    disable_fast_approximation = (
        f"Fast approximation disabled",
        f"P1: Fast approximation failed to be disabled")
    enter_game_mode = (
        "Entered game mode",
        "P0: Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "P0: Couldn't exit game mode")
    is_hidden = (
        "Entity is hidden",
        "P0: Entity was not hidden")
    is_visible = (
        "Entity is visible",
        "P0: Entity was not visible")
    entity_deleted = (
        "Entity deleted",
        "P0: Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "P0: UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "P0: REDO deletion failed")


def AtomEditorComponents_Light_AddedToEntity():
    """
    Summary:
    Tests the Light component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, all components can be manipulated,
        and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Light entity with no components.
    2) Add Light component to the Light entity.
    3) Remove existing Light component on the entity.
    4) UNDO the light component removal.
    5) Set the light type.
    6) Check for Shape component.
    7) Edit the Color parameter.
    8) Set the Intensity mode parameter.
    9) Edit the Intensity parameter.
    10) Set the Attenuation radius Mode.
    11) Edit the Attenuation radius Radius parameter (explicit only).
    12) Enable shadows (if applicable).
    13) Edit the Shadows Bias parameter.
    14) Edit the Normal shadow bias parameter.
    15) Set the Shadowmap size.
    16) Set the Shadow filter method.
    17) Edit the Filtering sample count parameter.
    18) Edit the ESM Exponent parameter.
    19) Disable Shadows (re-enabled after test for game mode verification).
    20) Edit the Inner angle parameter.
    21) Edit the Outer angle parameter.
    22) Disable Shutters.
    23) Enable Shutters.
    24) Enable Both directions.
    25) Disable Both directions (re-enabled after test for game mode verification).
    26) Enable Fast approximation.
    27) Disable Fast approximation.
    28) Enter/Exit game mode.
    29) Test IsHidden.
    30) Test IsVisible.
    REPEAT tests 2-30 for all applicable light types.
    31) Delete Light entity.
    32) UNDO deletion.
    33) REDO deletion.
    34) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import (AtomComponentProperties, LIGHT_TYPES, INTENSITY_MODE,
                                                ATTENUATION_RADIUS_MODE, SHADOWMAP_SIZE, SHADOW_FILTER_METHOD)

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create a Light entity with no components.
        light_entity = EditorEntity.create_editor_entity(AtomComponentProperties.light())
        Report.critical_result(Tests.light_creation, light_entity.exists())

        # 2. Add a Light component to the Light entity.
        light_component = light_entity.add_component(AtomComponentProperties.light())
        Report.critical_result(Tests.light_component,
                               light_entity.has_component(AtomComponentProperties.light()))

        # 3. Remove the light component.
        light_component.remove()
        general.idle_wait_frames(1)
        Report.critical_result(Tests.light_component_removal,
                               not light_entity.has_component(AtomComponentProperties.light()))

        # 4. Undo the Light component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.removal_undo,
                      light_entity.has_component(AtomComponentProperties.light()))

        # Copy LIGHT_TYPES to pop the 'unknown' key to prevent it from being run in this test.
        light_types_copy = LIGHT_TYPES.copy()
        light_types_copy.pop('unknown')

        # Cycle through light types to test component properties.
        for light_type in light_types_copy.keys():

            # Remove the Light component to begin loop with a clean component.
            light_component.remove()
            general.idle_wait_frames(1)
            Report.critical_result(Tests.light_component_removal,
                                   not light_entity.has_component(AtomComponentProperties.light()))

            # Add a new Light component to begin parameter tests.
            light_component = light_entity.add_component(AtomComponentProperties.light())

            # 5. Set light type.
            light_component.set_component_property_value(
                AtomComponentProperties.light('Light type'), LIGHT_TYPES[light_type])
            general.idle_wait_frames(1)
            test_light_type = (
                f"Set light type: {light_type.upper()}",
                f"P0: Light component failed to set {light_type.upper()} type")
            Report.result(test_light_type, light_component.get_component_property_value(
                              AtomComponentProperties.light('Light type')) == LIGHT_TYPES[light_type])

            # 6. Check for Shape component.
            if LIGHT_TYPES[light_type] in (LIGHT_TYPES['sphere'], LIGHT_TYPES['spot_disk'], LIGHT_TYPES['capsule'],
                                      LIGHT_TYPES['quad'], LIGHT_TYPES['polygon']):
                light_shape = LIGHT_SHAPES[light_type]
                test_light_shape = (
                    f"{light_shape} present",
                    f"P1: {light_shape} was not found")
                Report.result(test_light_shape, light_entity.has_component(light_shape))

            # 7. Edit Color parameter.
            color_value = math.Color(1.0, 0.0, 0.0, 1.0)
            light_component.set_component_property_value(AtomComponentProperties.light('Color'), color_value)
            general.idle_wait_frames(1)
            light_color = light_component.get_component_property_value(
               AtomComponentProperties.light('Color'))
            Report.result(Tests.edit_light_color, light_color.IsClose(color_value))

            # 8. Set Intensity mode.
            if LIGHT_TYPES[light_type] not in (LIGHT_TYPES['simple_point'], LIGHT_TYPES['simple_spot']):
                for intensity_mode in INTENSITY_MODE.keys():
                    light_component.set_component_property_value(
                        AtomComponentProperties.light('Intensity mode'), INTENSITY_MODE[intensity_mode])
                    general.idle_wait_frames(1)
                    test_intensity_mode = (
                        f"Intensity mode set to {intensity_mode}",
                        f"P1: Intensity mode failed to be set to {intensity_mode}")
                    Report.result(test_intensity_mode, light_component.get_component_property_value(
                        AtomComponentProperties.light('Intensity mode')) == INTENSITY_MODE[intensity_mode])

            # 9. Edit the Intensity parameter.
            light_component.set_component_property_value(AtomComponentProperties.light('Intensity'), 1000)
            general.idle_wait_frames(1)
            Report.result(Tests.edit_intensity_value,
                          light_component.get_component_property_value(
                              AtomComponentProperties.light('Intensity')) == 1000)

            # 10. Set the Attenuation radius Mode:
            for radius_mode in ATTENUATION_RADIUS_MODE.keys():
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Attenuation radius Mode'), ATTENUATION_RADIUS_MODE[radius_mode])
                general.idle_wait_frames(1)
                test_attenuation_mode = (
                    f"Attenuation radius Mode set to {radius_mode}",
                    f"P1: Attenuation radius Mode failed to be set to {radius_mode}")
                Report.result(test_attenuation_mode, light_component.get_component_property_value(
                    AtomComponentProperties.light('Attenuation radius Mode')) == ATTENUATION_RADIUS_MODE[radius_mode])

                # 11. Edit the Attenuation radius Radius parameter (explicit only).
                if ATTENUATION_RADIUS_MODE[radius_mode] == ATTENUATION_RADIUS_MODE['explicit']:
                    light_component.set_component_property_value(
                        AtomComponentProperties.light('Attenuation radius Radius'), 1000)
                    general.idle_wait_frames(1)
                    Report.result(Tests.edit_attenuation_radius,
                                  light_component.get_component_property_value(
                                      AtomComponentProperties.light('Attenuation radius Radius')) == 1000)

            # Shadow tests for applicable light types:
            if LIGHT_TYPES[light_type] in (LIGHT_TYPES['sphere'], LIGHT_TYPES['spot_disk']):
                # 12. Enable Shadows:
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Enable shadow'), True)
                general.idle_wait_frames(1)
                Report.result(
                    Tests.enable_shadows,
                    light_component.get_component_property_value(
                        AtomComponentProperties.light('Enable shadow')) is True)

                # 13. Edit the Shadows Bias parameter.
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Shadows Bias'), 100)
                general.idle_wait_frames(1)
                Report.result(
                    Tests.edit_shadow_bias,
                    light_component.get_component_property_value(
                        AtomComponentProperties.light('Shadows Bias')) == 100)

                # 14. Edit the Normal shadow bias parameter.
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Normal shadow bias'), 10)
                general.idle_wait_frames(1)
                Report.result(
                    Tests.edit_normal_bias,
                    light_component.get_component_property_value(
                        AtomComponentProperties.light('Normal shadow bias')) == 10)

                # 15. Set the Shadowmap size.
                for shadowmap_size in SHADOWMAP_SIZE.keys():
                    light_component.set_component_property_value(
                        AtomComponentProperties.light('Shadowmap size'), SHADOWMAP_SIZE[shadowmap_size])
                    general.idle_wait_frames(1)
                    test_shadowmap_size = (
                        f"Shadowmap size set to {shadowmap_size}.",
                        f"P1: Shadowmap size failed to be set to {shadowmap_size}.")
                    Report.result(test_shadowmap_size, light_component.get_component_property_value(
                        AtomComponentProperties.light('Shadowmap size')) == SHADOWMAP_SIZE[shadowmap_size])

                # Shadow filter method tests.
                # 16. Set the Shadow filter method.
                for filter_method in SHADOW_FILTER_METHOD.keys():
                    light_component.set_component_property_value(
                        AtomComponentProperties.light('Shadow filter method'), SHADOW_FILTER_METHOD[filter_method])
                    general.idle_wait_frames(1)
                    test_shadow_filter_method = (
                        f"Shadow filter method set to {filter_method}",
                        f"P1: Shadow filter method set to {filter_method}")
                    Report.result(test_shadow_filter_method, light_component.get_component_property_value(
                        AtomComponentProperties.light('Shadow filter method')) == SHADOW_FILTER_METHOD[filter_method])

                    # 17. Edit the Filtering sample count parameter.
                    if SHADOW_FILTER_METHOD[filter_method] in (SHADOW_FILTER_METHOD['PCF'],
                                                               SHADOW_FILTER_METHOD['PCF+ESM']):
                        light_component.set_component_property_value(
                            AtomComponentProperties.light('Filtering sample count'), 64)
                        general.idle_wait_frames(1)
                        Report.result(Tests.edit_filtering_sample_count,
                                      light_component.get_component_property_value(
                                          AtomComponentProperties.light('Filtering sample count')) == 64)

                    # 18. Edit the ESM Exponent parameter.
                    if SHADOW_FILTER_METHOD[filter_method] in (SHADOW_FILTER_METHOD['ESM'],
                                                               SHADOW_FILTER_METHOD['PCF+ESM']):
                        light_component.set_component_property_value(
                            AtomComponentProperties.light('ESM exponent'), 5000)
                        general.idle_wait_frames(1)
                        Report.result(Tests.edit_esm_exponent,
                                      light_component.get_component_property_value(
                                          AtomComponentProperties.light('ESM exponent')) == 5000)

                # 19. Disable Shadows (re-enabled after test for game mode verification):
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Enable shadow'), False)
                general.idle_wait_frames(1)
                Report.result(
                    Tests.disable_shadows,
                    light_component.get_component_property_value(
                        AtomComponentProperties.light('Enable shadow')) is False)
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Enable shadow'), True)

            # Shutter tests for applicable light types:
            if LIGHT_TYPES[light_type] in (LIGHT_TYPES['spot_disk'], LIGHT_TYPES['simple_spot']):
                # 20. Edit the Inner angle parameter:
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Inner angle'), 0.5)
                general.idle_wait_frames(1)
                Report.result(Tests.edit_inner_angle,
                              light_component.get_component_property_value(
                                  AtomComponentProperties.light('Inner angle')) == 0.5)

                # 21. Edit the Outer angle parameter:
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Outer angle'), 90)
                general.idle_wait_frames(1)
                Report.result(Tests.edit_outer_angle,
                              light_component.get_component_property_value(
                                  AtomComponentProperties.light('Outer angle')) == 90)

            # Disable/Enable Shutters tests:
            if LIGHT_TYPES[light_type] == LIGHT_TYPES['spot_disk']:
                # 22. Disable Shutters:
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Enable shutters'), False)
                general.idle_wait_frames(1)
                Report.result(
                    Tests.disable_shutters,
                    light_component.get_component_property_value(
                        AtomComponentProperties.light('Enable shutters')) is False)

                # 23. Enable Shutters:
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Enable shutters'), True)
                general.idle_wait_frames(1)
                Report.result(
                    Tests.enable_shutters,
                    light_component.get_component_property_value(
                        AtomComponentProperties.light('Enable shutters')) is True)

            # Enable/Disable Both directions tests:
            if LIGHT_TYPES[light_type] in (LIGHT_TYPES['quad'], LIGHT_TYPES['polygon']):
                # 24. Enable Both directions:
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Both directions'), True)
                general.idle_wait_frames(1)
                Report.result(
                    Tests.enable_both_directions,
                    light_component.get_component_property_value(
                        AtomComponentProperties.light('Both directions')) is True)

                # 25. Disable Both directions parameter (re-enabled after test for game-mode verification):
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Both directions'), False)
                general.idle_wait_frames(1)
                Report.result(
                    Tests.disable_both_directions,
                    light_component.get_component_property_value(
                        AtomComponentProperties.light('Both directions')) is False)
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Both directions'), True)

            # Enable/Disable Fast approximation tests:
            if LIGHT_TYPES[light_type] == LIGHT_TYPES['quad']:
                # 26. Enable Fast approximation:
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Fast approximation'), True)
                general.idle_wait_frames(1)
                Report.result(
                    Tests.enable_fast_approximation,
                    light_component.get_component_property_value(
                        AtomComponentProperties.light('Fast approximation')) is True)

                # 27. Disable Fast approximation:
                light_component.set_component_property_value(
                    AtomComponentProperties.light('Fast approximation'), False)
                general.idle_wait_frames(1)
                Report.result(
                    Tests.disable_fast_approximation,
                    light_component.get_component_property_value(
                        AtomComponentProperties.light('Fast approximation')) is False)

            # 28. Enter/Exit game mode.
            TestHelper.enter_game_mode(Tests.enter_game_mode)
            general.idle_wait_frames(1)
            TestHelper.exit_game_mode(Tests.exit_game_mode)

            # 29. Test IsHidden.
            light_entity.set_visibility_state(False)
            general.idle_wait_frames(1)
            Report.result(Tests.is_hidden, light_entity.is_hidden() is True)

            # 30. Test IsVisible.
            light_entity.set_visibility_state(True)
            general.idle_wait_frames(1)
            Report.result(Tests.is_visible, light_entity.is_visible() is True)

        # 31. Delete Light entity.
        light_entity.delete()
        general.idle_wait_frames(1)
        Report.result(Tests.entity_deleted, not light_entity.exists())

        # 32. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, light_entity.exists())

        # 33. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not light_entity.exists())

        # 34. Look for errors asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Light_AddedToEntity)
