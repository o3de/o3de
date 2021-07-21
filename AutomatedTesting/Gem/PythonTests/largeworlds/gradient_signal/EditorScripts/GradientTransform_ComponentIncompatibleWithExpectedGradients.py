"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.entity as EntityId

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper


class TestGradientTransform_ComponentIncompatibleWithExpectedGradients(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GradientTransform_ComponentIncompatibleWithExpectedGradients",
                                  args=["level"])

    def run_test(self):
        """
        Summary:
        A New level is created. A New entity is created with components Gradient Transform Modifier and Box Shape.
        Adding components Constant Gradient, Altitude Gradient, Gradient Mixer, Reference Gradient, Shape
        Falloff Gradient, Slope Gradient and Surface Mask Gradient to the same entity.

        Expected Behavior:
        All added components are disabled and inform the user that they are incompatible with the Gradient Transform
        Modifier

        Test Steps:
         1) Create level
         2) Create a new entity with components Gradient Transform Modifier and Box Shape
         3) Make sure all components are enabled in Entity
         4) Add Constant Gradient, Altitude Gradient, Gradient Mixer, Reference Gradient, Shape
            Falloff Gradient, Slope Gradient and Surface Mask Gradient to the same entity
         5) Make sure all newly added components are disabled

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def is_enabled(EntityComponentIdPair):
            return editor.EditorComponentAPIBus(bus.Broadcast, "IsComponentEnabled", EntityComponentIdPair)

        # 1) Create level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # 2) Create a new entity with components Gradient Transform Modifier and Box Shape
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        components_to_add = ["Gradient Transform Modifier", "Box Shape"]
        gradient_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
        )
        gradient = hydra.Entity("gradient", gradient_id)

        gradient.components = []

        for component in components_to_add:
            gradient.components.append(hydra.add_component(component, gradient_id))
        if gradient_id.isValid():
            self.log("New Entity Created")

        # 3) Make sure all components are enabled in Entity
        index = 0
        for component in components_to_add:
            is_enable = is_enabled(gradient.components[index])
            if is_enable:
                self.log(f"{component} is Enabled")
                self.test_success = self.test_success and is_enable
            elif not is_enable:
                self.log(f"{component} is disabled, but it should be enabled")
                self.test_success = self.test_success and is_enable
                break
            index += 1

        # 4) Add Constant Gradient, Altitude Gradient, Gradient Mixer, Reference Gradient, Shape
        #    Falloff Gradient, Slope Gradient and Surface Mask Gradient to the same entity
        new_components_to_add = [
            "Constant Gradient",
            "Altitude Gradient",
            "Gradient Mixer",
            "Reference Gradient",
            "Shape Falloff Gradient",
            "Slope Gradient",
            "Surface Mask Gradient",
        ]
        index = 2
        new_components_enabled = False
        for component in new_components_to_add:
            gradient.components.append(hydra.add_component(component, gradient_id))
            new_components_enabled = is_enabled(gradient.components[index])
            if new_components_enabled:
                self.log(f"{component} is enabled, but should be disabled")
                break
            editor.EditorComponentAPIBus(bus.Broadcast, "RemoveComponents", component)

        # 5) Make sure all newly added components are disabled
        if not new_components_enabled:
            self.log("All newly added components are incompatible and disabled")
        self.test_success = self.test_success and not new_components_enabled


test = TestGradientTransform_ComponentIncompatibleWithExpectedGradients()
test.run()
