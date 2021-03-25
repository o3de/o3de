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
import azlmbr.editor.graph as graph
import azlmbr.landscapecanvas as landscapecanvas
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
from automatedtesting_shared.editor_test_helper import EditorTestHelper

editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID
createdEntityId = None
deletedEntityId = None


class TestGradientNodeEntityDelete(EditorTestHelper):

    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GradientNodeEntityDelete", args=["level"])

    def run_test(self):

        def onEntityCreated(parameters):
            global createdEntityId
            createdEntityId = parameters[0]

        def onEntityDeleted(parameters):
            global deletedEntityId
            deletedEntityId = parameters[0]

        # Create a new empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
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

        # Gradient mapping with the key being the node name and the value is the
        # expected Component that should be added to the Entity created for the node
        gradients = [
            'AltitudeGradientNode',
            'ConstantGradientNode',
            'FastNoiseGradientNode',
            'ImageGradientNode',
            'PerlinNoiseGradientNode',
            'RandomNoiseGradientNode',
            'ShapeAreaFalloffGradientNode',
            'SlopeGradientNode',
            'SurfaceMaskGradientNode',
        ]

        # Create nodes for all the gradients we support and check if the Entity created by
        # adding the node has the appropriate Component added automatically to it
        newGraph = graph.GraphManagerRequestBus(bus.Broadcast, 'GetGraph', newGraphId)
        x = 10.0
        y = 10.0
        for nodeName in gradients:
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


test = TestGradientNodeEntityDelete()
test.run()
