"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# This module does a bulk test and update of many components at once.
# Each test case is listed below in the format:
#     "Test Case ID: Test Case Title (URL)"

# C32078130: Tone Mapper (https://testrail.agscollab.com/index.php?/cases/view/32078130)
# C32078129: Light (https://testrail.agscollab.com/index.php?/cases/view/32078129)
# C32078131: Radius Weight Modifier (https://testrail.agscollab.com/index.php?/cases/view/32078131)
# C32078127: PostFX Layer (https://testrail.agscollab.com/index.php?/cases/view/32078127)
# C32078126: Point Light (https://testrail.agscollab.com/index.php?/cases/view/32078126)
# C32078125: Physical Sky (https://testrail.agscollab.com/index.php?/cases/view/32078125)
# C32078115: Global Skylight (IBL) (https://testrail.agscollab.com/index.php?/cases/view/32078115)
# C32078121: Exposure Control (https://testrail.agscollab.com/index.php?/cases/view/32078121)
# C32078120: Directional Light (https://testrail.agscollab.com/index.php?/cases/view/32078120)
# C32078119: DepthOfField (https://testrail.agscollab.com/index.php?/cases/view/32078119)
# C32078118: Decal (https://testrail.agscollab.com/index.php?/cases/view/32078118)
# C32078117: Area Light (https://testrail.agscollab.com/index.php?/cases/view/32078117)

import os
import sys

import azlmbr.math as math
import azlmbr.bus as bus
import azlmbr.paths
import azlmbr.asset as asset
import azlmbr.entity as entity
import azlmbr.legacy.general as general
import azlmbr.editor as editor

sys.path.append(os.path.join(azlmbr.paths.devroot, "AutomatedTesting", "Gem", "PythonTests"))

import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.utils import TestHelper as helper


class TestAllComponentsBasicTests(object):
    """
    Holds shared hydra test functions for this set of tests.
    """


