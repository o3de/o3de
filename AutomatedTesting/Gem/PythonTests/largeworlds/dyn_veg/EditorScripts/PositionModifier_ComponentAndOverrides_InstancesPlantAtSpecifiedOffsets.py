"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import random
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.editor as editor
import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestPositionModifierComponentAndOverrides(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="PositionModifierComponentAndOverrides_InstanceOffset", args=["level"])

    def run_test(self):
        """
        Summary: Range Min/Max in the Vegetation Position Modifier component and component overrides can be set for all
        axes, and functions as expected when fed a gradient signal.

        Expected Behavior: Instances are offset by the specified amount.

        Test Steps:
        1) New test level is created
        2) Spawner area is setup with all necessary components
        3) Surface for planting is created
        4) Initial instance count validation pre-filter is performed
        5) An entity with a Constant Gradient of 1 is added as a child to the spawner entity, and pinned to the Position
        Modifier Gradient Entity Id fields
        6) Sector size is adjusted on a Vegetation System Settings component to allow for offset instances to not fall
        outside of the queried sector
        7) Random offsets are set for each axis of the Position Modifier component, and instance counts are validated
        8) Overrides are enabled on the Position Modifier component
        9) Random offsets are set for each axis of the descriptor's Position Modifier overrides, and instance counts
        are validated

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        position_modifier_paths = ['Configuration|Position X|Range Min', 'Configuration|Position X|Range Max',
                                   'Configuration|Position Y|Range Min', 'Configuration|Position Y|Range Max',
                                   'Configuration|Position Z|Range Min', 'Configuration|Position Z|Range Max']

        override_position_modifier_paths = ['Configuration|Embedded Assets|[0]|Position Modifier|Min X',
                                            'Configuration|Embedded Assets|[0]|Position Modifier|Max X',
                                            'Configuration|Embedded Assets|[0]|Position Modifier|Min Y',
                                            'Configuration|Embedded Assets|[0]|Position Modifier|Max Y',
                                            'Configuration|Embedded Assets|[0]|Position Modifier|Min Z',
                                            'Configuration|Embedded Assets|[0]|Position Modifier|Max Z']

        def generate_random_offset_list():
            offset_list = []
            while len(offset_list) < 10:
                offset = round(random.uniform(-8.0, 8.0), 2)
                if not -1.0 <= offset <= 1.0:
                    offset_list.append(offset)
            print("List of values to test against = " + str(offset_list))
            return offset_list

        def set_offset_and_verify_instance_counts(offset_to_test, center, is_override=False):
            print(f"Starting test with an offset of {offset_to_test}")
            # Set min/max values to the offset value
            if not is_override:
                for path in position_modifier_paths:
                    spawner_entity.get_set_test(3, path, offset_to_test)
            else:
                spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Position Modifier|Override Enabled",
                                            True)
                for path in override_position_modifier_paths:
                    spawner_entity.get_set_test(2, path, offset_to_test)
            center_point = math.Vector3(center.x + offset_to_test, center.y + offset_to_test, center.z + offset_to_test)
            radius = 0.5
            print(f"Querying for instances in a {radius * 2}m area around {center_point.ToString()}")
            offset_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(center_point, radius, 1), 5.0)
            offset_success2 = self.wait_for_condition(lambda: dynveg.validate_instance_count(center, radius, 0), 5.0)
            self.test_success = offset_success and offset_success2 and self.test_success

        # 1) Create a new, temporary level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # Set view of planting area for visual debugging
        general.set_current_view_position(16.0, -5.0, 32.0)

        # 2) Create a new entity with required vegetation area components
        spawner_center_point = math.Vector3(16.0, 16.0, 32.0)
        asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
        spawner_entity = dynveg.create_vegetation_area("Instance Spawner", spawner_center_point, 1.0, 1.0, 1.0, asset_path)

        # Add a Vegetation Position Modifier and set offset values to 0
        spawner_entity.add_component("Vegetation Position Modifier")
        for path in position_modifier_paths:
            spawner_entity.get_set_test(3, path, 0)

        # 3) Add flat surface to plant on
        dynveg.create_surface_entity("Planting Surface", spawner_center_point, 32.0, 32.0, 0.0)

        # 4) Verify initial instance counts pre-filter
        num_expected = 1  # Single instance planted
        spawner_success = self.wait_for_condition(
            lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected), 5.0)
        self.test_success = self.test_success and spawner_success

        # 5) Create a child entity of the spawner entity with a Constant Gradient component
        components_to_add = ["Constant Gradient"]
        gradient_entity = hydra.Entity("Gradient Entity")
        gradient_entity.create_entity(spawner_center_point, components_to_add, parent_id=spawner_entity.id)

        # Pin the Constant Gradient to each axis of the Position Modifier
        position_modifier_gradient_paths = ['Configuration|Position X|Gradient|Gradient Entity Id',
                                            'Configuration|Position Y|Gradient|Gradient Entity Id',
                                            'Configuration|Position Z|Gradient|Gradient Entity Id']
        for path in position_modifier_gradient_paths:
            spawner_entity.get_set_test(3, path, gradient_entity.id)

        # 6) Add a Vegetation System Settings Level component and change sector size to 32 sq meters so instances can
        # offset to a greater range and still be validated
        veg_system_settings_component = hydra.add_level_component("Vegetation System Settings")
        editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", veg_system_settings_component,
                                     "Configuration|Area System Settings|Sector Size In Meters", 32)
        sector_size = hydra.get_component_property_value(veg_system_settings_component,
                                                         "Configuration|Area System Settings|Sector Size In Meters")
        self.test_success = (sector_size == 32) and self.test_success

        # 7) Set offsets on all axes and verify instance counts
        offsets_to_test = generate_random_offset_list()
        for offset in offsets_to_test:
            if self.test_success:
                set_offset_and_verify_instance_counts(offset, spawner_center_point)

        # 8) Toggle on allow overrides on the Position Modifier Component
        spawner_entity.get_set_test(3, "Configuration|Allow Per-Item Overrides", True)

        # 9) Set offsets on all axes on descriptor overrides and verify instance counts
        for offset in offsets_to_test:
            if self.test_success:
                set_offset_and_verify_instance_counts(offset, spawner_center_point, is_override=True)


test = TestPositionModifierComponentAndOverrides()
test.run()
