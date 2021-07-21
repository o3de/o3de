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

editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID


class TestLandscapeCanvasComponentAddedRemoved(EditorTestHelper):

    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="LandscapeCanvasComponentAddedRemoved", args=["level"])

    def run_test(self):
        """
        Summary:
        This test verifies that the Landscape Canvas component can be added to/removed from an entity.

        Expected Behavior:
        Closing a tabbed graph only closes the appropriate graph.

        Test Steps:
         1) Create a new level
         2) Create a new entity
         3) Add a Landscape Canvas component to the entity
         4) Remove the Landscape Canvas component from the entity

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        # Create a new empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=128,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=128,
            use_terrain=False,
        )

        # Create an Entity at the root of the level
        newEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())

        # Find the component TypeId for our Landscape Canvas component
        landscape_canvas_type_id = hydra.get_component_type_id("Landscape Canvas")

        # Add the Landscape Canvas Component to our Entity
        componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', newEntityId, [landscape_canvas_type_id])
        components = componentOutcome.GetValue()
        landscapeCanvasComponent = components[0]

        # Validate the Landscape Canvas Component exists
        hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, landscape_canvas_type_id)
        self.test_success = self.test_success and hasComponent
        if hasComponent:
            self.log("Landscape Canvas Component added to Entity")

        # Remove the Landscape Canvas Component
        editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [landscapeCanvasComponent])

        # Validate the Landscape Canvas Component is no longer on our Entity
        hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, landscape_canvas_type_id)
        self.test_success = self.test_success and not hasComponent
        if not hasComponent:
            self.log("Landscape Canvas Component removed from Entity")


test = TestLandscapeCanvasComponentAddedRemoved()
test.run()
