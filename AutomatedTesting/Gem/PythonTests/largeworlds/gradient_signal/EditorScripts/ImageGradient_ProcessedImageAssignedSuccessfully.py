"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.asset as asset
import azlmbr.bus as bus
import azlmbr.entity as EntityId
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper


class TestImageGradient(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="ImageGradient_ProcessedImageAssignedSucessfully",
                                  args=["level"])

    def run_test(self):
        """
        Summary:
        Level created with Entity having Image Gradient and Gradient Transform Modifier components.
        Save any new image to your workspace with the suffix "_gsi" and assign as image asset.
        
        Expected Behavior:
        Image can be assigned as the Image Asset for the Image as Gradient component.

        Test Steps:
        1) Create level
        2) Create an entity with Image Gradient and Gradient Transform Modifier components.
        3) Assign the newly processed gradient image as Image asset.

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        # 1) Create level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # 2) Create an entity with Image Gradient and Gradient Transform Modifier components
        components_to_add = ["Image Gradient", "Gradient Transform Modifier", "Box Shape"]
        entity_position = math.Vector3(512.0, 512.0, 32.0)
        new_entity_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
        )
        if new_entity_id.IsValid():
            print("Image Gradient Entity created")
        image_gradient_entity = hydra.Entity("Image Gradient Entity", new_entity_id)
        image_gradient_entity.components = []
        for component in components_to_add:
            image_gradient_entity.add_component(component)

        # 3) Assign the processed gradient signal image as the Image Gradient's image asset and verify success

        # First, check for the base image in the workspace
        base_image = "image_grad_test_gsi.png"
        base_image_path = os.path.join("AutomatedTesting", "Assets", "ImageGradients", base_image)
        if os.path.isfile(base_image_path):
            print(f"{base_image} was found in the workspace")

        # Next, assign the processed image to the Image Gradient's Image Asset property
        processed_image_path = os.path.join("Assets", "ImageGradients", "image_grad_test_gsi.gradimage")
        asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", processed_image_path, math.Uuid(),
                                                False)
        hydra.get_set_test(image_gradient_entity, 0, "Configuration|Image Asset", asset_id)

        # Finally, verify if the gradient image is assigned as the Image Asset
        success = hydra.get_component_property_value(image_gradient_entity.components[0], "Configuration|Image Asset") == asset_id
        self.test_success = self.test_success and success


test = TestImageGradient()
test.run()
