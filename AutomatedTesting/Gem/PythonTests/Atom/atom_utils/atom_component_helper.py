"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""
import datetime
import os
import zipfile

from ly_test_tools.image.screenshot_compare_qssim import qssim as compare_screenshots


class ImageComparisonTestFailure(Exception):
    """Custom test failure for failed image comparisons."""
    pass


def create_screenshots_archive(screenshot_path):
    """
    Creates a new zip file archive at archive_path containing all files listed within archive_path.
    :param screenshot_path: location containing the files to archive, the zip archive file will also be saved here.
    :return: path to the created .zip file archive.
    """
    files_to_archive = []

    # Search for .png and .ppm files to add to the zip archive file.
    for (folder_name, sub_folders, file_names) in os.walk(screenshot_path):
        for file_name in file_names:
            if file_name.endswith(".png") or file_name.endswith(".ppm"):
                file_path = os.path.join(folder_name, file_name)
                files_to_archive.append(file_path)

    # Setup variables for naming the zip archive file.
    timestamp = datetime.datetime.now().timestamp()
    formatted_timestamp = datetime.datetime.utcfromtimestamp(timestamp).strftime("%Y-%m-%d_%H-%M-%S")
    screenshots_zip_file = os.path.join(screenshot_path, f'screenshots_{formatted_timestamp}.zip')

    # Write all of the valid .png and .ppm files to the archive file.
    with zipfile.ZipFile(screenshots_zip_file, 'w', compression=zipfile.ZIP_DEFLATED, allowZip64=True) as zip_archive:
        for file_path in files_to_archive:
            file_name = os.path.basename(file_path)
            zip_archive.write(file_path, file_name)

    return screenshots_zip_file


def golden_images_directory():
    """
    Uses this file location to return the valid location for golden image files.
    :return: The path to the golden_images directory, but raises an IOError if the golden_images directory is missing.
    """
    current_file_directory = os.path.join(os.path.dirname(__file__))
    golden_images_dir = os.path.join(current_file_directory, '..', 'golden_images')

    if not os.path.exists(golden_images_dir):
        raise IOError(
            f'golden_images" directory was not found at path "{golden_images_dir}"'
            f'Please add a "golden_images" directory inside: "{current_file_directory}"'
        )

    return golden_images_dir


def compare_screenshot_similarity(
        test_screenshot, golden_image, similarity_threshold, create_zip_archive=False, screenshot_directory=""):
    """
    Compares the similarity between a test screenshot and a golden image.
    It returns a "Screenshots match" string if the comparison mean value is higher than the similarity threshold.
    Otherwise, it returns an error string.
    :param test_screenshot: path to the test screenshot to compare.
    :param golden_image: path to the golden image to compare.
    :param similarity_threshold: value for the comparison mean value to be asserted against.
    :param create_zip_archive: toggle to create a zip archive containing the screenshots if the assert check fails.
    :param screenshot_directory: directory containing screenshots to create zip archive from.
    :return: Error string if compared mean value < similarity threshold or screenshot_directory is missing for .zip,
        otherwise it returns a "Screenshots match" string.
    """
    result = "Screenshots match"
    if create_zip_archive and not screenshot_directory:
        result = 'You must specify a screenshot_directory in order to create a zip archive.\n'

    mean_similarity = compare_screenshots(test_screenshot, golden_image)
    if not mean_similarity > similarity_threshold:
        if create_zip_archive:
            create_screenshots_archive(screenshot_directory)
        result = (
            f"When comparing the test_screenshot: '{test_screenshot}' "
            f"to golden_image: '{golden_image}' the mean similarity of '{mean_similarity}' "
            f"was lower than the similarity threshold of '{similarity_threshold}'. ")

    return result


def compare_screenshot_to_golden_image(
        screenshot_directory, test_screenshots, golden_images, similarity_threshold=0.99):
    """
    Compares a list of test_screenshots to a list of golden_images and return True if they match within the
    similarity threshold set. Otherwise, it will raise ImageComparisonTestFailure with a failure message.
    :param screenshot_directory: path to the directory containing screenshots for creating .zip archives.
    :param test_screenshots: list of test screenshot path strings.
    :param golden_images: list of golden image path strings.
    :param similarity_threshold: float threshold tolerance to set when comparing screenshots to golden images.
    """
    for test_screenshot, golden_image in zip(test_screenshots, golden_images):
        screenshot_comparison_result = compare_screenshot_similarity(
            test_screenshot, golden_image, similarity_threshold, True, screenshot_directory)
        if screenshot_comparison_result != "Screenshots match":
            raise ImageComparisonTestFailure(f"Screenshot test failed: {screenshot_comparison_result}")

    return True


