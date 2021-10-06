"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests :
    camera_component_added =                ("Camera component was added",                  "Camera component wasn't added")
    camera_fov_set =                        ("Camera component FOV property set",           "Camera component FOV property wasn't set")
    directional_light_component_added =     ("Directional Light component added",           "Directional Light component wasn't added")
    enter_game_mode =                       ("Entered game mode",                           "Failed to enter game mode")
    exit_game_mode =                        ("Exited game mode",                            "Couldn't exit game mode")
    global_skylight_component_added =       ("Global Skylight (IBL) component added",       "Global Skylight (IBL) component wasn't added")
    global_skylight_diffuse_image_set =     ("Global Skylight Diffuse Image property set",  "Global Skylight Diffuse Image property wasn't set")
    global_skylight_specular_image_set =    ("Global Skylight Specular Image property set", "Global Skylight Specular Image property wasn't set")
    ground_plane_material_asset_set =       ("Ground Plane Material Asset was set",         "Ground Plane Material Asset wasn't set")
    ground_plane_material_component_added = ("Ground Plane Material component added",       "Ground Plane Material component wasn't added")
    ground_plane_mesh_asset_set =           ("Ground Plane Mesh Asset property was set",    "Ground Plane Mesh Asset property wasn't set")
    hdri_skybox_component_added =           ("HDRi Skybox component added",                 "HDRi Skybox component wasn't added")
    hdri_skybox_cubemap_texture_set =       ("HDRi Skybox Cubemap Texture property set",    "HDRi Skybox Cubemap Texture property wasn't set")
    mesh_component_added =                  ("Mesh component added",                        "Mesh component wasn't added")
    no_assert_occurred =                    ("No asserts detected",                         "Asserts were detected")
    no_error_occurred =                     ("No errors detected",                          "Errors were detected")
    secondary_grid_spacing =                ("Secondary Grid Spacing set",                  "Secondary Grid Spacing not set")
    sphere_material_component_added =       ("Sphere Material component added",             "Sphere Material component wasn't added")
    sphere_material_set =                   ("Sphere Material Asset was set",               "Sphere Material Asset wasn't set")
    sphere_mesh_asset_set =                 ("Sphere Mesh Asset was set",                   "Sphere Mesh Asset wasn't set")
    viewport_set =                          ("Viewport set to correct size",                "Viewport not set to correct size")
# fmt: on


