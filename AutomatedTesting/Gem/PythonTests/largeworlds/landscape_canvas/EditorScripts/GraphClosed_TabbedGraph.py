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
import azlmbr.editor.graph as graph
import azlmbr.legacy.general as general
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
from automatedtesting_shared.editor_test_helper import EditorTestHelper

editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID


class TestGraphClosedTabbedGraph(EditorTestHelper):

    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GraphClosedTabbedGraph", args=["level"])

    def run_test(self):

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

        # Create 3 new graphs in Landscape Canvas
        newGraphId = graph.AssetEditorRequestBus(bus.Event, 'CreateNewGraph', editorId)
        self.test_success = self.test_success and newGraphId
        if newGraphId:
            self.log("New graph created")

        newGraphId2 = graph.AssetEditorRequestBus(bus.Event, 'CreateNewGraph', editorId)
        self.test_success = self.test_success and newGraphId2
        if newGraphId2:
            self.log("2nd new graph created")

        newGraphId3 = graph.AssetEditorRequestBus(bus.Event, 'CreateNewGraph', editorId)
        self.test_success = self.test_success and newGraphId3
        if newGraphId3:
            self.log("3rd new graph created")

        # Make sure the graphs we created are open in Landscape Canvas
        success = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId)
        success2 = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId2)
        success3 = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId3)
        self.test_success = self.test_success and success and success2 and success3
        if success and success2 and success3:
            self.log("Graphs registered with Landscape Canvas")

        # Close a single graph and verify it was properly closed and other graphs remain open
        success4 = graph.AssetEditorRequestBus(bus.Event, 'CloseGraph', editorId, newGraphId2)
        success = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId)
        success2 = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId2)
        success3 = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId3)
        self.test_success = self.test_success and success and success3 and success4 and not success2
        if success and success3 and success4 and not success2:
            self.log("Graph 2 was successfully closed")


test = TestGraphClosedTabbedGraph()
test.run()
