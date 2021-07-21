"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C3430292: Frequency Zoom can manually be set higher than 8.
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.paths
import azlmbr.entity as EntityId

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper


class TestGradientTransformFrequencyZoom(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GradientTransform_FrequencyZoomBeyondSliders", args=["level"])

    def run_test(self):
        """
        Summary:
        Frequency Zoom can manually be set higher than 8 in a random noise gradient

        Expected Behavior:
        The value properly changes, despite the value being outside of the slider limit

        Test Steps:
         1) Open level
         2) Create entity
         3) Add components to the entity
         4) Set the frequency value of the component
         5) Verify if the frequency value is set to higher value

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # 2) Create entity
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        entity_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
        )
        if entity_id.IsValid():
            print("Entity Created")

        # 3) Add components to the entity
        components_to_add = ["Random Noise Gradient", "Gradient Transform Modifier", "Box Shape"]
        entity = hydra.Entity("entity", entity_id)
        entity.components = []
        for component in components_to_add:
            entity.components.append(hydra.add_component(component, entity_id))
        print("Components added to the entity")

        # 4) Set the frequency value of the component
        hydra.get_set_test(entity, 1, "Configuration|Frequency Zoom", 10)

        # 5) Verify if the frequency value is set to higher value
        curr_value = hydra.get_component_property_value(entity.components[1], "Configuration|Frequency Zoom")
        if curr_value == 10.0:
            print("Frequency Zoom is equal to expected value")
        else:
            print("Frequency Zoom is not equal to expected value")


test = TestGradientTransformFrequencyZoom()
test.run()