def AtomGPU_BasicLevelSetup_SetsUpLevel():
    """
    Summary:
    Sets up a level to match the AtomBasicLevelSetup.ppm golden image then takes a screenshot to verify the setup.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

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
    13. Add the Mesh component to the Ground Plane Entity and set the Mesh component Mesh Asset property.
    14. Create a Directional Light Entity as a child entity of the Default Level Entity.
    15. Add Directional Light component to Directional Light Entity and set entity rotation.
    16. Create a Sphere Entity as a child entity of the Default Level Entity then add a Material component.
    17. Set the Material Asset property of the Material component for the Sphere Entity.
    18. Add Mesh component to Sphere Entity and set the Mesh Asset property for the Mesh component.
    19. Create a Camera Entity as a child entity of the Default Level Entity then add a Camera component.
    20. Set the Camera Entity rotation value and set the Camera component Field of View value.
    21. Enter game mode.
    22. Take screenshot.
    23. Exit game mode.
    24. Look for errors.

    :return: None
    """

    import os
    from math import isclose

    import azlmbr.asset as asset
    import azlmbr.bus as bus
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.paths

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper as helper

    from Atom.atom_utils.screenshot_utils import ScreenshotHelper

    MATERIAL_COMPONENT_NAME = "Material"
    MESH_COMPONENT_NAME = "Mesh"
    SCREENSHOT_NAME = "AtomBasicLevelSetup"
    SCREEN_WIDTH = 1280
    SCREEN_HEIGHT = 720
    DEGREE_RADIAN_FACTOR = 0.0174533

    def initial_viewport_setup(screen_width, screen_height):
        general.set_viewport_size(screen_width, screen_height)
        general.update_viewport()
        result = isclose(
            a=general.get_viewport_size().x, b=SCREEN_WIDTH, rel_tol=0.1) and isclose(
            a=general.get_viewport_size().y, b=SCREEN_HEIGHT, rel_tol=0.1)

        return result

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        helper.init_idle()
        helper.open_level("", "Base")

        # Test steps begin.
        # 1. Close error windows and display helpers then update the viewport size.
        helper.close_error_windows()
        helper.close_display_helpers()
        general.update_viewport()
        Report.critical_result(Tests.viewport_set, initial_viewport_setup(SCREEN_WIDTH, SCREEN_HEIGHT))

        # 2. Create Default Level Entity.
        default_level_entity_name = "Default Level"
        default_level_entity = EditorEntity.create_editor_entity_at(
            math.Vector3(0.0, 0.0, 0.0), default_level_entity_name)

        # 3. Create Grid Entity as a child entity of the Default Level Entity.
        grid_name = "Grid"
        grid_entity = EditorEntity.create_editor_entity(grid_name, default_level_entity.id)

        # 4. Add Grid component to Grid Entity and set Secondary Grid Spacing.
        grid_component = grid_entity.add_component(grid_name)
        secondary_grid_spacing_property = "Controller|Configuration|Secondary Grid Spacing"
        secondary_grid_spacing_value = 1.0
        grid_component.set_component_property_value(secondary_grid_spacing_property, secondary_grid_spacing_value)
        secondary_grid_spacing_set = grid_component.get_component_property_value(
            secondary_grid_spacing_property) == secondary_grid_spacing_value
        Report.result(Tests.secondary_grid_spacing, secondary_grid_spacing_set)

        # 5. Create Global Skylight (IBL) Entity as a child entity of the Default Level Entity.
        global_skylight_name = "Global Skylight (IBL)"
        global_skylight_entity = EditorEntity.create_editor_entity(global_skylight_name, default_level_entity.id)

        # 6. Add HDRi Skybox component to the Global Skylight (IBL) Entity.
        hdri_skybox_name = "HDRi Skybox"
        hdri_skybox_component = global_skylight_entity.add_component(hdri_skybox_name)
        Report.result(Tests.hdri_skybox_component_added, global_skylight_entity.has_component(hdri_skybox_name))

        # 7. Add Global Skylight (IBL) component to the Global Skylight (IBL) Entity.
        global_skylight_component = global_skylight_entity.add_component(global_skylight_name)
        Report.result(Tests.global_skylight_component_added, global_skylight_entity.has_component(global_skylight_name))

        # 8. Set the Cubemap Texture property of the HDRi Skybox component.
        global_skylight_image_asset_path = os.path.join(
            "LightingPresets", "greenwich_park_02_4k_iblskyboxcm_iblspecular.exr.streamingimage")
        global_skylight_image_asset = asset.AssetCatalogRequestBus(
            bus.Broadcast, "GetAssetIdByPath", global_skylight_image_asset_path, math.Uuid(), False)
        hdri_skybox_cubemap_texture_property = "Controller|Configuration|Cubemap Texture"
        hdri_skybox_component.set_component_property_value(
            hdri_skybox_cubemap_texture_property, global_skylight_image_asset)
        Report.result(
            Tests.hdri_skybox_cubemap_texture_set,
            hdri_skybox_component.get_component_property_value(
                hdri_skybox_cubemap_texture_property) == global_skylight_image_asset)

        # 9. Set the Diffuse Image property of the Global Skylight (IBL) component.
        # Re-use the same image that was used in the previous test step.
        global_skylight_diffuse_image_property = "Controller|Configuration|Diffuse Image"
        global_skylight_component.set_component_property_value(
            global_skylight_diffuse_image_property, global_skylight_image_asset)
        Report.result(
            Tests.global_skylight_diffuse_image_set,
            global_skylight_component.get_component_property_value(
                global_skylight_diffuse_image_property) == global_skylight_image_asset)

        # 10. Set the Specular Image property of the Global Skylight (IBL) component.
        # Re-use the same image that was used in the previous test step.
        global_skylight_specular_image_property = "Controller|Configuration|Specular Image"
        global_skylight_component.set_component_property_value(
            global_skylight_specular_image_property, global_skylight_image_asset)
        global_skylight_specular_image_set = global_skylight_component.get_component_property_value(
            global_skylight_specular_image_property)
        Report.result(
            Tests.global_skylight_specular_image_set, global_skylight_specular_image_set == global_skylight_image_asset)

        # 11. Create a Ground Plane Entity with a Material component that is a child entity of the Default Level Entity.
        ground_plane_name = "Ground Plane"
        ground_plane_entity = EditorEntity.create_editor_entity(ground_plane_name, default_level_entity.id)
        ground_plane_material_component = ground_plane_entity.add_component(MATERIAL_COMPONENT_NAME)
        Report.result(
            Tests.ground_plane_material_component_added, ground_plane_entity.has_component(MATERIAL_COMPONENT_NAME))

        # 12. Set the Material Asset property of the Material component for the Ground Plane Entity.
        ground_plane_entity.set_local_uniform_scale(32.0)
        ground_plane_material_asset_path = os.path.join("Materials", "Presets", "PBR", "metal_chrome.azmaterial")
        ground_plane_material_asset = asset.AssetCatalogRequestBus(
            bus.Broadcast, "GetAssetIdByPath", ground_plane_material_asset_path, math.Uuid(), False)
        ground_plane_material_asset_property = "Default Material|Material Asset"
        ground_plane_material_component.set_component_property_value(
            ground_plane_material_asset_property, ground_plane_material_asset)
        Report.result(
            Tests.ground_plane_material_asset_set,
            ground_plane_material_component.get_component_property_value(
                ground_plane_material_asset_property) == ground_plane_material_asset)

        # 13. Add the Mesh component to the Ground Plane Entity and set the Mesh component Mesh Asset property.
        ground_plane_mesh_component = ground_plane_entity.add_component(MESH_COMPONENT_NAME)
        Report.result(Tests.mesh_component_added, ground_plane_entity.has_component(MESH_COMPONENT_NAME))
        ground_plane_mesh_asset_path = os.path.join("Objects", "plane.azmodel")
        ground_plane_mesh_asset = asset.AssetCatalogRequestBus(
            bus.Broadcast, "GetAssetIdByPath", ground_plane_mesh_asset_path, math.Uuid(), False)
        ground_plane_mesh_asset_property = "Controller|Configuration|Mesh Asset"
        ground_plane_mesh_component.set_component_property_value(
            ground_plane_mesh_asset_property, ground_plane_mesh_asset)
        Report.result(
            Tests.ground_plane_mesh_asset_set,
            ground_plane_mesh_component.get_component_property_value(
                ground_plane_mesh_asset_property) == ground_plane_mesh_asset)

        # 14. Create a Directional Light Entity as a child entity of the Default Level Entity.
        directional_light_name = "Directional Light"
        directional_light_entity = EditorEntity.create_editor_entity_at(
            math.Vector3(0.0, 0.0, 10.0), directional_light_name, default_level_entity.id)

        # 15. Add Directional Light component to Directional Light Entity and set entity rotation.
        directional_light_entity.add_component(directional_light_name)
        directional_light_entity_rotation = math.Vector3(DEGREE_RADIAN_FACTOR * -90.0, 0.0, 0.0)
        directional_light_entity.set_local_rotation(directional_light_entity_rotation)
        Report.result(
            Tests.directional_light_component_added, directional_light_entity.has_component(directional_light_name))

        # 16. Create a Sphere Entity as a child entity of the Default Level Entity then add a Material component.
        sphere_entity = EditorEntity.create_editor_entity_at(
            math.Vector3(0.0, 0.0, 1.0), "Sphere", default_level_entity.id)
        sphere_material_component = sphere_entity.add_component(MATERIAL_COMPONENT_NAME)
        Report.result(Tests.sphere_material_component_added, sphere_entity.has_component(MATERIAL_COMPONENT_NAME))

        # 17. Set the Material Asset property of the Material component for the Sphere Entity.
        sphere_material_asset_path = os.path.join("Materials", "Presets", "PBR", "metal_brass_polished.azmaterial")
        sphere_material_asset = asset.AssetCatalogRequestBus(
            bus.Broadcast, "GetAssetIdByPath", sphere_material_asset_path, math.Uuid(), False)
        sphere_material_asset_property = "Default Material|Material Asset"
        sphere_material_component.set_component_property_value(sphere_material_asset_property, sphere_material_asset)
        Report.result(Tests.sphere_material_set, sphere_material_component.get_component_property_value(
            sphere_material_asset_property) == sphere_material_asset)

        # 18. Add Mesh component to Sphere Entity and set the Mesh Asset property for the Mesh component.
        sphere_mesh_component = sphere_entity.add_component(MESH_COMPONENT_NAME)
        sphere_mesh_asset_path = os.path.join("Models", "sphere.azmodel")
        sphere_mesh_asset = asset.AssetCatalogRequestBus(
            bus.Broadcast, "GetAssetIdByPath", sphere_mesh_asset_path, math.Uuid(), False)
        sphere_mesh_asset_property = "Controller|Configuration|Mesh Asset"
        sphere_mesh_component.set_component_property_value(sphere_mesh_asset_property, sphere_mesh_asset)
        Report.result(Tests.sphere_mesh_asset_set, sphere_mesh_component.get_component_property_value(
            sphere_mesh_asset_property) == sphere_mesh_asset)

        # 19. Create a Camera Entity as a child entity of the Default Level Entity then add a Camera component.
        camera_name = "Camera"
        camera_entity = EditorEntity.create_editor_entity_at(
            math.Vector3(5.5, -12.0, 9.0), camera_name, default_level_entity.id)
        camera_component = camera_entity.add_component(camera_name)
        Report.result(Tests.camera_component_added, camera_entity.has_component(camera_name))

        # 20. Set the Camera Entity rotation value and set the Camera component Field of View value.
        camera_entity_rotation = math.Vector3(
            DEGREE_RADIAN_FACTOR * -27.0, DEGREE_RADIAN_FACTOR * -12.0, DEGREE_RADIAN_FACTOR * 25.0)
        camera_entity.set_local_rotation(camera_entity_rotation)
        camera_fov_property = "Controller|Configuration|Field of view"
        camera_fov_value = 60.0
        camera_component.set_component_property_value(camera_fov_property, camera_fov_value)
        azlmbr.camera.EditorCameraViewRequestBus(azlmbr.bus.Event, "ToggleCameraAsActiveView", camera_entity.id)
        Report.result(Tests.camera_fov_set, camera_component.get_component_property_value(
            camera_fov_property) == camera_fov_value)

        # 21. Enter game mode.
        helper.enter_game_mode(Tests.enter_game_mode)
        helper.wait_for_condition(function=lambda: general.is_in_game_mode(), timeout_in_seconds=4.0)

        # 22. Take screenshot.
        ScreenshotHelper(general.idle_wait_frames).capture_screenshot_blocking(f"{SCREENSHOT_NAME}.ppm")

        # 23. Exit game mode.
        helper.exit_game_mode(Tests.exit_game_mode)
        helper.wait_for_condition(function=lambda: not general.is_in_game_mode(), timeout_in_seconds=4.0)

        # 24. Look for errors.
        helper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        Report.result(Tests.no_assert_occurred, not error_tracer.has_asserts)
        Report.result(Tests.no_error_occurred, not error_tracer.has_errors)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomGPU_BasicLevelSetup_SetsUpLevel)
