"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C2627906: A simple Vegetation Layer Blender area can be created
"""

import os
from math import radians
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.asset as asset
import azlmbr.areasystem as areasystem
import azlmbr.legacy.general as general
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.entity as EntityId
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestVegLayerBlenderCreated(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="LayerBlender_E2E_Editor", args=["level"])
        self.screenshot_count = 0

    def run_test(self):
        """
        Summary:
        A temporary level is loaded. Two vegetation areas with different meshes are added and then
        pinned to a vegetation blender. Screenshots are taken in the editor normal mode and in game mode.

        Expected Behavior:
        The specified assets plant in the specified blend area and are visible in the Viewport in
        Edit Mode, Game Mode.

        Test Steps:
         1) Create level
         2) Create 2 vegetation areas with different meshes
         3) Create Blender entity and pin the vegetation areas
         4) Take screenshot in normal mode
         5) Create a new entity with a Camera component for testing in the launcher
         6) Save level and take screenshot in game mode
         7) Export to engine

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        # 1) Create/prepare a new level and set an appropriate view of blender area
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        general.set_current_view_position(500.49, 498.69, 46.66)
        general.set_current_view_rotation(-42.05, 0.00, -36.33)

        # 2) Create 2 vegetation areas with different meshes
        purple_position = math.Vector3(504.0, 512.0, 32.0)
        purple_asset_path = os.path.join("Slices", "PurpleFlower.dynamicslice")
        spawner_entity_1 = dynveg.create_vegetation_area("Purple Spawner",
                                                         purple_position,
                                                         16.0, 16.0, 1.0,
                                                         purple_asset_path)

        pink_position = math.Vector3(520.0, 512.0, 32.0)
        pink_asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
        spawner_entity_2 = dynveg.create_vegetation_area("Pink Spawner",
                                                         pink_position,
                                                         16.0, 16.0, 1.0,
                                                         pink_asset_path)

        base_position = math.Vector3(512.0, 512.0, 32.0)
        dynveg.create_surface_entity("Surface Entity",
                                     base_position,
                                     16.0, 16.0, 1.0)

        hydra.add_level_component("Vegetation Debugger")

        # 3) Create Blender entity and pin the vegetation areas. We also add and attach a Lua script to validate in the
        # launcher for the follow-up test
        blender_entity = hydra.Entity("Blender")
        blender_entity.create_entity(
            base_position,
            ["Box Shape", "Vegetation Layer Blender", "Lua Script"]
        )
        if blender_entity.id.IsValid():
            print(f"'{blender_entity.name}' created")

        blender_entity.get_set_test(0, "Box Shape|Box Configuration|Dimensions", math.Vector3(16.0, 16.0, 1.0))
        blender_entity.get_set_test(1, "Configuration|Vegetation Areas", [spawner_entity_1.id, spawner_entity_2.id])
        instance_counter_path = os.path.join("luascripts", "instance_counter_blender.lua")
        instance_counter_script = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", instance_counter_path,
                                                               math.Uuid(), False)
        blender_entity.get_set_test(2, "Script properties|Asset", instance_counter_script)

        # 4) Verify instances in blender area are equally represented by both descriptors

        # Wait for instances to spawn
        general.run_console('veg_debugClearAllAreas')
        num_expected = 20 * 20
        self.test_success = self.test_success and self.wait_for_condition(
            lambda: dynveg.validate_instance_count(base_position, 8.0, num_expected), 5.0)

        if self.test_success:
            box = math.Aabb_CreateCenterRadius(base_position, 8.0)
            instances = areasystem.AreaSystemRequestBus(bus.Broadcast, 'GetInstancesInAabb', box)
            pink_count = 0
            purple_count = 0
            for instance in instances:
                purple_asset_path = purple_asset_path.replace("\\", "/").lower()
                pink_asset_path = pink_asset_path.replace("\\", "/").lower()
                if instance.descriptor.spawner.GetSliceAssetPath() == pink_asset_path:
                    pink_count += 1
                elif instance.descriptor.spawner.GetSliceAssetPath() == purple_asset_path:
                    purple_count += 1
            self.test_success = pink_count == purple_count and (pink_count + purple_count == num_expected) and self.test_success

        # 5) Create a new entity with a Camera component for testing in the launcher
        entity_position = math.Vector3(500.0, 500.0, 47.0)
        rot_degrees_vector = math.Vector3(radians(-55.0), radians(28.5), radians(-17.0))
        camera_component = ["Camera"]
        camera_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
        )
        if camera_id.IsValid():
            self.log("Camera entity created")
        camera_entity = hydra.Entity("Camera Entity", camera_id)
        camera_entity.components = []
        for component in camera_component:
            camera_entity.components.append(hydra.add_component(component, camera_id))
        azlmbr.components.TransformBus(bus.Event, "SetLocalRotation", camera_id, rot_degrees_vector)

        # 6) Save and export level
        general.save_level()
        general.idle_wait(1.0)
        general.export_to_engine()
        general.idle_wait(1.0)


test = TestVegLayerBlenderCreated()
test.run()
