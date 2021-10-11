"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os
import sys

import azlmbr.asset as asset
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.paths
import azlmbr.legacy.general as general

sys.path.append(os.path.join(azlmbr.paths.projectroot, "Gem", "PythonTests"))

import editor_python_test_tools.hydra_editor_utils as hydra
from Atom.atom_utils import atom_component_helper, atom_constants, screenshot_utils
from editor_python_test_tools.editor_test_helper import EditorTestHelper

helper = EditorTestHelper(log_prefix="Atom_EditorTestHelper")

LEVEL_NAME = "auto_test"
LIGHT_COMPONENT = "Light"
LIGHT_TYPE_PROPERTY = 'Controller|Configuration|Light type'
DEGREE_RADIAN_FACTOR = 0.0174533


def run():
    """
    Sets up the tests by making sure the required level is created & setup correctly.
    It then executes 2 test cases - see each associated test function's docstring for more info.

    Finally prints the string "Light component tests completed" after completion

    Tests will fail immediately if any of these log lines are found:
    1. Trace::Assert
    2. Trace::Error
    3. Traceback (most recent call last):

    :return: None
    """
    atom_component_helper.create_basic_atom_level(level_name=LEVEL_NAME)

    # Run tests.
    area_light_test()
    spot_light_test()
    general.log("Light component tests completed.")


def area_light_test():
    """
    Basic test for the "Light" component attached to an "area_light" entity.

    Test Case - Light Component: Capsule, Spot (disk), and Point (sphere):
    1. Creates "area_light" entity w/ a Light component that has a Capsule Light type w/ the color set to 255, 0, 0
    2. Enters game mode to take a screenshot for comparison, then exits game mode.
    3. Sets the Light component Intensity Mode to Lumens (default).
    4. Ensures the Light component Mode is Automatic (default).
    5. Sets the Intensity value of the Light component to 0.0
    6. Enters game mode again, takes another screenshot for comparison, then exits game mode.
    7. Updates the Intensity value of the Light component to 1000.0
    8. Enters game mode again, takes another screenshot for comparison, then exits game mode.
    9. Swaps the Capsule light type option to Spot (disk) light type on the Light component
    10. Updates "area_light" entity Transform rotate value to x: 90.0, y:0.0, z:0.0
    11. Enters game mode again, takes another screenshot for comparison, then exits game mode.
    12. Swaps the Spot (disk) light type for the Point (sphere) light type in the Light component.
    13. Enters game mode again, takes another screenshot for comparison, then exits game mode.
    14. Deletes the Light component from the "area_light" entity and verifies its successful.
    """
    # Create an "area_light" entity with "Light" component using Light type of "Capsule"
    area_light_entity_name = "area_light"
    area_light = hydra.Entity(area_light_entity_name)
    area_light.create_entity(math.Vector3(-1.0, -2.0, 3.0), [LIGHT_COMPONENT])
    general.log(
        f"{area_light_entity_name}_test: Component added to the entity: "
        f"{hydra.has_components(area_light.id, [LIGHT_COMPONENT])}")
    light_component_id_pair = hydra.attach_component_to_entity(area_light.id, LIGHT_COMPONENT)

    # Select the "Capsule" light type option.
    azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast,
        'SetComponentProperty',
        light_component_id_pair,
        LIGHT_TYPE_PROPERTY,
        atom_constants.LIGHT_TYPES['capsule']
    )

    # Update color and take screenshot in game mode
    color = math.Color(255.0, 0.0, 0.0, 0.0)
    area_light.get_set_test(0, "Controller|Configuration|Color", color)
    general.idle_wait(1.0)
    screenshot_utils.take_screenshot_game_mode("AreaLight_1", area_light_entity_name)

    # Update intensity value to 0.0 and take screenshot in game mode
    area_light.get_set_test(0, "Controller|Configuration|Attenuation Radius|Mode", 1)
    area_light.get_set_test(0, "Controller|Configuration|Intensity", 0.0)
    general.idle_wait(1.0)
    screenshot_utils.take_screenshot_game_mode("AreaLight_2", area_light_entity_name)

    # Update intensity value to 1000.0 and take screenshot in game mode
    area_light.get_set_test(0, "Controller|Configuration|Intensity", 1000.0)
    general.idle_wait(1.0)
    screenshot_utils.take_screenshot_game_mode("AreaLight_3", area_light_entity_name)

    # Swap the "Capsule" light type option to "Spot (disk)" light type
    azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast,
        'SetComponentProperty',
        light_component_id_pair,
        LIGHT_TYPE_PROPERTY,
        atom_constants.LIGHT_TYPES['spot_disk']
    )
    area_light_rotation = math.Vector3(DEGREE_RADIAN_FACTOR * 90.0, 0.0, 0.0)
    azlmbr.components.TransformBus(azlmbr.bus.Event, "SetLocalRotation", area_light.id, area_light_rotation)
    general.idle_wait(1.0)
    screenshot_utils.take_screenshot_game_mode("AreaLight_4", area_light_entity_name)

    # Swap the "Spot (disk)" light type to the "Point (sphere)" light type and take screenshot.
    azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast,
        'SetComponentProperty',
        light_component_id_pair,
        LIGHT_TYPE_PROPERTY,
        atom_constants.LIGHT_TYPES['sphere']
    )
    general.idle_wait(1.0)
    screenshot_utils.take_screenshot_game_mode("AreaLight_5", area_light_entity_name)

    editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntityById", area_light.id)


