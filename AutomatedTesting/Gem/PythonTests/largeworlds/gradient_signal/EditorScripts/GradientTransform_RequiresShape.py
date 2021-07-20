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


class TestGradientTransformRequiresShape(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GradientTransformRequiresShape", args=["level"])

    def run_test(self):
        """
        Summary:
        This test verifies that the Gradient Transform Modifier component is dependent on a shape component.

        Expected Result:
        Gradient Transform Modifier component is disabled until a shape component is added to the entity.

        Test Steps:
         1) Open level
         2) Create a new entity with a Gradient Transform Modifier component
         3) Verify the component is disabled until a shape component is also added to the entity

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        # Create a new empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # Create a new Entity in the level
        entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())

        # Add a Gradient Transform Component (that will be disabled since there is no shape on the Entity)
        gradient_transform_component = hydra.add_component('Gradient Transform Modifier', entity_id)

        # Verify the Gradient Transform Component is not active before adding the Shape
        active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', gradient_transform_component)
        self.test_success = self.test_success and not active
        if not active:
            self.log("Gradient Transform component is not active without a Shape component on the Entity")

        # Add a Shape component to the same Entity
        hydra.add_component('Box Shape', entity_id)

        # Check if the Gradient Transform Component is active now after adding the Shape
        active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', gradient_transform_component)
        self.test_success = self.test_success and active
        if active:
            self.log("Gradient Transform Modifier component is active now that the Entity has a Shape")


test = TestGradientTransformRequiresShape()
test.run()
