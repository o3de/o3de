"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.legacy.general as general
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, "AutomatedTesting", "Gem", "PythonTests"))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestLayerBlocker(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="LayerBlocker_InstancesBlocked", args=["level"])

    def run_test(self):
        """
        Summary:
        An empty level is created. A Vegetation Layer Spawner area is configured. A Vegetation Layer Blocker area is
        configured to block instances in the spawner area.

        Expected Behavior:
        Vegetation is blocked by the configured Blocker area.

        Test Steps:
        1. A new level is created
        2. Vegetation Layer Spawner area is created
        3. Planting surface is created
        4. Vegetation System Settings level component is added, and Snap Mode set to center to ensure expected instance
        counts are accurate in the configured vegetation area
        5. Initial instance counts pre-blocker are validated
        6. A Vegetation Layer Blocker area is created, overlapping the spawner area
        7. Post-blocker instance counts are validated

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

        # Set view of planting area for visual debugging
        general.set_current_view_position(512.0, 500.0, 38.0)
        general.set_current_view_rotation(-20.0, 0.0, 0.0)

        # 2) Create a new instance spawner entity
        spawner_center_point = math.Vector3(512.0, 512.0, 32.0)
        asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
        spawner_entity = dynveg.create_vegetation_area("Instance Spawner", spawner_center_point, 16.0, 16.0, 16.0,
                                                       asset_path)

        # 3) Create surface for planting on
        dynveg.create_surface_entity("Surface Entity", spawner_center_point, 32.0, 32.0, 1.0)

        # 4) Add a Vegetation System Settings Level component and set Sector Point Snap Mode to Center
        veg_system_settings_component = hydra.add_level_component("Vegetation System Settings")
        editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", veg_system_settings_component,
                                     'Configuration|Area System Settings|Sector Point Snap Mode', 1)

        # 5) Verify initial instance counts
        num_expected = 20 * 20
        success = self.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                                 num_expected), 5.0)
        self.test_success = success and self.test_success

        # 6) Create a new Vegetation Layer Blocker area overlapping the spawner area
        blocker_entity = hydra.Entity("Blocker Area")
        blocker_entity.create_entity(
            spawner_center_point,
            ["Vegetation Layer Blocker", "Box Shape"]
        )
        if blocker_entity.id.IsValid():
            print(f"'{blocker_entity.name}' created")
        blocker_entity.get_set_test(1, "Box Shape|Box Configuration|Dimensions",
                                    math.Vector3(3.0, 3.0, 3.0))

        # 7) Validate instance counts post-blocker. 16 instances should now be blocked in the center of the spawner area
        num_expected = (20 * 20) - 16
        success = self.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                                 num_expected), 5.0)
        self.test_success = success and self.test_success


test = TestLayerBlocker()
test.run()
