"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

import azlmbr.bus as bus
import azlmbr.editor.graph as graph
import azlmbr.legacy.general as general
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
from editor_python_test_tools.editor_test_helper import EditorTestHelper

editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID


class TestGraphClosedOnLevelChange(EditorTestHelper):

    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GraphClosedOnLevelChange", args=["level"])

    def run_test(self):
        """
        Summary:
        This test verifies that Landscape Canvas graphs are auto-closed when the currently open level changes.

        Expected Behavior:
        When a new level is loaded in the Editor, open Landscape Canvas graphs are automatically closed.

        Test Steps:
         1) Create a new level
         2) Open Landscape Canvas and create a new graph
         3) Open a different level
         4) Verify the open graph is closed

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

        # Open Landscape Canvas tool and verify
        general.open_pane('Landscape Canvas')
        self.test_success = self.test_success and general.is_pane_visible('Landscape Canvas')
        if general.is_pane_visible('Landscape Canvas'):
            self.log('Landscape Canvas pane is open')

        # Create a new graph in Landscape Canvas
        newGraphId = graph.AssetEditorRequestBus(bus.Event, 'CreateNewGraph', editorId)
        self.test_success = self.test_success and newGraphId
        if newGraphId:
            self.log("New graph created")

        # Make sure the graph we created is in Landscape Canvas
        graphIsOpen = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId)
        self.test_success = self.test_success and graphIsOpen
        if graphIsOpen:
            self.log("Graph registered with Landscape Canvas")

        # Open a different level, which should close any open Landscape Canvas graphs
        general.open_level_no_prompt('WhiteBox/EmptyLevel')

        # Make sure the graph we created is now closed
        graphIsOpen = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId)
        self.test_success = self.test_success and not graphIsOpen
        if not graphIsOpen:
            self.log("Graph is no longer open in Landscape Canvas")


test = TestGraphClosedOnLevelChange()
test.run()
