"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import os
import sys

import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, "AutomatedTesting", "Gem", "PythonTests"))
import automatedtesting_shared.hydra_editor_utils as hydra
from automatedtesting_shared.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestLayerSpawnerFilterStageToggle(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="LayerSpawner_FilterStageToggle", args=["level"])

    def run_test(self):
        """
        Summary:
        C4765973 Filter Stage toggle affects final vegetation position.

        Expected Result:
        Vegetation instances plant differently depending on the Filter Stage setting.

        :return: None
        """

        PREPROCESS_INSTANCE_COUNT = 16
        POSTPROCESS_INSTANCE_COUNT = 19

        # Create empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # Create a vegetation area with all needed components
        position = math.Vector3(512.0, 512.0, 32.0)
        asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
        vegetation_entity = dynveg.create_vegetation_area("vegetation", position, 16.0, 16.0, 10.0, asset_path)
        vegetation_entity.add_component("Vegetation Slope Filter")
        vegetation_entity.add_component("Vegetation Position Modifier")

        # Create a child entity under vegetation area
        child_entity = hydra.Entity("child_entity")
        components_to_add = ["Random Noise Gradient", "Gradient Transform Modifier", "Box Shape"]
        child_entity.create_entity(position, components_to_add, vegetation_entity.id)

        # Set the Gradient Id in X and Y direction
        vegetation_entity.get_set_test(4, "Configuration|Position X|Gradient|Gradient Entity Id", child_entity.id)
        vegetation_entity.get_set_test(4, "Configuration|Position Y|Gradient|Gradient Entity Id", child_entity.id)

        # Set the min and max values for Slope Filter
        vegetation_entity.get_set_test(3, "Configuration|Slope Min", 25)
        vegetation_entity.get_set_test(3, "Configuration|Slope Max", 35)

        # Add entity with Mesh to replicate creation of hills
        dynveg.create_mesh_surface_entity_with_slopes("hill", position, 5.0, 5.0, 5.0)

        # Set the filter stage to preprocess and postprocess respectively and verify instance count
        vegetation_entity.get_set_test(0, "Configuration|Filter Stage", 1)
        self.wait_for_condition(lambda: dynveg.validate_instance_count(position, 16.0, PREPROCESS_INSTANCE_COUNT), 3.0)
        result = dynveg.validate_instance_count(position, 16.0, PREPROCESS_INSTANCE_COUNT)
        self.log(f"Preprocess filter stage vegetation instance count is as expected: {result}")
        vegetation_entity.get_set_test(0, "Configuration|Filter Stage", 2)
        self.wait_for_condition(lambda: dynveg.validate_instance_count(position, 16.0, POSTPROCESS_INSTANCE_COUNT), 3.0)
        result = dynveg.validate_instance_count(position, 16.0, POSTPROCESS_INSTANCE_COUNT)
        self.log(f"Postprocess filter vegetation instance stage count is as expected: {result}")


test = TestLayerSpawnerFilterStageToggle()
test.run()
