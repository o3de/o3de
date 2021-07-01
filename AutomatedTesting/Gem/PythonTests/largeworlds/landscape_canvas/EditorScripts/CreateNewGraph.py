"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.editor.graph as graph
import azlmbr.legacy.general as general
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper

editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID
new_root_entity_id = None


class TestCreateNewGraph(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="CreateNewGraph", args=["level"])

    def on_entity_created(self, parameters):
        global new_root_entity_id
        new_root_entity_id = parameters[0]

        print("New root entity created")

    def run_test(self):
        """
        Summary:
        This test verifies that new graphs can be created in Landscape Canvas.

        Expected Behavior:
        New graphs can be created, and proper entity is created to hold graph data with a Landscape Canvas component.

        Test Steps:
         1) Create a new level
         2) Open Landscape Canvas and create a new graph
         3) Ensures the root entity created contains a Landscape Canvas component

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=128,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=128,
            use_terrain=False,
        )
        # Open Landscape Canvas tool and verify
        general.open_pane("Landscape Canvas")
        self.test_success = self.test_success and general.is_pane_visible("Landscape Canvas")
        if general.is_pane_visible("Landscape Canvas"):
            self.log("Landscape Canvas pane is open")

        # Listen for entity creation notifications so we can check if the entity created
        # with the new graph has our Landscape Canvas component automatically added
        handler = editor.EditorEntityContextNotificationBusHandler()
        handler.connect()
        handler.add_callback("OnEditorEntityCreated", self.on_entity_created)

        # Create a new graph in Landscape Canvas
        newGraphId = graph.AssetEditorRequestBus(bus.Event, "CreateNewGraph", editorId)
        self.test_success = self.test_success and newGraphId
        if newGraphId:
            self.log("New graph created")

        # Make sure the graph we created is in Landscape Canvas
        success = graph.AssetEditorRequestBus(bus.Event, "ContainsGraph", editorId, newGraphId)
        self.test_success = self.test_success and success
        if success:
            self.log("Graph registered with Landscape Canvas")

        # Check if the entity created when we create a new graph has the
        # Landscape Canvas component already added to it
        landscape_canvas_type_id = hydra.get_component_type_id("Landscape Canvas")
        success = editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", new_root_entity_id,
                                               landscape_canvas_type_id)
        self.test_success = self.test_success and success
        if success:
            self.log("Root entity has Landscape Canvas component")

        # Close Landscape Canvas tool and verify
        general.close_pane("Landscape Canvas")
        self.test_success = self.test_success and not general.is_pane_visible("Landscape Canvas")
        if not general.is_pane_visible("Landscape Canvas"):
            self.log("Landscape Canvas pane is closed")

        # Stop listening for entity creation notifications
        handler.disconnect()


test = TestCreateNewGraph()
test.run()
