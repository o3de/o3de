"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    directional_light_component_disabled = (
        "Disabled Directional Light component",
        "Couldn't disable Directional Light component")
    enter_game_mode = (
        "Entered game mode",
        "Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "Couldn't exit game mode")
    global_skylight_component_disabled = (
        "Disabled Global Skylight (IBL) component",
        "Couldn't disable Global Skylight (IBL) component")
    hdri_skybox_component_disabled = (
        "Disabled HDRi Skybox component",
        "Couldn't disable HDRi Skybox component")
    light_component_added = (
        "Light component added",
        "Couldn't add Light component")
    light_component_color_property_set = (
        "Color property was set",
        "Color property was not set")
    light_component_enable_shadow_property_set = (
        "Enable shadow property was set",
        "Enable shadow property was not set")
    light_component_enable_shutters_property_set = (
        "Enable shutters property was set",
        "Enable shutters property was not set")
    light_component_inner_angle_property_set = (
        "Inner angle property was set",
        "Inner angle property was not set")
    light_component_intensity_property_set = (
        "Intensity property was set",
        "Intensity property was not set")
    light_component_light_type_property_set = (
        "Light type property was set",
        "Light type property was not set")
    light_component_outer_angle_property_set = (
        "Outer angle property was set",
        "Outer angle property was not set")
    material_component_material_asset_property_set = (
        "Material Asset property was set",
        "Material Asset property was not set")
    spot_light_entity_created = (
        "Spot Light entity created",
        "Couldn't create Spot Light entity")


def AtomGPU_LightComponent_SpotLightScreenshotsMatchGoldenImages():
    """
    Summary:
    Light component test using the Spot (disk) Light type property option and modifying the shadows and colors.
    Sets each scene up and then takes a screenshot of each scene for test comparison.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.
    - Close error windows and display helpers then update the viewport size.
    - Runs the create_basic_atom_rendering_scene() function to setup the test scene.

    Expected Behavior:
    The test scripts sets up the scenes correctly and takes accurate screenshots.

    Test Steps:
    1. Find the Directional Light entity then disable its Directional Light component.
    2. Disable Global Skylight (IBL) component on the Global Skylight (IBL) entity.
    3. Disable HDRi Skybox component on the Global Skylight (IBL) entity.
    4. Create a Spot Light entity and rotate it.
    5. Attach a Light component to the Spot Light entity.
    6. Set the Light component Light Type to Spot (disk).
    7. Enter game mode and take a screenshot then exit game mode.
    8. Change the default material asset for the Ground Plane entity.
    9. Enter game mode and take a screenshot then exit game mode.
    10. Increase the Intensity value of the Light component.
    11. Enter game mode and take a screenshot then exit game mode.
    12. Change the Light component Color property value.
    13. Enter game mode and take a screenshot then exit game mode.
    14. Change the Light component Enable shutters, Inner angle, and Outer angle property values.
    15. Enter game mode and take a screenshot then exit game mode.
    16. Change the Light component Enable shadow and Shadowmap size property values then move Spot Light entity.
    17. Enter game mode and take a screenshot then exit game mode.
    18. Look for errors.

    :return: None
    """
    import os

    import azlmbr.legacy.general as general
    import azlmbr.paths

    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper

    from Atom.atom_utils.atom_constants import AtomComponentProperties, LIGHT_TYPES
    from Atom.atom_utils.atom_component_helper import (
        initial_viewport_setup, create_basic_atom_rendering_scene, enter_exit_game_mode_take_screenshot)

    DEGREE_RADIAN_FACTOR = 0.0174533

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("", "Base")

        # Setup: Close error windows and display helpers then update the viewport size.
        TestHelper.close_error_windows()
        TestHelper.close_display_helpers()
        initial_viewport_setup()
        general.update_viewport()

        # Setup: Runs the create_basic_atom_rendering_scene() function to setup the test scene.
        create_basic_atom_rendering_scene()

        # Test steps begin.
        # 1. Find the Directional Light entity then disable its Directional Light component.
        directional_light_entity = EditorEntity.find_editor_entity(AtomComponentProperties.directional_light())
        directional_light_component = directional_light_entity.get_components_of_type(
            [AtomComponentProperties.directional_light()])[0]
        directional_light_component.disable_component()
        Report.critical_result(Tests.directional_light_component_disabled, not directional_light_component.is_enabled())

        # 2. Disable Global Skylight (IBL) component on the Global Skylight (IBL) entity.
        global_skylight_entity = EditorEntity.find_editor_entity(AtomComponentProperties.global_skylight())
        global_skylight_component = global_skylight_entity.get_components_of_type(
            [AtomComponentProperties.global_skylight()])[0]
        global_skylight_component.disable_component()
        Report.critical_result(Tests.global_skylight_component_disabled, not global_skylight_component.is_enabled())

        # 3. Disable HDRi Skybox component on the Global Skylight (IBL) entity.
        hdri_skybox_component = global_skylight_entity.get_components_of_type(
            [AtomComponentProperties.hdri_skybox()])[0]
        hdri_skybox_component.disable_component()
        Report.critical_result(Tests.hdri_skybox_component_disabled, not hdri_skybox_component.is_enabled())

        # 4. Create a Spot Light entity and rotate it.
        spot_light_name = "Spot Light"
        spot_light_entity = EditorEntity.create_editor_entity_at(
            azlmbr.math.Vector3(0.7, -2.0, 1.0), spot_light_name)
        rotation = azlmbr.math.Vector3(DEGREE_RADIAN_FACTOR * 300.0, 0.0, 0.0)
        spot_light_entity.set_local_rotation(rotation)
        Report.critical_result(Tests.spot_light_entity_created, spot_light_entity.exists())

        # 5. Attach a Light component to the Spot Light entity.
        light_component = spot_light_entity.add_component(AtomComponentProperties.light())
        Report.critical_result(Tests.light_component_added, light_component.is_enabled())

        # 6. Set the Light component Light Type to Spot (disk).
        light_component.set_component_property_value(
            AtomComponentProperties.light('Light type'), LIGHT_TYPES['spot_disk'])
        Report.result(
            Tests.light_component_light_type_property_set,
            light_component.get_component_property_value(
                AtomComponentProperties.light('Light type')) == LIGHT_TYPES['spot_disk'])

        # 7. Enter game mode and take a screenshot then exit game mode.
        enter_exit_game_mode_take_screenshot("SpotLight_1.ppm", Tests.enter_game_mode, Tests.exit_game_mode)

        # 8. Change the default material asset for the Ground Plane entity.
        ground_plane_name = "Ground Plane"
        ground_plane_entity = EditorEntity.find_editor_entity(ground_plane_name)
        ground_plane_material_component_name = AtomComponentProperties.material()
        ground_plane_material_component = ground_plane_entity.get_components_of_type(
            [ground_plane_material_component_name])[0]
        ground_plane_material_asset_path = os.path.join(
            "Materials", "Presets", "Macbeth", "22_neutral_5-0_0-70d.azmaterial")
        ground_plane_material_asset = Asset.find_asset_by_path(ground_plane_material_asset_path, False)
        ground_plane_material_component.set_component_property_value(
            AtomComponentProperties.material('Material Asset'), ground_plane_material_asset.id)
        Report.result(
            Tests.material_component_material_asset_property_set,
            ground_plane_material_component.get_component_property_value(
                AtomComponentProperties.material('Material Asset')) == ground_plane_material_asset.id)

        # 9. Enter game mode and take a screenshot then exit game mode.
        enter_exit_game_mode_take_screenshot("SpotLight_2.ppm", Tests.enter_game_mode, Tests.exit_game_mode)

        # 10. Increase the Intensity value of the Light component.
        light_component.set_component_property_value(AtomComponentProperties.light('Intensity'), 800.0)
        Report.result(
            Tests.light_component_intensity_property_set,
            light_component.get_component_property_value(
                AtomComponentProperties.light('Intensity')) == 800.0)

        # 11. Enter game mode and take a screenshot then exit game mode.
        enter_exit_game_mode_take_screenshot("SpotLight_3.ppm", Tests.enter_game_mode, Tests.exit_game_mode)

        # 12. Change the Light component Color property value.
        color_value = azlmbr.math.Color(47.0 / 255.0, 75.0 / 255.0, 37.0 / 255.0, 255.0 / 255.0)
        light_component.set_component_property_value(AtomComponentProperties.light('Color'), color_value)
        Report.result(
            Tests.light_component_color_property_set,
            light_component.get_component_property_value(AtomComponentProperties.light('Color')) == color_value)

        # 13. Enter game mode and take a screenshot then exit game mode.
        enter_exit_game_mode_take_screenshot("SpotLight_4.ppm", Tests.enter_game_mode, Tests.exit_game_mode)

        # 14. Change the Light component Enable shutters, Inner angle, and Outer angle property values.
        enable_shutters = True
        inner_angle = 60.0
        outer_angle = 75.0
        light_component.set_component_property_value(AtomComponentProperties.light('Enable shutters'), enable_shutters)
        light_component.set_component_property_value(AtomComponentProperties.light('Inner angle'), inner_angle)
        light_component.set_component_property_value(AtomComponentProperties.light('Outer angle'), outer_angle)
        Report.result(
            Tests.light_component_enable_shutters_property_set,
            light_component.get_component_property_value(
                AtomComponentProperties.light('Enable shutters')) == enable_shutters)
        Report.result(
            Tests.light_component_inner_angle_property_set,
            light_component.get_component_property_value(AtomComponentProperties.light('Inner angle')) == inner_angle)
        Report.result(
            Tests.light_component_outer_angle_property_set,
            light_component.get_component_property_value(AtomComponentProperties.light('Outer angle')) == outer_angle)

        # 15. Enter game mode and take a screenshot then exit game mode.
        enter_exit_game_mode_take_screenshot("SpotLight_5.ppm", Tests.enter_game_mode, Tests.exit_game_mode)

        # 16. Change the Light component Enable shadow and slightly move Spot Light entity.
        light_component.set_component_property_value(AtomComponentProperties.light('Enable shadow'), True)
        Report.result(
            Tests.light_component_enable_shadow_property_set,
            light_component.get_component_property_value(AtomComponentProperties.light('Enable shadow')) is True)
        spot_light_entity.set_world_rotation(azlmbr.math.Vector3(0.7, -2.0, 1.9))

        # 17. Enter game mode and take a screenshot then exit game mode.
        enter_exit_game_mode_take_screenshot("SpotLight_6.ppm", Tests.enter_game_mode, Tests.exit_game_mode)

        # 18. Look for errors.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomGPU_LightComponent_SpotLightScreenshotsMatchGoldenImages)