def initial_viewport_setup(screen_width=1280, screen_height=720):
    """
    For setting up the initial viewport resolution to expected default values before running a screenshot test.
    Defaults to 1280 x 720 resolution (in pixels).
    :param screen_width: Width in pixels to set the viewport width size to.
    :param screen_height: Height in pixels to set the viewport height size to.
    :return: None
    """
    import azlmbr.legacy.general as general

    general.set_viewport_size(screen_width, screen_height)
    general.update_viewport()


def enter_exit_game_mode_take_screenshot(screenshot_name, enter_game_tuple, exit_game_tuple, timeout_in_seconds=4):
    """
    Enters game mode, takes a screenshot named screenshot_name (must include file extension), and exits game mode.
    :param screenshot_name: string representing the name of the screenshot file, including file extension.
    :param enter_game_tuple: tuple where the 1st string is success & 2nd string is failure for entering the game.
    :param exit_game_tuple: tuple where the 1st string is success & 2nd string is failure for exiting the game.
    :param timeout_in_seconds: int or float seconds to wait for entering/exiting game mode.
    :return: None
    """
    import azlmbr.legacy.general as general

    from editor_python_test_tools.utils import TestHelper

    from Atom.atom_utils.screenshot_utils import ScreenshotHelper

    TestHelper.enter_game_mode(enter_game_tuple)
    TestHelper.wait_for_condition(function=lambda: general.is_in_game_mode(), timeout_in_seconds=timeout_in_seconds)
    ScreenshotHelper(general.idle_wait_frames).capture_screenshot_blocking(screenshot_name)
    TestHelper.exit_game_mode(exit_game_tuple)
    TestHelper.wait_for_condition(function=lambda: not general.is_in_game_mode(), timeout_in_seconds=timeout_in_seconds)


