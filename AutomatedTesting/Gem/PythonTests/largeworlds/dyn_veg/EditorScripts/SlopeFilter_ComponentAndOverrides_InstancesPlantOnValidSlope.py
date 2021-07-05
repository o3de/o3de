"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C4874096 - Slope Min/Max properties can be set, and properly affect planted vegetation
C4814464 - Slope Filter overrides function as expected
"""

import os
import sys

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, "AutomatedTesting", "Gem", "PythonTests"))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestSlopeFilterComponentAndOverrides(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="SlopeFilter_InstancesPlantOnValidSlope", args=["level"])

    def run_test(self):
        """
        Summary:
        A new level is created. A spawner entity is added, along with a flat planting surface at 32 on Z, and sphere
        mesh at 38 on Z to provide a sloped surface. A Slope Filter is added to the spawner entity, and Slope Min/Max
        values are set. Instance counts are validated. The same test is then performed for Slope Filter overrides.

        Expected Behavior:
        Instances plant only on surfaces that fall between the Slope Filter Min/Max settings

        Test Steps:
         1) Create a new level
         2) Create an instance spawner entity
         3) Create surfaces to plant on, one at 32 on Z and another sloped surface at 38 on Z.
         4) Initial instance counts pre-filter are verified.
         5) Slope Min/Max values are set on the Slope Filter component
         6) Instance counts are validated
         7) Setup for overrides tests
         8) Slope Min/Max values are set on the descriptor overrides
         9) Instance counts are validated

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
        general.set_current_view_position(512.0, 475.0, 38.0)

        # 2) Create a new entity with required vegetation area components
        center_point = math.Vector3(512.0, 512.0, 32.0)
        asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
        spawner_entity = dynveg.create_vegetation_area("Instance Spawner", center_point, 32.0, 32.0, 32.0, asset_path)

        # Add a Vegetation Slope Filter
        spawner_entity.add_component("Vegetation Slope Filter")

        # 3) Add surfaces to plant on. This will include a flat surface and a sphere mesh to provide a sloped surface
        dynveg.create_surface_entity("Planting Surface", center_point, 32.0, 32.0, 1.0)
        sloped_surface_center = math.Vector3(512.0, 512.0, 38.0)
        dynveg.create_mesh_surface_entity_with_slopes("Sloped Planting Surface", sloped_surface_center, 5.0, 5.0, 5.0)

        # Set instances to spawn on a center snap point to avoid unexpected instances around the edges of the box shape
        veg_system_settings_component = hydra.add_level_component("Vegetation System Settings")
        editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", veg_system_settings_component,
                                     'Configuration|Area System Settings|Sector Point Snap Mode', 1)

        # 4) Validate instance counts pre-filter
        num_expected_flat_surface = 40 * 40     # 20x20 instances per 16m
        num_expected_slopes_pre_filter = 120    # Unfiltered planting on the top of the sphere mesh
        num_expected = num_expected_flat_surface + num_expected_slopes_pre_filter
        initial_success = self.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(
            spawner_entity.id, num_expected), 5.0)
        self.test_success = initial_success and self.test_success

        # 5) Change Slope Min/Max on the Vegetation Slope Filter component
        spawner_entity.get_set_test(3, "Configuration|Slope Min", 20)
        spawner_entity.get_set_test(3, "Configuration|Slope Max", 45)

        # 6) Validate instance counts post-filter: instances should only plant on slopes between 20-45 degrees
        num_expected_slopes_post_filter = 44
        slope_min_max_success = self.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(
            spawner_entity.id, num_expected_slopes_post_filter), 5.0)
        self.test_success = slope_min_max_success and self.test_success

        # 7) Setup for overrides on the Slope Filter component and the spawner entity's descriptor
        spawner_entity.get_set_test(3, "Configuration|Allow Per-Item Overrides", True)
        spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Slope Filter|Override Enabled", True)

        # 8) Set Slope Filter Min/Max overrides on the spawner entity's descriptor
        spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Slope Filter|Min", 5)
        spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Slope Filter|Max", 20)

        # 9) Validate instance counts post-filter: instances should only plant on slopes between 5-20 degrees
        num_expected_slopes_post_filter_overrides = 16
        overrides_min_max_success = self.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(
            spawner_entity.id, num_expected_slopes_post_filter_overrides), 5.0)
        self.test_success = overrides_min_max_success and self.test_success


test = TestSlopeFilterComponentAndOverrides()
test.run()
