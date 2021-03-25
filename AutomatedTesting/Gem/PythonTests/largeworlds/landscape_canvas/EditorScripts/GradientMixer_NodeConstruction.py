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
import azlmbr.entity as entity
import azlmbr.landscapecanvas as landscapecanvas
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import automatedtesting_shared.hydra_editor_utils as hydra
from automatedtesting_shared.editor_test_helper import EditorTestHelper

editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID
newEntityId = None


class TestGradientMixerNodeConstruction(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GradientMixerNodeConstruction", args=["level"])

    def run_test(self):

        def onEntityCreated(parameters):
            global newEntityId
            newEntityId = parameters[0]

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

        # Listen for entity creation notifications so we can verify the component EntityId
        # references are set correctly when connecting slots on the nodes
        handler = editor.EditorEntityContextNotificationBusHandler()
        handler.connect()
        handler.add_callback('OnEditorEntityCreated', onEntityCreated)

        positionX = 10.0
        positionY = 10.0
        offsetX = 340.0
        offsetY = 100.0

        # Add a Box Shape node to the graph
        newGraph = graph.GraphManagerRequestBus(bus.Broadcast, 'GetGraph', newGraphId)
        boxShapeNode = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName', newGraph,
                                                                            'BoxShapeNode')
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, boxShapeNode, math.Vector2(positionX, positionY))
        boxShapeEntityId = newEntityId

        positionX += offsetX
        positionY += offsetY

        # Add a Random Noise Gradient node to the graph
        perlinNoiseNode = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName', newGraph,
                                                                               'PerlinNoiseGradientNode')
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, perlinNoiseNode, math.Vector2(positionX, positionY))
        perlinNoiseEntityId = newEntityId

        positionX += offsetX
        positionY += offsetY

        # Add a FastNoise Gradient node to the graph
        fastNoiseNode = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName', newGraph,
                                                                             'FastNoiseGradientNode')
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, fastNoiseNode, math.Vector2(positionX, positionY))
        fastNoiseEntityId = newEntityId

        positionX += offsetX
        positionY += offsetY

        # Add a Gradient Mixer node to the graph
        gradientMixerNode = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName', newGraph,
                                                                                 'GradientMixerNode')
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, gradientMixerNode, math.Vector2(positionX, positionY))
        gradientMixerEntityId = newEntityId

        boundsSlotId = graph.GraphModelSlotId('Bounds')
        previewBoundsSlotId = graph.GraphModelSlotId('PreviewBounds')
        inboundGradientSlotId = graph.GraphModelSlotId('InboundGradient')
        outboundGradientSlotId = graph.GraphModelSlotId('OutboundGradient')
        inboundGradientSlotId2 = graph.GraphControllerRequestBus(bus.Event, 'ExtendSlot', newGraphId, gradientMixerNode,
                                                                 'InboundGradient')

        # Connect slots on our nodes to construct a Gradient Mixer hierarchy
        graph.GraphControllerRequestBus(bus.Event, 'AddConnectionBySlotId', newGraphId, boxShapeNode, boundsSlotId,
                                        perlinNoiseNode, previewBoundsSlotId)
        graph.GraphControllerRequestBus(bus.Event, 'AddConnectionBySlotId', newGraphId, boxShapeNode, boundsSlotId,
                                        fastNoiseNode, previewBoundsSlotId)
        graph.GraphControllerRequestBus(bus.Event, 'AddConnectionBySlotId', newGraphId, boxShapeNode, boundsSlotId,
                                        gradientMixerNode, previewBoundsSlotId)
        graph.GraphControllerRequestBus(bus.Event, 'AddConnectionBySlotId', newGraphId, perlinNoiseNode, outboundGradientSlotId,
                                        gradientMixerNode, inboundGradientSlotId)
        graph.GraphControllerRequestBus(bus.Event, 'AddConnectionBySlotId', newGraphId, fastNoiseNode, outboundGradientSlotId,
                                        gradientMixerNode, inboundGradientSlotId2)

        # Delay to allow all the underlying component properties to be updated after the slot connections are made
        general.idle_wait(1.0)

        # Get component info
        gradientMixerTypeId = hydra.get_component_type_id("Gradient Mixer")
        perlinNoiseTypeId = hydra.get_component_type_id("Perlin Noise Gradient")
        gradientMixerOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', gradientMixerEntityId,
                                                            gradientMixerTypeId)
        gradientMixerComponent = gradientMixerOutcome.GetValue()
        perlinNoiseOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', perlinNoiseEntityId,
                                                          perlinNoiseTypeId)
        perlinNoiseComponent = perlinNoiseOutcome.GetValue()

        # Verify the Preview EntityId property on our Perlin Noise Gradient component has been set to our Box Shape's EntityId
        previewEntityId = hydra.get_component_property_value(perlinNoiseComponent, 'Preview Settings|Pin Preview to Shape')
        self.test_success = self.test_success and previewEntityId and boxShapeEntityId.invoke("Equal", previewEntityId)
        if previewEntityId and boxShapeEntityId.invoke("Equal", previewEntityId):
            self.log("Perlin Noise Gradient component Preview Entity property set to Box Shape EntityId")

        # Verify the 1st Inbound Gradient EntityId property on our Gradient Mixer component has been set to our Perlin Noise
        # Gradient's EntityId
        inboundGradientEntityId = hydra.get_component_property_value(gradientMixerComponent,
                                                                     'Configuration|Layers|[0]|Gradient|Gradient Entity Id')
        self.test_success = self.test_success and inboundGradientEntityId and perlinNoiseEntityId.invoke("Equal", inboundGradientEntityId)
        if inboundGradientEntityId and perlinNoiseEntityId.invoke("Equal", inboundGradientEntityId):
            self.log("Gradient Mixer component Inbound Gradient extendable property set to Perlin Noise Gradient EntityId")

        # Verify the 2nd Inbound Gradient EntityId property on our Gradient Mixer component has been set to our FastNoise
        # Gradient Modifier's EntityId
        inboundGradientEntityId2 = hydra.get_component_property_value(gradientMixerComponent,
                                                                      'Configuration|Layers|[1]|Gradient|Gradient Entity Id')
        self.test_success = self.test_success and inboundGradientEntityId2 and fastNoiseEntityId.invoke("Equal", inboundGradientEntityId2)
        if inboundGradientEntityId2 and fastNoiseEntityId.invoke("Equal", inboundGradientEntityId2):
            self.log("Gradient Mixer component Inbound Gradient extendable property set to FastNoise Gradient EntityId")

        # Verify that Gradient Mixer Layer Operations are properly set
        hydra.get_component_property_value(gradientMixerComponent, 'Configuration|Layers|[0]|Operation')
        hydra.get_component_property_value(gradientMixerComponent, 'Configuration|Layers|[1]|Operation')

        # Stop listening for entity creation notifications
        handler.disconnect()


test = TestGradientMixerNodeConstruction()
test.run()
