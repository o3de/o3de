"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.areasystem as areasystem
import azlmbr.bus as bus
import azlmbr.legacy.general as general
import azlmbr.math as math

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestScaleModifier_InstancesProperlyScale(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="ScaleModifier_InstancesProperlyScale", args=["level"])

    def run_test(self):
        """
        Summary:
        A New level is created. A New entity is created with components Vegetation Layer Spawner, Vegetation Asset List,
        Box Shape and Vegetation Scale Modifier. A New child entity is created with components Random Noise Gradient,
        Gradient Transform Modifier, and Box Shape. Pin the Random Noise entity to the Gradient Entity Id field for
        the Gradient group. Range Min and Range Max are set to few values and values are validated. Range Min and Range
        Max are set to few other values and values are validated.

        Expected Behavior:
        Vegetation instances are scaled within Range Min/Range Max.

        Test Steps:
         1) Create level
         2) Create a new entity with components Vegetation Layer Spawner, Vegetation Asset List, Box Shape and
            Vegetation Scale Modifier
         3) Create child entity with components Random Noise Gradient, Gradient Transform Modifier and Box Shape
         4) Pin the Random Noise entity to the Gradient Entity Id field for the Gradient group.
         5) Range Min/Max is set to few different values on the Vegetation Scale Modifier component and
            scale of instances is validated

        Note:
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def set_and_validate_scale(entity, min_scale, max_scale):
            # Set Range Min/Max
            entity.get_set_test(3, "Configuration|Range Min", min_scale)
            entity.get_set_test(3, "Configuration|Range Max", max_scale)

            # Clear all areas to force a refresh
            general.run_console('veg_debugClearAllAreas')

            # Wait for instances to spawn
            num_expected = 20 * 20
            self.test_success = self.test_success and self.wait_for_condition(
                lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected), 5.0)

            # Validate scale values of instances
            box = azlmbr.shape.ShapeComponentRequestsBus(bus.Event, 'GetEncompassingAabb', entity.id)
            instances = areasystem.AreaSystemRequestBus(bus.Broadcast, 'GetInstancesInAabb', box)
            if len(instances) == num_expected:
                for instance in instances:
                    if min_scale <= instance.scale <= max_scale:
                        self.log("All instances scaled within appropriate range")
                        return True
                    self.log(f"Instance at {instance.position} scale is {instance.scale}. Expected between "
                             f"{min_scale}/{max_scale}")
                    return False
            self.log(f"Failed to find all instances! Found {len(instances)}, expected {num_expected}.")
            return False

        # 1) Create level and set an appropriate view of spawner area
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        general.set_current_view_position(500.49, 498.69, 46.66)
        general.set_current_view_rotation(-42.05, 0.00, -36.33)

        # 2) Create a new entity with components Vegetation Layer Spawner, Vegetation Asset List, Box Shape and
        #    Vegetation Scale Modifier
        entity_position = math.Vector3(512.0, 512.0, 32.0)
        asset_path = os.path.join("Slices", "PurpleFlower.dynamicslice")
        spawner_entity = dynveg.create_vegetation_area("Spawner Entity", entity_position, 16.0, 16.0, 16.0,
                                                       asset_path)
        spawner_entity.add_component("Vegetation Scale Modifier")

        # Create a surface to plant on and add a Vegetation Debugger Level component to allow refreshes
        dynveg.create_surface_entity("Surface Entity", entity_position, 20.0, 20.0, 1.0)
        hydra.add_level_component("Vegetation Debugger")

        # 3) Create child entity with components Random Noise Gradient, Gradient Transform Modifier and Box Shape
        gradient_entity = hydra.Entity("Gradient Entity")
        gradient_entity.create_entity(
            entity_position,
            ["Random Noise Gradient", "Gradient Transform Modifier", "Box Shape"],
            parent_id=spawner_entity.id
        )
        if gradient_entity.id.IsValid():
            self.log(f"'{gradient_entity.name}' created")

        # 4) Pin the Random Noise entity to the Gradient Entity Id field for the Gradient group.
        spawner_entity.get_set_test(3, "Configuration|Gradient|Gradient Entity Id", gradient_entity.id)

        # 5) Set Range Min/Max on the Vegetation Scale Modifier component to diff values, and verify instance scale is
        # within bounds
        self.test_success = set_and_validate_scale(spawner_entity, 2.0, 4.0) and self.test_success
        self.test_success = set_and_validate_scale(spawner_entity, 12.0, 40.0) and self.test_success
        self.test_success = set_and_validate_scale(spawner_entity, 0.5, 2.5) and self.test_success


test = TestScaleModifier_InstancesProperlyScale()
test.run()