def spot_light_test():
    """
    Basic test for the Light component attached to a "spot_light" entity.

    Test Case - Light Component: Spot (disk) with shadows & colors:
    1. Creates "spot_light" entity w/ a Light component attached to it.
    2. Selects the "directional_light" entity already present in the level and disables it.
    3. Selects the "global_skylight" entity already present in the level and disables the HDRi Skybox component,
        as well as the Global Skylight (IBL) component.
    4. Enters game mode to take a screenshot for comparison, then exits game mode.
    5. Selects the "ground_plane" entity and changes updates the material to a new material.
    6. Enters game mode to take a screenshot for comparison, then exits game mode.
    7. Selects the "spot_light" entity and increases the Light component Intensity to 800 lm
    8. Enters game mode to take a screenshot for comparison, then exits game mode.
    9. Selects the "spot_light" entity and sets the Light component Color to 47, 75, 37
    10. Enters game mode to take a screenshot for comparison, then exits game mode.
    11. Selects the "spot_light" entity and modifies the Shutter controls to the following values:
        - Enable shutters: True
        - Inner Angle: 60.0
        - Outer Angle: 75.0
    12. Enters game mode to take a screenshot for comparison, then exits game mode.
    13. Selects the "spot_light" entity and modifies the Shadow controls to the following values:
        - Enable Shadow: True
        - ShadowmapSize: 256
    14. Modifies the world translate position of the "spot_light" entity to 0.7, -2.0, 1.9 (for casting shadows better)
    15. Enters game mode to take a screenshot for comparison, then exits game mode.
    """
    # Disable "Directional Light" component for the "directional_light" entity
    # "directional_light" entity is created by the create_basic_atom_level() function by default.
    directional_light_entity_id = hydra.find_entity_by_name("directional_light")
    directional_light = hydra.Entity(name='directional_light', id=directional_light_entity_id)
    directional_light_component_type = azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Directional Light"], 0)[0]
    directional_light_component = azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 'GetComponentOfType', directional_light.id, directional_light_component_type
    ).GetValue()
    editor.EditorComponentAPIBus(bus.Broadcast, "DisableComponents", [directional_light_component])
    general.idle_wait(0.5)

    # Disable "Global Skylight (IBL)" and "HDRi Skybox" components for the "global_skylight" entity
    global_skylight_entity_id = hydra.find_entity_by_name("global_skylight")
    global_skylight = hydra.Entity(name='global_skylight', id=global_skylight_entity_id)
    global_skylight_component_type = azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Global Skylight (IBL)"], 0)[0]
    global_skylight_component = azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 'GetComponentOfType', global_skylight.id, global_skylight_component_type
    ).GetValue()
    editor.EditorComponentAPIBus(bus.Broadcast, "DisableComponents", [global_skylight_component])
    hdri_skybox_component_type = azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["HDRi Skybox"], 0)[0]
    hdri_skybox_component = azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 'GetComponentOfType', global_skylight.id, hdri_skybox_component_type
    ).GetValue()
    editor.EditorComponentAPIBus(bus.Broadcast, "DisableComponents", [hdri_skybox_component])
    general.idle_wait(0.5)

    # Create a "spot_light" entity with "Light" component using Light Type of "Spot (disk)"
    spot_light_entity_name = "spot_light"
    spot_light = hydra.Entity(spot_light_entity_name)
    spot_light.create_entity(math.Vector3(0.7, -2.0, 1.0), [LIGHT_COMPONENT])
    general.log(
        f"{spot_light_entity_name}_test: Component added to the entity: "
        f"{hydra.has_components(spot_light.id, [LIGHT_COMPONENT])}")
    rotation = math.Vector3(DEGREE_RADIAN_FACTOR * 300.0, 0.0, 0.0)
    azlmbr.components.TransformBus(azlmbr.bus.Event, "SetLocalRotation", spot_light.id, rotation)
    light_component_type = hydra.attach_component_to_entity(spot_light.id, LIGHT_COMPONENT)
    editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast,
        'SetComponentProperty',
        light_component_type,
        LIGHT_TYPE_PROPERTY,
        atom_constants.LIGHT_TYPES['spot_disk']
    )

    general.idle_wait(1.0)
    screenshot_utils.take_screenshot_game_mode("SpotLight_1", spot_light_entity_name)

    # Change default material of ground plane entity and take screenshot
    ground_plane_entity_id = hydra.find_entity_by_name("ground_plane")
    ground_plane = hydra.Entity(name='ground_plane', id=ground_plane_entity_id)
    ground_plane_asset_path = os.path.join("Materials", "Presets", "MacBeth", "22_neutral_5-0_0-70d.azmaterial")
    ground_plane_asset_value = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", ground_plane_asset_path, math.Uuid(), False)
    material_property_path = "Default Material|Material Asset"
    material_component_type = azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Material"], 0)[0]
    material_component = azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 'GetComponentOfType', ground_plane.id, material_component_type).GetValue()
    editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast,
        'SetComponentProperty',
        material_component,
        material_property_path,
        ground_plane_asset_value
    )
    general.idle_wait(1.0)
    screenshot_utils.take_screenshot_game_mode("SpotLight_2", spot_light_entity_name)

    # Increase intensity value of the Spot light and take screenshot in game mode
    spot_light.get_set_test(0, "Controller|Configuration|Intensity", 800.0)
    general.idle_wait(1.0)
    screenshot_utils.take_screenshot_game_mode("SpotLight_3", spot_light_entity_name)

    # Update the Spot light color and take screenshot in game mode
    color_value = math.Color(47.0 / 255.0, 75.0 / 255.0, 37.0 / 255.0, 255.0 / 255.0)
    spot_light.get_set_test(0, "Controller|Configuration|Color", color_value)
    general.idle_wait(1.0)
    screenshot_utils.take_screenshot_game_mode("SpotLight_4", spot_light_entity_name)

    # Update the Shutter controls of the Light component and take screenshot
    spot_light.get_set_test(0, "Controller|Configuration|Shutters|Enable shutters", True)
    spot_light.get_set_test(0, "Controller|Configuration|Shutters|Inner angle", 60.0)
    spot_light.get_set_test(0, "Controller|Configuration|Shutters|Outer angle", 75.0)
    general.idle_wait(1.0)
    screenshot_utils.take_screenshot_game_mode("SpotLight_5", spot_light_entity_name)

    # Update the Shadow controls, move the spot_light entity world translate position and take screenshot
    spot_light.get_set_test(0, "Controller|Configuration|Shadows|Enable shadow", True)
    spot_light.get_set_test(0, "Controller|Configuration|Shadows|Shadowmap size", 256.0)
    azlmbr.components.TransformBus(
        azlmbr.bus.Event, "SetWorldTranslation", spot_light.id, math.Vector3(0.7, -2.0, 1.9))
    general.idle_wait(1.0)
    screenshot_utils.take_screenshot_game_mode("SpotLight_6", spot_light_entity_name)


if __name__ == "__main__":
    run()
