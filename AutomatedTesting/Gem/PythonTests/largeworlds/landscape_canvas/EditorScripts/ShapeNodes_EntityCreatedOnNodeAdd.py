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
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper

editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID
newEntityId = None


class TestShapeNodeEntityCreate(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="ShapeNodeEntityCreate", args=["level"])

    def run_test(self):

        def onEntityCreated(parameters):
            global newEntityId
            newEntityId = parameters[0]

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
        # from adding shape nodes has the appropriate Shape Component
        handler = editor.EditorEntityContextNotificationBusHandler()
        handler.connect()
        handler.add_callback('OnEditorEntityCreated', onEntityCreated)

        # Shape mapping with the key being the node name and the value is the
        # expected Component that should be added to the Entity created for the node
        shapes = {
            'BoxShapeNode': 'Box Shape',
            'CapsuleShapeNode': 'Capsule Shape',
            'CompoundShapeNode': 'Compound Shape',
            'CylinderShapeNode': 'Cylinder Shape',
            'PolygonPrismShapeNode': 'Polygon Prism Shape',
            'SphereShapeNode': 'Sphere Shape',
            'TubeShapeNode': 'Tube Shape',
            'DiskShapeNode': 'Disk Shape'
        }

        # Retrieve a mapping of the TypeIds for all the components
        # we will be checking for
        componentNames = []
        for name in shapes:
            componentNames.append(shapes[name])
        componentTypeIds = hydra.get_component_type_id_map(componentNames)

        # Create nodes for all the shapes we support and check if the Entity created by
        # adding the node has the appropriate Component added automatically to it
        newGraph = graph.GraphManagerRequestBus(bus.Broadcast, 'GetGraph', newGraphId)
        x = 10.0
        y = 10.0
        for nodeName in shapes:
            nodePosition = math.Vector2(x, y)
            node = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName',
                                                                        newGraph, nodeName)
            graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, node, nodePosition)

            shapeComponent = shapes[nodeName]
            componentTypeId = componentTypeIds[shapeComponent]
            hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId,
                                                        componentTypeId)
            self.test_success = self.test_success and hasComponent
            if hasComponent:
                self.log("{node} created new Entity with {component} Component".format(node=nodeName,
                                                                                    component=shapeComponent))

            x += 40.0
            y += 40.0

        # Stop listening for entity creation notifications
        handler.disconnect()


test = TestShapeNodeEntityCreate()
test.run()
