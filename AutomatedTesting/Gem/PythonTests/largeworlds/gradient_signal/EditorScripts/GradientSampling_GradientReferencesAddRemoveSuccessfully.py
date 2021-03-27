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

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.math as math
import azlmbr.paths
import azlmbr.entity as EntityId

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import automatedtesting_shared.hydra_editor_utils as hydra
from automatedtesting_shared.editor_test_helper import EditorTestHelper


class TestGradientSampling(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GradientSampling_GradientReferences", args=["level"])

    def run_test(self):
        """
        Summary:
        An existing gradient generator can be pinned and cleared to/from the Gradient Entity Id field

        Expected Behavior:
        Gradient generator is assigned to the Gradient Entity Id field.
        Gradient generator is removed from the field.

        Test Steps:
         1) Open level
         2) Create a new entity with components "Random Noise Gradient", "Gradient Transform Modifier" and "Box Shape"
         3) Create a new entity with Gradient Modifier's, pin and clear the random noise entity id to the Gradient Id
            field in Gradient Modifier

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def modifier_pin_clear_to_gradiententityid(modifier):
            entity_position = math.Vector3(125.0, 136.0, 32.0)
            component_to_add = [modifier]
            gradient_modifier = hydra.Entity(modifier)
            gradient_modifier.create_entity(entity_position, component_to_add)
            gradient_modifier.get_set_test(0, "Configuration|Gradient|Gradient Entity Id", random_noise.id)
            entity = hydra.get_component_property_value(
                gradient_modifier.components[0], "Configuration|Gradient|Gradient Entity Id"
            )
            if entity.Equal(random_noise.id):
                print(f"Gradient Generator is pinned to the {modifier} successfully")
            else:
                print(f"Failed to pin Gradient Generator to the {modifier}")
            hydra.get_set_test(gradient_modifier, 0, "Configuration|Gradient|Gradient Entity Id", EntityId.EntityId())
            entity = hydra.get_component_property_value(
                gradient_modifier.components[0], "Configuration|Gradient|Gradient Entity Id"
            )
            if entity.Equal(EntityId.EntityId()):
                print(f"Gradient Generator is cleared from the {modifier} successfully")
            else:
                print(f"Failed to clear Gradient Generator from the {modifier}")

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # 2) Create a new entity with components "Random Noise Gradient", "Gradient Transform Modifier" and "Box Shape"
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        components_to_add = ["Random Noise Gradient", "Gradient Transform Modifier", "Box Shape"]
        random_noise = hydra.Entity("Random_Noise")
        random_noise.create_entity(entity_position, components_to_add)

        # 3) Create a new entity with Gradient Modifier's, pin and clear the random noise entity id to the Gradient Id
        #    field in Gradient Modifier
        modifier_pin_clear_to_gradiententityid("Dither Gradient Modifier")
        modifier_pin_clear_to_gradiententityid("Invert Gradient Modifier")
        modifier_pin_clear_to_gradiententityid("Levels Gradient Modifier")
        modifier_pin_clear_to_gradiententityid("Posterize Gradient Modifier")
        modifier_pin_clear_to_gradiententityid("Smooth-Step Gradient Modifier")
        modifier_pin_clear_to_gradiententityid("Threshold Gradient Modifier")


test = TestGradientSampling()
test.run()
