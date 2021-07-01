"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.editor.graph as graph
import azlmbr.landscapecanvas as landscapecanvas
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
from editor_python_test_tools.editor_test_helper import EditorTestHelper

editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID
createdEntityId = None
deletedEntityId = None


class TestGradientModifierNodeEntityDelete(EditorTestHelper):

    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GradientModifierNodeEntityDelete", args=["level"])

    def run_test(self):
        """
        Summary:
        This test verifies that the Landscape Canvas node deletion properly cleans up entities in the Editor.

        Expected Behavior:
        Entities are removed when Gradient Modifier nodes are deleted from a graph.

        Test Steps:
         1) Create a new level
         2) Open Landscape Canvas and create a new graph
         3) Drag each of the Gradient Modifier nodes to the graph area, and ensure a new entity is created
         4) Delete the nodes, and ensure the newly created entities are removed

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        def onEntityCreated(parameters):
            global createdEntityId
            createdEntityId = parameters[0]

        def onEntityDeleted(parameters):
            global deletedEntityId
            deletedEntityId = parameters[0]

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
        success = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId)
        self.test_success = self.test_success and success
        if success:
            self.log("Graph registered with Landscape Canvas")

        # Listen for entity creation notifications so we can check if the entity created
        # from adding gradient nodes has the appropriate Gradient Component
        handler = editor.EditorEntityContextNotificationBusHandler()
        handler.connect()
        handler.add_callback('OnEditorEntityCreated', onEntityCreated)
        handler.add_callback('OnEditorEntityDeleted', onEntityDeleted)

        # Vegetation Area mapping with the key being the node name and the value is the
        # expected Component that should be added to the Entity created for the node
        gradientModifiers = [
            'DitherGradientModifierNode',
            'GradientMixerNode',
            'InvertGradientModifierNode',
            'LevelsGradientModifierNode',
            'PosterizeGradientModifierNode',
            'SmoothStepGradientModifierNode',
            'ThresholdGradientModifierNode'
        ]

        # Create nodes for all the gradients we support and check if the Entity created by
        # adding the node has the appropriate Component added automatically to it
        newGraph = graph.GraphManagerRequestBus(bus.Broadcast, 'GetGraph', newGraphId)
        x = 10.0
        y = 10.0
        for nodeName in gradientModifiers:
            nodePosition = math.Vector2(x, y)
            node = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName', newGraph, nodeName)
            graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, node, nodePosition)

            removed = graph.GraphControllerRequestBus(bus.Event, 'RemoveNode', newGraphId, node)

            # Verify that the created Entity for this node matches the Entity that gets
            # deleted when the node is removed
            self.test_success = self.test_success and removed and createdEntityId.invoke("Equal", deletedEntityId)
            if removed and createdEntityId.invoke("Equal", deletedEntityId):
                self.log("{node} corresponding Entity was deleted when node is removed".format(node=nodeName))

        # Stop listening for entity creation notifications
        handler.disconnect()


test = TestGradientModifierNodeEntityDelete()
test.run()
