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


def create_basic_atom_level(level_name):
    """
    Creates a new level inside the Editor matching level_name & adds the following:
    1. "default_level" entity to hold all other entities.
    2. Adds Grid, Global Skylight (IBL), ground Mesh, Directional Light, Sphere w/ material+mesh, & Camera components.
    3. Each of these components has its settings tweaked slightly to match the ideal scene to test Atom rendering.
    :param level_name: name of the level to create and apply this basic setup to.
    :return: None
    """
    import azlmbr.asset as asset
    import azlmbr.bus as bus
    import azlmbr.camera as camera
    import azlmbr.editor as editor
    import azlmbr.entity as entity
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.object

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.editor_test_helper import EditorTestHelper

    helper = EditorTestHelper(log_prefix="Atom_EditorTestHelper")

    # Wait for Editor idle loop before executing Python hydra scripts.
    general.idle_enable(True)

    # Basic setup for opened level.
    helper.open_level(level_name="Base")
    general.idle_wait(1.0)
    general.update_viewport()
    general.idle_wait(0.5)  # half a second is more than enough for updating the viewport.

    # Close out problematic windows, FPS meters, and anti-aliasing.
    if general.is_helpers_shown():  # Turn off the helper gizmos if visible
        general.toggle_helpers()
        general.idle_wait(1.0)
    if general.is_pane_visible("Error Report"):  # Close Error Report windows that block focus.
        general.close_pane("Error Report")
    if general.is_pane_visible("Error Log"):  # Close Error Log windows that block focus.
        general.close_pane("Error Log")
    general.idle_wait(1.0)
    general.run_console("r_displayInfo=0")
    general.idle_wait(1.0)

    # Delete all existing entities & create default_level entity
    search_filter = azlmbr.entity.SearchFilter()
    all_entities = entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", search_filter)
    editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", all_entities)
    default_level = hydra.Entity("default_level")
    default_position = math.Vector3(0.0, 0.0, 0.0)
    default_level.create_entity(default_position, ["Grid"])
    default_level.get_set_test(0, "Controller|Configuration|Secondary Grid Spacing", 1.0)

    # Set the viewport up correctly after adding the parent default_level entity.
    screen_width = 1280
    screen_height = 720
    degree_radian_factor = 0.0174533  # Used by "Rotation" property for the Transform component.
    general.set_viewport_size(screen_width, screen_height)
    general.update_viewport()
    helper.wait_for_condition(
        function=lambda: helper.isclose(a=general.get_viewport_size().x, b=screen_width, rel_tol=0.1)
        and helper.isclose(a=general.get_viewport_size().y, b=screen_height, rel_tol=0.1),
        timeout_in_seconds=4.0
    )
    result = helper.isclose(a=general.get_viewport_size().x, b=screen_width, rel_tol=0.1) and helper.isclose(
        a=general.get_viewport_size().y, b=screen_height, rel_tol=0.1)
    general.log(general.get_viewport_size().x)
    general.log(general.get_viewport_size().y)
    general.log(general.get_viewport_size().z)
    general.log(f"Viewport is set to the expected size: {result}")
    general.log("Basic level created")
    general.run_console("r_DisplayInfo = 0")

    # Create global_skylight entity and set the properties
    global_skylight = hydra.Entity("global_skylight")
    global_skylight.create_entity(
        entity_position=default_position,
        components=["HDRi Skybox", "Global Skylight (IBL)"],
        parent_id=default_level.id)
    global_skylight_asset_path = os.path.join("LightingPresets", "default_iblskyboxcm.exr.streamingimage")
    global_skylight_asset_value = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", global_skylight_asset_path, math.Uuid(), False)
    global_skylight.get_set_test(0, "Controller|Configuration|Cubemap Texture", global_skylight_asset_value)
    global_skylight.get_set_test(1, "Controller|Configuration|Diffuse Image", global_skylight_asset_value)
    global_skylight.get_set_test(1, "Controller|Configuration|Specular Image", global_skylight_asset_value)

    # Create ground_plane entity and set the properties
    ground_plane = hydra.Entity("ground_plane")
    ground_plane.create_entity(
        entity_position=default_position,
        components=["Material"],
        parent_id=default_level.id)
    azlmbr.components.TransformBus(azlmbr.bus.Event, "SetLocalUniformScale", ground_plane.id, 32.0)

    # Work around to add the correct Atom Mesh component and asset.
    mesh_type_id = azlmbr.globals.property.EditorMeshComponentTypeId
    ground_plane.components.append(
        editor.EditorComponentAPIBus(
            bus.Broadcast, "AddComponentsOfType", ground_plane.id, [mesh_type_id]
        ).GetValue()[0]
    )
    ground_plane_mesh_asset_path = os.path.join("TestData", "Objects", "plane.azmodel")
    ground_plane_mesh_asset_value = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", ground_plane_mesh_asset_path, math.Uuid(), False)
    ground_plane.get_set_test(1, "Controller|Configuration|Mesh Asset", ground_plane_mesh_asset_value)

    # Add Atom Material component and asset.
    ground_plane_material_asset_path = os.path.join("Materials", "Presets", "PBR", "metal_chrome.azmaterial")
    ground_plane_material_asset_value = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", ground_plane_material_asset_path, math.Uuid(), False)
    ground_plane.get_set_test(0, "Default Material|Material Asset", ground_plane_material_asset_value)

    # Create directional_light entity and set the properties
    directional_light = hydra.Entity("directional_light")
    directional_light.create_entity(
        entity_position=math.Vector3(0.0, 0.0, 10.0),
        components=["Directional Light"],
        parent_id=default_level.id)
    directional_light_rotation = math.Vector3(degree_radian_factor * -90.0, 0.0, 0.0)
    azlmbr.components.TransformBus(
        azlmbr.bus.Event, "SetLocalRotation", directional_light.id, directional_light_rotation)

    # Create sphere entity and set the properties
    sphere_entity = hydra.Entity("sphere")
    sphere_entity.create_entity(
        entity_position=math.Vector3(0.0, 0.0, 1.0),
        components=["Material"],
        parent_id=default_level.id)

    # Work around to add the correct Atom Mesh component and asset.
    sphere_entity.components.append(
        editor.EditorComponentAPIBus(
            bus.Broadcast, "AddComponentsOfType", sphere_entity.id, [mesh_type_id]
        ).GetValue()[0]
    )
    sphere_mesh_asset_path = os.path.join("Models", "sphere.azmodel")
    sphere_mesh_asset_value = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", sphere_mesh_asset_path, math.Uuid(), False)
    sphere_entity.get_set_test(1, "Controller|Configuration|Mesh Asset", sphere_mesh_asset_value)

    # Add Atom Material component and asset.
    sphere_material_asset_path = os.path.join("Materials", "Presets", "PBR", "metal_brass_polished.azmaterial")
    sphere_material_asset_value = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", sphere_material_asset_path, math.Uuid(), False)
    sphere_entity.get_set_test(0, "Default Material|Material Asset", sphere_material_asset_value)

    # Create camera component and set the properties
    camera_entity = hydra.Entity("camera")
    camera_entity.create_entity(
        entity_position=math.Vector3(5.5, -12.0, 9.0),
        components=["Camera"],
        parent_id=default_level.id)
    rotation = math.Vector3(
        degree_radian_factor * -27.0, degree_radian_factor * -12.0, degree_radian_factor * 25.0
    )
    azlmbr.components.TransformBus(azlmbr.bus.Event, "SetLocalRotation", camera_entity.id, rotation)
    camera_entity.get_set_test(0, "Controller|Configuration|Field of view", 60.0)
    camera.EditorCameraViewRequestBus(azlmbr.bus.Event, "ToggleCameraAsActiveView", camera_entity.id)
