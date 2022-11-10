"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    area_light_entity_created = (
        "Area Light entity successfully created",
        "Area Light entity failed to be created")
    area_light_entity_deleted = (
        "Area Light entity was deleted",
        "Area Light entity was not deleted")
    enter_game_mode = (
        "Entered game mode",
        "Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "Couldn't exit game mode")
    light_component_added = (
        "Light component was added",
        "Light component wasn't added")
    light_component_attenuation_radius_property_set = (
        "Light component Attenuation Radius Property was set",
        "Light component Attenuation Radius Property was not set")
    light_component_color_property_set = (
        "Light component Color property was set",
        "Light component Color property was not set")
    light_component_intensity_property_set = (
        "Light component Intensity property was set",
        "Light component Intensity property was not set")
    light_component_light_type_property_set = (
        "Light type property was set",
        "Light type property was not set")


def AtomGPU_LightComponent_AreaLightScreenshotsMatchGoldenImages():
    """
    Summary:
    Light component test using the Capsule, Spot (disk), and Point (sphere) Light type property options.
    Sets each scene up and then takes a screenshot of each scene for test comparison.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.
    - Close error windows and display helpers then update the viewport size.
    - Runs the create_basic_atom_rendering_scene() function to setup the test scene.

    Expected Behavior:
    The test scripts sets up the scenes correctly and takes accurate screenshots.

    Test Steps:
    1. Create Area Light entity with no components.
    2. Add a Light component to the Area Light entity.
    3. Set the Light type property to Capsule for the Light component.
    4. Set the Light component's Color property to 255, 0, 0.
    5. Enter game mode and take a screenshot then exit game mode.
    6. Set the Intensity property of the Light component to 0.0.
    7. Set the Attenuation Radius Mode property of the Light component to 1 (automatic).
    8. Enter game mode and take a screenshot then exit game mode.
    9. Set the Intensity property of the Light component to 1000.0
    10. Enter game mode and take a screenshot then exit game mode.
    11. Set the Light type property to Spot (disk) for the Light component & rotate DEGREE_RADIAN_FACTOR * 90 degrees.
    12. Enter game mode and take a screenshot then exit game mode.
    13. Set the Light type property to Point (sphere) instead of Spot (disk) for the Light component.
    14. Enter game mode and take a screenshot then exit game mode.
    15. Delete the Area Light entity.
    16. Compare the screenshots to golden images.
    17. Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.paths

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper

    from Atom.atom_utils.atom_constants import AtomComponentProperties, ATTENUATION_RADIUS_MODE, LIGHT_TYPES
    from Atom.atom_utils.atom_component_helper import (
        initial_viewport_setup,
        create_basic_atom_rendering_scene,
        enter_exit_game_mode_take_screenshot,
        compare_screenshot_to_golden_image)

    from Atom.atom_utils.screenshot_utils import (FOLDER_PATH, screenshot_compare_result_code_to_string)

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

        # Setup: Define the screenshot names and threshold pairs
        screenshot_thresholds = {
            "AreaLight_1.png" : 0.02,
            "AreaLight_2.png" : 0.02,
            "AreaLight_3.png" : 0.02,
            "AreaLight_4.png" : 0.02,
            "AreaLight_5.png" : 0.02
        }

        # Test steps begin.
        # 1. Create Area Light entity with no components.
        area_light_entity_name = "Area Light"
        area_light_entity = EditorEntity.create_editor_entity_at(
            azlmbr.math.Vector3(0.0, 0.0, 0.0), area_light_entity_name)
        Report.critical_result(Tests.area_light_entity_created, area_light_entity.exists())

        # 2. Add a Light component to the Area Light entity.
        light_component = area_light_entity.add_component(AtomComponentProperties.light())
        Report.critical_result(
            Tests.light_component_added, area_light_entity.has_component(AtomComponentProperties.light()))

        # 3. Set the Light type property to Capsule for the Light component.
        light_component.set_component_property_value(
            AtomComponentProperties.light('Light type'), LIGHT_TYPES['capsule'])
        Report.result(
            Tests.light_component_light_type_property_set,
            light_component.get_component_property_value(
                AtomComponentProperties.light('Light type')) == LIGHT_TYPES['capsule'])

        # 4. Set the Light component's Color property to 255, 0, 0.
        light_component_color_value = azlmbr.math.Color(255.0, 0.0, 0.0, 0.0)
        light_component.set_component_property_value(
            AtomComponentProperties.light('Color'), light_component_color_value)
        Report.result(
            Tests.light_component_color_property_set,
            light_component.get_component_property_value(
                AtomComponentProperties.light('Color')) == light_component_color_value)

        # 5. Enter game mode and take a screenshot then exit game mode.
        enter_exit_game_mode_take_screenshot("AreaLight_1.png", Tests.enter_game_mode, Tests.exit_game_mode)

        # 6. Set the Intensity property of the Light component to 0.0.
        light_component.set_component_property_value(AtomComponentProperties.light('Intensity'), 0.0)
        Report.result(
            Tests.light_component_intensity_property_set,
            light_component.get_component_property_value(AtomComponentProperties.light('Intensity')) == 0.0)

        # 7. Set the Attenuation Radius Mode property of the Light component to 1 (automatic).
        light_component.set_component_property_value(
            AtomComponentProperties.light('Attenuation radius Mode'), ATTENUATION_RADIUS_MODE['automatic'])
        Report.result(
            Tests.light_component_attenuation_radius_property_set,
            light_component.get_component_property_value(
                AtomComponentProperties.light('Attenuation radius Mode')) == ATTENUATION_RADIUS_MODE['automatic'])

        # 8. Enter game mode and take a screenshot then exit game mode.
        enter_exit_game_mode_take_screenshot("AreaLight_2.png", Tests.enter_game_mode, Tests.exit_game_mode)

        # 9. Set the Intensity property of the Light component to 1000.0
        light_component.set_component_property_value(AtomComponentProperties.light('Intensity'), 1000.0)
        Report.result(
            Tests.light_component_intensity_property_set,
            light_component.get_component_property_value(AtomComponentProperties.light('Intensity')) == 1000.0)

        # 10. Enter game mode and take a screenshot then exit game mode.
        enter_exit_game_mode_take_screenshot("AreaLight_3.png", Tests.enter_game_mode, Tests.exit_game_mode)

        # 11. Set the Light type property to Spot (disk) for the Light component &
        # rotate DEGREE_RADIAN_FACTOR * 90 degrees.
        light_component.set_component_property_value(
            AtomComponentProperties.light('Light type'), LIGHT_TYPES['spot_disk'])
        area_light_rotation = azlmbr.math.Vector3(DEGREE_RADIAN_FACTOR * 90.0, 0.0, 0.0)
        azlmbr.components.TransformBus(azlmbr.bus.Event, "SetLocalRotation", area_light_entity.id, area_light_rotation)
        Report.result(
            Tests.light_component_light_type_property_set,
            light_component.get_component_property_value(
                AtomComponentProperties.light('Light type')) == LIGHT_TYPES['spot_disk'])

        # 12. Enter game mode and take a screenshot then exit game mode.
        enter_exit_game_mode_take_screenshot("AreaLight_4.png", Tests.enter_game_mode, Tests.exit_game_mode)

        # 13. Set the Light type property to Point (sphere) instead of Spot (disk) for the Light component.
        light_component.set_component_property_value(
            AtomComponentProperties.light('Light type'), LIGHT_TYPES['sphere'])
        Report.result(
            Tests.light_component_light_type_property_set,
            light_component.get_component_property_value(
                AtomComponentProperties.light('Light type')) == LIGHT_TYPES['sphere'])

        # 14. Enter game mode and take a screenshot then exit game mode.
        enter_exit_game_mode_take_screenshot("AreaLight_5.png", Tests.enter_game_mode, Tests.exit_game_mode)

        # 15. Delete the Area Light entity.
        area_light_entity.delete()
        Report.result(Tests.area_light_entity_deleted, not area_light_entity.exists())

        # 16. Compare screenshots to golden images.
        for screenshot_name, screenshot_threshold in screenshot_thresholds.items():
            image_diff_result = compare_screenshot_to_golden_image(FOLDER_PATH, screenshot_name, screenshot_name)
            screenshot_compare_execution = (
                    f"Screenshot {screenshot_name} comparison succeeded.",
                    f"Screenshot {screenshot_name} comparison failed due to "
                    + f"{screenshot_compare_result_code_to_string(image_diff_result.result_code)}.");
            Report.result(
                screenshot_compare_execution,
                image_diff_result.result_code == azlmbr.utils.ImageDiffResultCode_Success
            )

            if image_diff_result.result_code == azlmbr.utils.ImageDiffResultCode_Success:
                screenshot_compare_result = (
                        "{0} diff score {1} under threshold {2}.".format(
                            screenshot_name,
                            image_diff_result.diff_score,
                            screenshot_threshold),
                        "{0} diff score {1} over threshold {2}.".format(
                            screenshot_name,
                            image_diff_result.diff_score,
                            screenshot_threshold)
                        )
                Report.result(
                    screenshot_compare_result,
                    image_diff_result.diff_score < screenshot_threshold)

        # 17. Look for errors.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomGPU_LightComponent_AreaLightScreenshotsMatchGoldenImages)