def create_basic_atom_rendering_scene():
    """
    Sets up a new scene inside the Editor for testing Atom rendering GPU output.
    Setup: Deletes all existing entities before creating the scene.
    The created scene includes:
    1. "Default Level" entity that holds all of the other entities.
    2. "Grid" entity: Contains a Grid component.
    3. "Global Skylight (IBL)" entity: Contains HDRI Skybox & Global Skylight (IBL) components.
    4. "Ground Plane" entity: Contains Material & Mesh components.
    5. "Directional Light" entity: Contains Directional Light component.
    6. "Sphere" entity: Contains Material & Mesh components.
    7. "Camera" entity: Contains Camera component.
    :return: None
    """
    import azlmbr.math as math
    import azlmbr.paths

    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity

    from Atom.atom_utils.atom_constants import AtomComponentProperties

    DEGREE_RADIAN_FACTOR = 0.0174533

    # Setup: Deletes all existing entities before creating the scene.
    search_filter = azlmbr.entity.SearchFilter()
    all_entities = azlmbr.entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", search_filter)
    azlmbr.editor.ToolsApplicationRequestBus(azlmbr.bus.Broadcast, "DeleteEntities", all_entities)

    # 1. "Default Level" entity that holds all of the other entities.
    default_level_entity_name = "Default Level"
    default_level_entity = EditorEntity.create_editor_entity_at(math.Vector3(0.0, 0.0, 0.0), default_level_entity_name)

    # 2. "Grid" entity: Contains a Grid component.
    grid_entity = EditorEntity.create_editor_entity(AtomComponentProperties.grid(), default_level_entity.id)
    grid_component = grid_entity.add_component(AtomComponentProperties.grid())
    secondary_grid_spacing_value = 1.0
    grid_component.set_component_property_value(
        AtomComponentProperties.grid('Secondary Grid Spacing'), secondary_grid_spacing_value)

    # 3. "Global Skylight (IBL)" entity: Contains HDRI Skybox & Global Skylight (IBL) components.
    global_skylight_entity = EditorEntity.create_editor_entity(
        AtomComponentProperties.global_skylight(), default_level_entity.id)
    hdri_skybox_component = global_skylight_entity.add_component(AtomComponentProperties.hdri_skybox())
    global_skylight_component = global_skylight_entity.add_component(AtomComponentProperties.global_skylight())
    global_skylight_image_asset_path = os.path.join("LightingPresets", "default_iblskyboxcm.exr.streamingimage")
    global_skylight_image_asset = Asset.find_asset_by_path(global_skylight_image_asset_path, False)
    hdri_skybox_component.set_component_property_value(
        AtomComponentProperties.hdri_skybox('Cubemap Texture'), global_skylight_image_asset.id)
    global_skylight_diffuse_image_asset_path = os.path.join(
        "LightingPresets", "default_iblskyboxcm_ibldiffuse.exr.streamingimage")
    global_skylight_diffuse_image_asset = Asset.find_asset_by_path(global_skylight_diffuse_image_asset_path, False)
    global_skylight_component.set_component_property_value(
        AtomComponentProperties.global_skylight('Diffuse Image'), global_skylight_diffuse_image_asset.id)
    global_skylight_specular_image_asset_path = os.path.join(
        "LightingPresets", "default_iblskyboxcm_iblspecular.exr.streamingimage")
    global_skylight_specular_image_asset = Asset.find_asset_by_path(
        global_skylight_specular_image_asset_path, False)
    global_skylight_component.set_component_property_value(
        AtomComponentProperties.global_skylight('Specular Image'), global_skylight_specular_image_asset.id)

    # 4. "Ground Plane" entity: Contains Material & Mesh components.
    ground_plane_name = "Ground Plane"
    ground_plane_entity = EditorEntity.create_editor_entity(ground_plane_name, default_level_entity.id)
    ground_plane_material_component = ground_plane_entity.add_component(AtomComponentProperties.material())
    ground_plane_entity.set_local_uniform_scale(32.0)
    ground_plane_mesh_component = ground_plane_entity.add_component(AtomComponentProperties.mesh())
    ground_plane_mesh_asset_path = os.path.join("TestData", "Objects", "plane.azmodel")
    ground_plane_mesh_asset = Asset.find_asset_by_path(ground_plane_mesh_asset_path, False)
    ground_plane_mesh_component.set_component_property_value(
        AtomComponentProperties.mesh('Mesh Asset'), ground_plane_mesh_asset.id)
    ground_plane_material_asset_path = os.path.join("Materials", "Presets", "PBR", "metal_chrome.azmaterial")
    ground_plane_material_asset = Asset.find_asset_by_path(ground_plane_material_asset_path, False)
    ground_plane_material_component.set_component_property_value(
        AtomComponentProperties.material('Material Asset'), ground_plane_material_asset.id)

    # 5. "Directional Light" entity: Contains Directional Light component.
    directional_light_entity = EditorEntity.create_editor_entity_at(
        math.Vector3(0.0, 0.0, 10.0), AtomComponentProperties.directional_light(), default_level_entity.id)
    directional_light_entity.add_component(AtomComponentProperties.directional_light())
    directional_light_entity_rotation = math.Vector3(DEGREE_RADIAN_FACTOR * -90.0, 0.0, 0.0)
    directional_light_entity.set_local_rotation(directional_light_entity_rotation)

    # 6. "Sphere" entity: Contains Material & Mesh components.
    sphere_entity = EditorEntity.create_editor_entity_at(
        math.Vector3(0.0, 0.0, 1.0), "Sphere", default_level_entity.id)
    sphere_mesh_component = sphere_entity.add_component(AtomComponentProperties.mesh())
    sphere_mesh_asset_path = os.path.join("Models", "sphere.azmodel")
    sphere_mesh_asset = Asset.find_asset_by_path(sphere_mesh_asset_path, False)
    sphere_mesh_component.set_component_property_value(
        AtomComponentProperties.mesh('Mesh Asset'), sphere_mesh_asset.id)
    sphere_material_component = sphere_entity.add_component(AtomComponentProperties.material())
    sphere_material_asset_path = os.path.join("Materials", "Presets", "PBR", "metal_brass_polished.azmaterial")
    sphere_material_asset = Asset.find_asset_by_path(sphere_material_asset_path, False)
    sphere_material_component.set_component_property_value(
        AtomComponentProperties.material('Material Asset'), sphere_material_asset.id)

    # 7. "Camera" entity: Contains Camera component.
    camera_entity = EditorEntity.create_editor_entity_at(
        math.Vector3(5.5, -12.0, 9.0), AtomComponentProperties.camera(), default_level_entity.id)
    camera_component = camera_entity.add_component(AtomComponentProperties.camera())
    camera_entity_rotation = math.Vector3(
        DEGREE_RADIAN_FACTOR * -27.0, DEGREE_RADIAN_FACTOR * -12.0, DEGREE_RADIAN_FACTOR * 25.0)
    camera_entity.set_local_rotation(camera_entity_rotation)
    camera_fov_value = 60.0
    camera_component.set_component_property_value(AtomComponentProperties.camera('Field of view'), camera_fov_value)
    azlmbr.camera.EditorCameraViewRequestBus(azlmbr.bus.Event, "ToggleCameraAsActiveView", camera_entity.id)
