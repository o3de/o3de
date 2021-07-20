"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os
import sys

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper


class TestGradientGeneratorIncompatibilities(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GradientGeneratorIncompatibilities", args=["level"])

    def run_test(self):
        """
        Summary:
        This test verifies that components are disabled when conflicting components are present on the same entity.

        Expected Behavior:
        Gradient Generator components are incompatible with Vegetation area components.

        Test Steps:
         1) Create a new level
         2) Create a new entity in the level
         3) Add each Gradient Generator component to an entity, and add a Vegetation Area component to the same entity
         4) Verify that components are only enabled when entity is free of a conflicting component

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        gradient_generators = [
            'Altitude Gradient',
            'Constant Gradient',
            'FastNoise Gradient',
            'Image Gradient',
            'Perlin Noise Gradient',
            'Random Noise Gradient',
            'Shape Falloff Gradient',
            'Slope Gradient',
            'Surface Mask Gradient'
        ]
        require_transform_modifiers = [
            'FastNoise Gradient',
            'Image Gradient',
            'Perlin Noise Gradient',
            'Random Noise Gradient'
        ]
        vegetation_areas = [
            'Vegetation Layer Spawner',
            'Vegetation Layer Blender',
            'Vegetation Layer Blocker',
            'Vegetation Layer Blocker (Mesh)'
        ]
        area_dependencies = {
            'Vegetation Layer Spawner': 'Vegetation Asset List',
            'Vegetation Layer Blocker (Mesh)': 'Mesh'
        }

        # Create a new empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # For every gradient generator component, verify that they are incompatible
        # which each vegetation area component
        for component_name in gradient_generators:
            for vegetation_area_name in vegetation_areas:
                # Create a new Entity in the level
                entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())

                # Most of these need a shape, so use a Box Shape
                hydra.add_component('Box Shape', entity_id)

                # Add the specific vegetation area dependencies (if necessary)
                if vegetation_area_name in area_dependencies:
                    hydra.add_component(area_dependencies[vegetation_area_name], entity_id)

                # Add the vegetation area component we are validating against, then add the
                # gradient generator afterwards, so that the gradient generator will actually
                # be disabled (if it was present before, it would only get deactivated instead of disabled
                # by the vegetation area)
                area_component = hydra.add_component(vegetation_area_name, entity_id)
                gradient_component = hydra.add_component(component_name, entity_id)

                # Verify the gradient generator component is disabled since the vegetation area is incompatible
                active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', gradient_component)
                self.test_success = self.test_success and not active
                if not active:
                    self.log(f"{component_name} is disabled before removing {vegetation_area_name} component")

                # Remove the vegetation area component
                editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [area_component])

                # Add required dependencies for our gradient generators after the vegetation
                # area has been removed, because the transform modifier is also incompatible
                # with the vegetation areas
                if component_name in require_transform_modifiers:
                    hydra.add_component('Gradient Transform Modifier', entity_id)

                # Verify the gradient generator component is enabled now that the vegetation area is gone
                active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', gradient_component)
                self.test_success = self.test_success and active
                if active:
                    self.log(f"{component_name} is enabled after removing {vegetation_area_name} component")


test = TestGradientGeneratorIncompatibilities()
test.run()
