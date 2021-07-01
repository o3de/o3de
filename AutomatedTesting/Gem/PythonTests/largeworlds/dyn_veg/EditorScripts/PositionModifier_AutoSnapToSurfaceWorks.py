"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.bus as bus
import azlmbr.legacy.general as general
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestPositionModifierAutoSnapToSurface(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="PositionModifier_AutoSnapToSurface", args=["level"])

    def run_test(self):
        """
        Summary:
        Instance spawner is setup to plant on a spherical mesh. Offsets are set on the x-axis, and checks are performed
        to ensure instances plant where expected depending on the toggle setting.

        Expected Behavior:
        Offset instances snap to the expected surface when Auto Snap to Surface is enabled, and offset away from surface
        when it is disabled.

        Test Steps:
         1) Create a new, temporary level
         2) Create a new entity with required vegetation area components and a Position Modifier
         3) Create a spherical planting surface
         4) Verify initial instance counts pre-filter
         5) Create a child entity of the spawner entity with a Constant Gradient component and pin to spawner
         6) Set the Position Modifier offset to 5 on the x-axis
         7) Validate instance counts on top of and inside the sphere mesh with Auto Snap to Surface enabled
         8) Validate instance counts on top of and inside the sphere mesh with Auto Snap to Surface disabled

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        position_modifier_paths = ['Configuration|Position X|Range Min', 'Configuration|Position X|Range Max',
                                   'Configuration|Position Y|Range Min', 'Configuration|Position Y|Range Max',
                                   'Configuration|Position Z|Range Min', 'Configuration|Position Z|Range Max']

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

        # 2) Create a new entity with required vegetation area components and a Position Modifier
        spawner_center_point = math.Vector3(512.0, 512.0, 32.0)
        asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
        spawner_entity = dynveg.create_vegetation_area("Instance Spawner", spawner_center_point, 16.0, 16.0, 16.0,
                                                       asset_path)

        # Add a Vegetation Position Modifier and set offset values to 0
        spawner_entity.add_component("Vegetation Position Modifier")
        for path in position_modifier_paths:
            spawner_entity.get_set_test(3, path, 0)

        # 3) Create a spherical planting surface and a flat surface
        flat_entity = dynveg.create_surface_entity("Flat Surface", spawner_center_point, 32.0, 32.0, 1.0)
        hill_entity = dynveg.create_mesh_surface_entity_with_slopes("Planting Surface", spawner_center_point, 5.0, 5.0, 5.0)

        # Disable/Re-enable Mesh component due to ATOM-14299
        general.idle_wait(1.0)
        editor.EditorComponentAPIBus(bus.Broadcast, 'DisableComponents', [hill_entity.components[0]])
        is_enabled = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', hill_entity.components[0])
        if is_enabled:
            print("Mesh component is still enabled")
        else:
            print("Mesh component was disabled")
        editor.EditorComponentAPIBus(bus.Broadcast, 'EnableComponents', [hill_entity.components[0]])
        is_enabled = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', hill_entity.components[0])
        if is_enabled:
            print("Mesh component is now enabled")
        else:
            print("Mesh component is still disabled")

        # Disable the Flat Surface Box Shape component, and temporarily ignore initial instance counts due to LYN-2245
        editor.EditorComponentAPIBus(bus.Broadcast, 'DisableComponents', [flat_entity.components[0]])
        """
        # 4) Verify initial instance counts pre-filter
        num_expected = 121
        spawner_success = self.wait_for_condition(
            lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected), 5.0)
        self.test_success = self.test_success and spawner_success
        """

        # 5) Create a child entity of the spawner entity with a Constant Gradient component and pin to spawner
        components_to_add = ["Constant Gradient"]
        gradient_entity = hydra.Entity("Gradient Entity")
        gradient_entity.create_entity(spawner_center_point, components_to_add, parent_id=spawner_entity.id)

        # Pin the Constant Gradient to the X axis of the spawner's Position Modifier component
        spawner_entity.get_set_test(3, 'Configuration|Position X|Gradient|Gradient Entity Id', gradient_entity.id)

        # 6) Set the Position Modifier offset to 5 on the x-axis
        spawner_entity.get_set_test(3, position_modifier_paths[0], 5)
        spawner_entity.get_set_test(3, position_modifier_paths[1], 5)

        # 7) Validate instance count at the top of the sphere mesh and inside the sphere mesh while Auto Snap to Surface
        # is enabled
        top_point = math.Vector3(512.0, 512.0, 37.0)
        inside_point = math.Vector3(512.0, 512.0, 33.0)
        radius = 0.5
        num_expected = 1
        self.log(f"Checking for instances in a {radius * 2}m area at {top_point.ToString()}")
        top_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(top_point, radius, num_expected),
                                              5.0)
        self.test_success = top_success and self.test_success
        num_expected = 0
        self.log(f"Checking for instances in a {radius * 2}m area at {inside_point.ToString()}")
        inside_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(inside_point, radius,
                                                                                        num_expected), 5.0)
        self.test_success = inside_success and self.test_success

        # 8) Toggle off Auto Snap to Surface. Instances should now plant inside the sphere and no longer on top
        spawner_entity.get_set_test(3, "Configuration|Auto Snap To Surface", False)
        num_expected = 0
        self.log(f"Checking for instances in a {radius * 2}m area at {top_point.ToString()}")
        top_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(top_point, radius, num_expected),
                                              5.0)
        self.test_success = top_success and self.test_success
        num_expected = 1
        self.log(f"Checking for instances in a {radius * 2}m area at {inside_point.ToString()}")
        inside_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(inside_point, radius,
                                                                                        num_expected), 5.0)
        self.test_success = inside_success and self.test_success


test = TestPositionModifierAutoSnapToSurface()
test.run()