def run():
    """
    Summary:
    The below common tests are done for each of the components.
    1) Addition of component to the entity
    2) UNDO/REDO of addition of component
    3) Enter/Exit game mode
    4) Hide/Show entity containing component
    5) Deletion of component
    6) UNDO/REDO of deletion of component
    Some additional tests for specific components include
    1) Assigning value to some properties of each component
    2) Verifying if the component is activated only when the required components are added

    Expected Result:
    1) Component can be added to an entity.
    2) The addition of component can be undone and redone.
    3) Game mode can be entered/exited without issue.
    4) Entity with component can be hidden/shown.
    5) Component can be deleted.
    6) The deletion of component can be undone and redone.
    7) Component is activated only when the required components are added
    8) Values can be assigned to the properties of the component

    :return: None
    """

    def after_level_load():
        """Function to call after creating/opening a level to ensure it loads."""
        # Give everything a second to initialize.
        general.idle_enable(True)
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
        general.run_console("r_antialiasingmode=0")
        general.idle_wait(1.0)

        return True

    def create_entity_undo_redo_component_addition(component_name):
        new_entity = hydra.Entity(f"{component_name}")
        new_entity.create_entity(math.Vector3(512.0, 512.0, 34.0), [component_name])
        general.log(f"{component_name}_test: Component added to the entity: "
                    f"{hydra.has_components(new_entity.id, [component_name])}")

        # undo component addition
        general.undo()
        helper.wait_for_condition(lambda: not hydra.has_components(new_entity.id, [component_name]), 2.0)
        general.log(f"{component_name}_test: Component removed after UNDO: "
                    f"{not hydra.has_components(new_entity.id, [component_name])}")

        # redo component addition
        general.redo()
        helper.wait_for_condition(lambda: hydra.has_components(new_entity.id, [component_name]), 2.0)
        general.log(f"{component_name}_test: Component added after REDO: "
                    f"{hydra.has_components(new_entity.id, [component_name])}")

        return new_entity

    def verify_enter_exit_game_mode(component_name):
        general.enter_game_mode()
        helper.wait_for_condition(lambda: general.is_in_game_mode(), 1.0)
        general.log(f"{component_name}_test: Entered game mode: {general.is_in_game_mode()}")
        general.exit_game_mode()
        helper.wait_for_condition(lambda: not general.is_in_game_mode(), 1.0)
        general.log(f"{component_name}_test: Exit game mode: {not general.is_in_game_mode()}")

    def verify_hide_unhide_entity(component_name, entity_obj):

        def is_entity_hidden(entity_id):
            return editor.EditorEntityInfoRequestBus(bus.Event, "IsHidden", entity_id)

        editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", entity_obj.id, False)
        general.idle_wait_frames(1)
        general.log(f"{component_name}_test: Entity is hidden: {is_entity_hidden(entity_obj.id)}")
        editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", entity_obj.id, True)
        general.idle_wait_frames(1)
        general.log(f"{component_name}_test: Entity is shown: {not is_entity_hidden(entity_obj.id)}")

    def verify_deletion_undo_redo(component_name, entity_obj):
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntityById", entity_obj.id)
        helper.wait_for_condition(lambda: not hydra.find_entity_by_name(entity_obj.name), 1.0)
        general.log(f"{component_name}_test: Entity deleted: {not hydra.find_entity_by_name(entity_obj.name)}")

        general.undo()
        helper.wait_for_condition(lambda: hydra.find_entity_by_name(entity_obj.name) is not None, 1.0)
        general.log(f"{component_name}_test: UNDO entity deletion works: "
                    f"{hydra.find_entity_by_name(entity_obj.name) is not None}")

        general.redo()
        helper.wait_for_condition(lambda: not hydra.find_entity_by_name(entity_obj.name), 1.0)
        general.log(f"{component_name}_test: REDO entity deletion works: "
                    f"{not hydra.find_entity_by_name(entity_obj.name)}")

    def verify_required_component_addition(entity_obj, components_to_add, component_name):

        def is_component_enabled(entity_componentid_pair):
            return editor.EditorComponentAPIBus(bus.Broadcast, "IsComponentEnabled", entity_componentid_pair)

        general.log(
            f"{component_name}_test: Entity disabled initially: "
            f"{not is_component_enabled(entity_obj.components[0])}")
        for component in components_to_add:
            entity_obj.add_component(component)
        helper.wait_for_condition(lambda: is_component_enabled(entity_obj.components[0]), 1.0)
        general.log(
            f"{component_name}_test: Entity enabled after adding "
            f"required components: {is_component_enabled(entity_obj.components[0])}"
        )

    def verify_set_property(entity_obj, path, value):
        entity_obj.get_set_test(0, path, value)

    # Wait for Editor idle loop before executing Python hydra scripts.
    helper.init_idle()

    # Create a new level.
    new_level_name = "tmp_level"  # Specified in TestAllComponentsBasicTests.py
    heightmap_resolution = 512
    heightmap_meters_per_pixel = 1
    terrain_texture_resolution = 412
    use_terrain = False

    # Return codes are ECreateLevelResult defined in CryEdit.h
    return_code = general.create_level_no_prompt(
        new_level_name, heightmap_resolution, heightmap_meters_per_pixel, terrain_texture_resolution, use_terrain)
    if return_code == 1:
        general.log(f"{new_level_name} level already exists")
    elif return_code == 2:
        general.log("Failed to create directory")
    elif return_code == 3:
        general.log("Directory length is too long")
    elif return_code != 0:
        general.log("Unknown error, failed to create level")
    else:
        general.log(f"{new_level_name} level created successfully")
    after_level_load()

    # Delete all existing entities initially
    search_filter = azlmbr.entity.SearchFilter()
    all_entities = entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", search_filter)
    editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", all_entities)

    class ComponentTests:
        """Test launcher for each component."""
        def __init__(self, component_name, *additional_tests):
            self.component_name = component_name
            self.additional_tests = additional_tests
            self.run_component_tests()

        def run_component_tests(self):
            # Run common and additional tests
            entity_obj = create_entity_undo_redo_component_addition(self.component_name)

            # Enter/Exit game mode test
            verify_enter_exit_game_mode(self.component_name)

            # Any additional tests are executed here
            for test in self.additional_tests:
                test(entity_obj)

            # Hide/Unhide entity test
            verify_hide_unhide_entity(self.component_name, entity_obj)

            # Deletion/Undo/Redo test
            verify_deletion_undo_redo(self.component_name, entity_obj)

    # Area Light Component
    area_light = "Area Light"
    ComponentTests(
        area_light, lambda entity_obj: verify_required_component_addition(
            entity_obj, ["Capsule Shape"], area_light))

    # Decal Component
    material_asset_path = os.path.join("Materials", "decal", "aiirship_nose_number_decal.material")
    material_asset = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", material_asset_path, math.Uuid(), False)
    ComponentTests(
        "Decal", lambda entity_obj: verify_set_property(
            entity_obj, "Settings|Decal Settings|Material", material_asset))

    # DepthOfField Component
    camera_entity = hydra.Entity("camera_entity")
    camera_entity.create_entity(math.Vector3(512.0, 512.0, 34.0), ["Camera"])
    depth_of_field = "DepthOfField"
    ComponentTests(
        depth_of_field,
        lambda entity_obj: verify_required_component_addition(entity_obj, ["PostFX Layer"], depth_of_field),
        lambda entity_obj: verify_set_property(
            entity_obj, "Controller|Configuration|Camera Entity", camera_entity.id))

    # Directional Light Component
    ComponentTests(
        "Directional Light",
        lambda entity_obj: verify_set_property(
            entity_obj, "Controller|Configuration|Shadow|Camera", camera_entity.id))

    # Exposure Control Component
    ComponentTests(
        "Exposure Control", lambda entity_obj: verify_required_component_addition(
            entity_obj, ["PostFX Layer"], "Exposure Control"))

    # Global Skylight (IBL) Component
    diffuse_image_path = os.path.join("LightingPresets", "greenwich_park_02_4k_iblskyboxcm.exr.streamingimage")
    diffuse_image_asset = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", diffuse_image_path, math.Uuid(), False)
    specular_image_path = os.path.join("LightingPresets", "greenwich_park_02_4k_iblskyboxcm.exr.streamingimage")
    specular_image_asset = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", specular_image_path, math.Uuid(), False)
    ComponentTests(
        "Global Skylight (IBL)",
        lambda entity_obj: verify_set_property(
            entity_obj, "Controller|Configuration|Diffuse Image", diffuse_image_asset),
        lambda entity_obj: verify_set_property(
            entity_obj, "Controller|Configuration|Specular Image", specular_image_asset))

    # Physical Sky Component
    ComponentTests("Physical Sky")

    # Point Light Component
    ComponentTests("Point Light")

    # PostFX Layer Component
    ComponentTests("PostFX Layer")

    # Radius Weight Modifier Component
    ComponentTests("Radius Weight Modifier")

    # Spot Light Component
    ComponentTests("Light")


if __name__ == "__main__":
    run()
