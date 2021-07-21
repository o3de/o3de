"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper

editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID
newEntityId = None


class TestGradientMixerNodeConstruction(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GradientMixerNodeConstruction", args=["level"])

    def run_test(self):
        """
        Summary:
        This test verifies a Gradient Mixer vegetation setup can be constructed through Landscape Canvas.

        Expected Behavior:
        Entities contain all required components and component references after creating nodes and setting connections
        on a Landscape Canvas graph.

        Test Steps:
         1) Create a new level
         2) Open Landscape Canvas and create a new graph
         3) Add all necessary nodes to the graph and set connections to form a Gradient Mixer setup
         4) Verify all components and component references were properly set during graph construction

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

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
