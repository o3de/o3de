"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    camera_component_added = (
        "Camera component was added",
        "Camera component wasn't added")
    camera_fov_set = (
        "Camera component FOV property set",
        "Camera component FOV property wasn't set")
    directional_light_component_added = (
        "Directional Light component added",
        "Directional Light component wasn't added")
    enter_game_mode = (
        "Entered game mode",
        "Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "Couldn't exit game mode")
    global_skylight_component_added = (
        "Global Skylight (IBL) component added",
        "Global Skylight (IBL) component wasn't added")
    global_skylight_diffuse_image_set = (
        "Global Skylight Diffuse Image property set",
        "Global Skylight Diffuse Image property wasn't set")
    global_skylight_specular_image_set = (
        "Global Skylight Specular Image property set",
        "Global Skylight Specular Image property wasn't set")
    ground_plane_material_asset_set = (
        "Ground Plane Material Asset was set",
        "Ground Plane Material Asset wasn't set")
    ground_plane_material_component_added = (
        "Ground Plane Material component added",
        "Ground Plane Material component wasn't added")
    ground_plane_model_asset_set = (
        "Ground Plane Model Asset property was set",
        "Ground Plane Model Asset property wasn't set")
    hdri_skybox_component_added = (
        "HDRi Skybox component added",
        "HDRi Skybox component wasn't added")
    hdri_skybox_cubemap_texture_set = (
        "HDRi Skybox Cubemap Texture property set",
        "HDRi Skybox Cubemap Texture property wasn't set")
    mesh_component_added = (
        "Mesh component added",
        "Mesh component wasn't added")
    no_assert_occurred = (
        "No asserts detected",
        "Asserts were detected")
    no_error_occurred = (
        "No errors detected",
        "Errors were detected")
    secondary_grid_spacing = (
        "Secondary Grid Spacing set",
        "Secondary Grid Spacing not set")
    sphere_material_component_added = (
        "Sphere Material component added",
        "Sphere Material component wasn't added")
    sphere_material_set = (
        "Sphere Material Asset was set",
        "Sphere Material Asset wasn't set")
    sphere_model_asset_set = (
        "Sphere Model Asset was set",
        "Sphere Model Asset wasn't set")
    viewport_set = (
        "Viewport set to correct size",
        "Viewport not set to correct size")


def AtomGPU_BasicLevelSetup_SetsUpLevel():
    """
    Summary:
    Sets up a level to match the AtomBasicLevelSetup.ppm golden image then takes a screenshot to verify the setup.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.
    - Deletes all existing entities before creating the scene.

    Expected Behavior:
    The scene can be setup for a basic level.
    The test screenshot matches the appearance of the AtomBasicLevelSetup.ppm golden image.

    Test Steps:
    1. Close error windows and display helpers then update the viewport size.
    2. Create Default Level Entity.
    3. Create Grid Entity as a child entity of the Default Level Entity.
    4. Add Grid component to Grid Entity and set Secondary Grid Spacing.
    5. Create Global Skylight (IBL) Entity as a child entity of the Default Level Entity.
    6. Add HDRi Skybox component to the Global Skylight (IBL) Entity.
    7. Add Global Skylight (IBL) component to the Global Skylight (IBL) Entity.
    8. Set the Cubemap Texture property of the HDRi Skybox component.
    9. Set the Diffuse Image property of the Global Skylight (IBL) component.
    10. Set the Specular Image property of the Global Skylight (IBL) component.
    11. Create a Ground Plane Entity with a Material component that is a child entity of the Default Level Entity.
    12. Set the Material Asset property of the Material component for the Ground Plane Entity.
    13. Add the Mesh component to the Ground Plane Entity and set the Mesh component Model Asset property.
    14. Create a Directional Light Entity as a child entity of the Default Level Entity.
    15. Add Directional Light component to Directional Light Entity and set entity rotation.
    16. Create a Sphere Entity as a child entity of the Default Level Entity then add a Material component.
    17. Set the Material Asset property of the Material component for the Sphere Entity.
    18. Add Mesh component to Sphere Entity and set the Model Asset property for the Mesh component.
    19. Create a Camera Entity as a child entity of the Default Level Entity then add a Camera component.
    20. Set the Camera Entity rotation value and set the Camera component Field of View value.
    21. Enter/Exit game mode taking screenshot.
    22. Compare the screenshots to golden images.
    23. Look for errors.

    :return: None
    """

    import os

    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.paths

    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper

    from Atom.atom_utils.atom_constants import AtomComponentProperties
    from Atom.atom_utils.atom_component_helper import initial_viewport_setup
    from Atom.atom_utils.atom_component_helper import (
        enter_exit_game_mode_take_screenshot, compare_screenshot_to_golden_image)

    from Atom.atom_utils.screenshot_utils import (FOLDER_PATH, screenshot_compare_result_code_to_string)

    DEGREE_RADIAN_FACTOR = 0.0174533

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("", "Base")

        # Setup: Deletes all existing entities before creating the scene.
        search_filter = azlmbr.entity.SearchFilter()
        all_entities = azlmbr.entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", search_filter)
        azlmbr.editor.ToolsApplicationRequestBus(azlmbr.bus.Broadcast, "DeleteEntities", all_entities)

        # Setup: Define the screenshot names and threshold pairs
        screenshot_thresholds = {
            "AtomBasicLevelSetup.png" : 0.02
        }

        # Test steps begin.
        # 1. Close error windows and display helpers then update the viewport size.
        TestHelper.close_error_windows()
        TestHelper.close_display_helpers()
        initial_viewport_setup()
        general.update_viewport()

        # 2. Create Default Level Entity.
        default_level_entity_name = "Default Level"
        default_level_entity = EditorEntity.create_editor_entity_at(
            math.Vector3(0.0, 0.0, 0.0), default_level_entity_name)

        # 3. Create Grid Entity as a child entity of the Default Level Entity.
        grid_entity = EditorEntity.create_editor_entity(AtomComponentProperties.grid(), default_level_entity.id)

        # 4. Add Grid component to Grid Entity and set Secondary Grid Spacing.
        grid_component = grid_entity.add_component(AtomComponentProperties.grid())
        secondary_grid_spacing_value = 1.0
        grid_component.set_component_property_value(
            AtomComponentProperties.grid('Secondary Grid Spacing'), secondary_grid_spacing_value)
        secondary_grid_spacing_set = grid_component.get_component_property_value(
            AtomComponentProperties.grid('Secondary Grid Spacing')) == secondary_grid_spacing_value
        Report.result(Tests.secondary_grid_spacing, secondary_grid_spacing_set)

        # 5. Create Global Skylight (IBL) Entity as a child entity of the Default Level Entity.
        global_skylight_entity = EditorEntity.create_editor_entity(
            AtomComponentProperties.global_skylight(), default_level_entity.id)

        # 6. Add HDRi Skybox component to the Global Skylight (IBL) Entity.
        hdri_skybox_component = global_skylight_entity.add_component(AtomComponentProperties.hdri_skybox())
        Report.result(Tests.hdri_skybox_component_added, global_skylight_entity.has_component(
            AtomComponentProperties.hdri_skybox()))

        # 7. Add Global Skylight (IBL) component to the Global Skylight (IBL) Entity.
        global_skylight_component = global_skylight_entity.add_component(AtomComponentProperties.global_skylight())
        Report.result(Tests.global_skylight_component_added, global_skylight_entity.has_component(
            AtomComponentProperties.global_skylight()))

        # 8. Set the Cubemap Texture property of the HDRi Skybox component.
        global_skylight_image_asset_path = os.path.join("lightingpresets", "default_iblskyboxcm.exr.streamingimage")
        global_skylight_image_asset = Asset.find_asset_by_path(global_skylight_image_asset_path, False)
        hdri_skybox_component.set_component_property_value(
            AtomComponentProperties.hdri_skybox('Cubemap Texture'), global_skylight_image_asset.id)
        Report.result(
            Tests.hdri_skybox_cubemap_texture_set,
            hdri_skybox_component.get_component_property_value(
                AtomComponentProperties.hdri_skybox('Cubemap Texture')) == global_skylight_image_asset.id)

        # 9. Set the Diffuse Image property of the Global Skylight (IBL) component.
        # Re-use the same image that was used in the previous test step.
        global_skylight_diffuse_image_asset_path = os.path.join(
            "lightingpresets", "default_iblskyboxcm_ibldiffuse.exr.streamingimage")
        global_skylight_diffuse_image_asset = Asset.find_asset_by_path(global_skylight_diffuse_image_asset_path, False)
        global_skylight_component.set_component_property_value(
            AtomComponentProperties.global_skylight('Diffuse Image'), global_skylight_diffuse_image_asset.id)
        Report.result(
            Tests.global_skylight_diffuse_image_set,
            global_skylight_component.get_component_property_value(
                AtomComponentProperties.global_skylight('Diffuse Image')) == global_skylight_diffuse_image_asset.id)

        # 10. Set the Specular Image property of the Global Skylight (IBL) component.
        # Re-use the same image that was used in the previous test step.
        global_skylight_specular_image_asset_path = os.path.join(
            "lightingpresets", "default_iblskyboxcm_iblspecular.exr.streamingimage")
        global_skylight_specular_image_asset = Asset.find_asset_by_path(
            global_skylight_specular_image_asset_path, False)
        global_skylight_component.set_component_property_value(
            AtomComponentProperties.global_skylight('Specular Image'), global_skylight_specular_image_asset.id)
        global_skylight_specular_image_set = global_skylight_component.get_component_property_value(
            AtomComponentProperties.global_skylight('Specular Image'))
        Report.result(
            Tests.global_skylight_specular_image_set,
            global_skylight_specular_image_set == global_skylight_specular_image_asset.id)

        # 11. Create a Ground Plane Entity with a Material component that is a child entity of the Default Level Entity.
        ground_plane_name = "Ground Plane"
        ground_plane_entity = EditorEntity.create_editor_entity(ground_plane_name, default_level_entity.id)
        ground_plane_material_component = ground_plane_entity.add_component(AtomComponentProperties.material())
        Report.result(
            Tests.ground_plane_material_component_added,
            ground_plane_entity.has_component(AtomComponentProperties.material()))

        # 12. Set the Material Asset property of the Material component for the Ground Plane Entity.
        ground_plane_entity.set_local_uniform_scale(32.0)
        ground_plane_material_asset_path = os.path.join("materials", "presets", "pbr", "metal_chrome.azmaterial")
        ground_plane_material_asset = Asset.find_asset_by_path(ground_plane_material_asset_path, False)
        ground_plane_material_component.set_component_property_value(
            AtomComponentProperties.material('Material Asset'), ground_plane_material_asset.id)
        Report.result(
            Tests.ground_plane_material_asset_set,
            ground_plane_material_component.get_component_property_value(
                AtomComponentProperties.material('Material Asset')) == ground_plane_material_asset.id)

        # 13. Add the Mesh component to the Ground Plane Entity and set the Mesh component Model Asset property.
        ground_plane_mesh_component = ground_plane_entity.add_component(AtomComponentProperties.mesh())
        Report.result(Tests.mesh_component_added, ground_plane_entity.has_component(AtomComponentProperties.mesh()))
        ground_plane_model_asset_path = os.path.join("testdata", "objects", "plane.azmodel")
        ground_plane_model_asset = Asset.find_asset_by_path(ground_plane_model_asset_path, False)
        ground_plane_mesh_component.set_component_property_value(
            AtomComponentProperties.mesh('Model Asset'), ground_plane_model_asset.id)
        Report.result(
            Tests.ground_plane_model_asset_set,
            ground_plane_mesh_component.get_component_property_value(
                AtomComponentProperties.mesh('Model Asset')) == ground_plane_model_asset.id)

        # 14. Create a Directional Light Entity as a child entity of the Default Level Entity.
        directional_light_entity = EditorEntity.create_editor_entity_at(
            math.Vector3(0.0, 0.0, 10.0), AtomComponentProperties.directional_light(), default_level_entity.id)

        # 15. Add Directional Light component to Directional Light Entity and set entity rotation.
        directional_light_entity.add_component(AtomComponentProperties.directional_light())
        directional_light_entity_rotation = math.Vector3(DEGREE_RADIAN_FACTOR * -90.0, 0.0, 0.0)
        directional_light_entity.set_local_rotation(directional_light_entity_rotation)
        Report.result(
            Tests.directional_light_component_added, directional_light_entity.has_component(
                AtomComponentProperties.directional_light()))

        # 16. Create a Sphere Entity as a child entity of the Default Level Entity then add a Material component.
        sphere_entity = EditorEntity.create_editor_entity_at(
            math.Vector3(0.0, 0.0, 1.0), "Sphere", default_level_entity.id)
        sphere_material_component = sphere_entity.add_component(AtomComponentProperties.material())
        Report.result(Tests.sphere_material_component_added, sphere_entity.has_component(
            AtomComponentProperties.material()))

        # 17. Set the Material Asset property of the Material component for the Sphere Entity.
        sphere_material_asset_path = os.path.join("materials", "presets", "pbr", "metal_brass_polished.azmaterial")
        sphere_material_asset = Asset.find_asset_by_path(sphere_material_asset_path, False)
        sphere_material_component.set_component_property_value(
            AtomComponentProperties.material('Material Asset'), sphere_material_asset.id)
        Report.result(Tests.sphere_material_set, sphere_material_component.get_component_property_value(
            AtomComponentProperties.material('Material Asset')) == sphere_material_asset.id)

        # 18. Add Mesh component to Sphere Entity and set the Model Asset property for the Mesh component.
        sphere_mesh_component = sphere_entity.add_component(AtomComponentProperties.mesh())
        sphere_model_asset_path = os.path.join("models", "sphere.azmodel")
        sphere_model_asset = Asset.find_asset_by_path(sphere_model_asset_path, False)
        sphere_mesh_component.set_component_property_value(
            AtomComponentProperties.mesh('Model Asset'), sphere_model_asset.id)
        Report.result(Tests.sphere_model_asset_set, sphere_mesh_component.get_component_property_value(
            AtomComponentProperties.mesh('Model Asset')) == sphere_model_asset.id)

        # 19. Create a Camera Entity as a child entity of the Default Level Entity then add a Camera component.
        camera_entity = EditorEntity.create_editor_entity_at(
            math.Vector3(5.5, -12.0, 9.0), AtomComponentProperties.camera(), default_level_entity.id)
        camera_component = camera_entity.add_component(AtomComponentProperties.camera())
        Report.result(Tests.camera_component_added, camera_entity.has_component(AtomComponentProperties.camera()))

        # 20. Set the Camera Entity rotation value and set the Camera component Field of View value.
        camera_entity_rotation = math.Vector3(
            DEGREE_RADIAN_FACTOR * -27.0, DEGREE_RADIAN_FACTOR * -12.0, DEGREE_RADIAN_FACTOR * 25.0)
        camera_entity.set_local_rotation(camera_entity_rotation)
        camera_fov_value = 60.0
        camera_component.set_component_property_value(AtomComponentProperties.camera('Field of view'), camera_fov_value)
        azlmbr.camera.EditorCameraViewRequestBus(azlmbr.bus.Event, "ToggleCameraAsActiveView", camera_entity.id)
        Report.result(Tests.camera_fov_set, camera_component.get_component_property_value(
            AtomComponentProperties.camera('Field of view')) == camera_fov_value)

        # 21. Enter/Exit game mode taking screenshot.
        enter_exit_game_mode_take_screenshot("AtomBasicLevelSetup.png", Tests.enter_game_mode, Tests.exit_game_mode)

        # 22. Compare the screenshots to golden images.
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

        # 23. Look for errors.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomGPU_BasicLevelSetup_SetsUpLevel)
