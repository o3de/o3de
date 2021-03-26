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

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import automatedtesting_shared.hydra_editor_utils as hydra
from automatedtesting_shared.editor_test_helper import EditorTestHelper

editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID


class TestLandscapeCanvasComponentAddedRemoved(EditorTestHelper):

    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="LandscapeCanvasComponentAddedRemoved", args=["level"])

    def run_test(self):

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
