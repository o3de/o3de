"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.asset as asset
import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.entity as EntityId
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestDynamicSliceInstanceSpawnerEmbeddedEditor(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="DynamicSliceInstanceSpawnerEmbeddedEditor", args=["level"])

    def run_test(self):
        """
        Summary:
        A new temporary level is created. Surface for planting is created. Simple vegetation area is created using
        Dynamic Slice Instance Spawner type.

        Expected Behavior:
        Instances plant as expected in the assigned area.

        Test Steps:
         1) Create level
         2) Create a Vegetation Layer Spawner setup using Dynamic Slice Instance Spawner type assets
         3) Create a surface to plant on
         4) Verify expected instance counts
         5) Add a camera component looking at the planting area for visual debugging
         6) Save and export to engine

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        # 1) Create a new, temporary level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        general.set_current_view_position(512.0, 480.0, 38.0)

        # 2) Create a new entity with required vegetation area components and Script Canvas component for launcher test
        center_point = math.Vector3(512.0, 512.0, 32.0)
        asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
        spawner_entity = dynveg.create_vegetation_area("Instance Spawner", center_point, 16.0, 16.0, 1.0, asset_path)
        spawner_entity.add_component("Script Canvas")
        instance_counter_path = os.path.join("scriptcanvas", "instance_counter.scriptcanvas")
        instance_counter_script = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", instance_counter_path,
                                                               math.Uuid(), False)
        spawner_entity.get_set_test(3, "Script Canvas Asset|Script Canvas Asset", instance_counter_script)

        # 3) Create a surface to plant on
        dynveg.create_surface_entity("Planting Surface", center_point, 128.0, 128.0, 1.0)

        # 4) Verify instance counts are accurate
        general.idle_wait(3.0)      # Allow a few seconds for instances to spawn
        num_expected_instances = 20 * 20
        box = azlmbr.shape.ShapeComponentRequestsBus(bus.Event, 'GetEncompassingAabb', spawner_entity.id)
        num_found = azlmbr.areasystem.AreaSystemRequestBus(bus.Broadcast, 'GetInstanceCountInAabb', box)
        self.log(f"Expected {num_expected_instances} instances - Found {num_found} instances")
        self.test_success = self.test_success and num_found == num_expected_instances

        # 5) Create a new entity with a Camera component for testing in the launcher
        cam_position = math.Vector3(512.0, 500.0, 35.0)
        camera_component = ["Camera"]
        new_entity_id2 = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", cam_position, EntityId.EntityId()
        )
        if new_entity_id2.IsValid():
            self.log("Camera entity created")
        camera_entity = hydra.Entity("Camera Entity", new_entity_id2)
        camera_entity.components = []
        for component in camera_component:
            camera_entity.components.append(hydra.add_component(component, new_entity_id2))

        # 6) Save and export to engine
        general.save_level()
        general.idle_wait(1.0)
        general.export_to_engine()
        general.idle_wait(1.0)


test = TestDynamicSliceInstanceSpawnerEmbeddedEditor()
test.run()
