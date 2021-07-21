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
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper

editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID
newEntityId = None


class TestSlotConnectionsUpdateComponents(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="SlotConnectionsUpdateComponents", args=["level"])

    def run_test(self):
        """
        Summary:
        This test verifies that the Landscape Canvas slot connections properly update component references.

        Expected Behavior:
        A reference created through slot connections in Landscape Canvas is reflected in the Entity Inspector.

        Test Steps:
         1) Create a new level
         2) Open Landscape Canvas and create a new graph
         3) Several nodes are added to a graph, and connections are set between the nodes
         4) Component references are verified via Entity Inspector

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        # Retrieve the proper component TypeIds per component name
        componentNames = [
            'Random Noise Gradient',
            'Dither Gradient Modifier',
            'Gradient Mixer'
        ]
        componentTypeIds = hydra.get_component_type_id_map(componentNames)

        # Helper method for retrieving an EntityId from a specific property on a component
        def getEntityIdFromComponentProperty(targetEntityId, componentTypeName, propertyPath):
            componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', targetEntityId,
                                                            componentTypeIds[componentTypeName])
            if not componentOutcome.IsSuccess():
                return None

            component = componentOutcome.GetValue()
            propertyOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component,
                                                           propertyPath)
            if not propertyOutcome.IsSuccess():
                return None

            return propertyOutcome.GetValue()

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

        # Listen for entity creation notifications so we can verify the component EntityId
        # references are set correctly when connecting slots on the nodes
        handler = editor.EditorEntityContextNotificationBusHandler()
        handler.connect()
        handler.add_callback('OnEditorEntityCreated', onEntityCreated)

        positionX = 10.0
        positionY = 10.0
        offsetX = 340.0
        offsetY = 100.0

        # Add a box shape node to the graph
        newGraph = graph.GraphManagerRequestBus(bus.Broadcast, 'GetGraph', newGraphId)
        boxShapeNode = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName',
                                                                            newGraph, 'BoxShapeNode')
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, boxShapeNode, math.Vector2(positionX,
                                                                                                     positionY))
        boxShapeEntityId = newEntityId

        positionX += offsetX
        positionY += offsetY

        # Add a random noise gradient node to the graph
        randomNoiseNode = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName',
                                                                               newGraph, 'RandomNoiseGradientNode')
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, randomNoiseNode, math.Vector2(positionX,
                                                                                                        positionY))
        randomNoiseEntityId = newEntityId

        positionX += offsetX
        positionY += offsetY

        # Add a dither gradient modifier node to the graph
        ditherNode = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName',
                                                                          newGraph, 'DitherGradientModifierNode')
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, ditherNode, math.Vector2(positionX,
                                                                                                   positionY))
        ditherEntityId = newEntityId

        positionX += offsetX
        positionY += offsetY

        # Add a gradient mixer node to the graph
        gradientMixerNode = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName',
                                                                                 newGraph, 'GradientMixerNode')
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, gradientMixerNode, math.Vector2(positionX,
                                                                                                          positionY))
        gradientMixerEntityId = newEntityId

        boundsSlotId = graph.GraphModelSlotId('Bounds')
        previewBoundsSlotId = graph.GraphModelSlotId('PreviewBounds')
        inboundGradientSlotId = graph.GraphModelSlotId('InboundGradient')
        outboundGradientSlotId = graph.GraphModelSlotId('OutboundGradient')

        # Connect slots on our nodes to test all slot types like so:
        #     Shape -> Gradient -> Gradient Modifier -> Gradient Mixer
        #
        # Which tests the following slot types:
        #     * Shape -> Preview Bounds
        #     * Gradient Output -> Gradient Modifier
        #     * Gradient Output -> Gradient Mixer (extendable slots)
        graph.GraphControllerRequestBus(bus.Event, 'AddConnectionBySlotId', newGraphId, boxShapeNode, boundsSlotId,
                                        randomNoiseNode, previewBoundsSlotId)
        graph.GraphControllerRequestBus(bus.Event, 'AddConnectionBySlotId', newGraphId, randomNoiseNode,
                                        outboundGradientSlotId, ditherNode, inboundGradientSlotId)
        graph.GraphControllerRequestBus(bus.Event, 'AddConnectionBySlotId', newGraphId, ditherNode,
                                        outboundGradientSlotId, gradientMixerNode, inboundGradientSlotId)

        # Delay to allow all the underlying component properties to be updated after the slot connections are made
        general.idle_wait(1.0)

        # Verify the Preview EntityId property on our Random Noise Gradient component has been set to our Box Shape's
        # EntityId
        previewEntityId = getEntityIdFromComponentProperty(randomNoiseEntityId, 'Random Noise Gradient',
                                                           'Preview Settings|Pin Preview to Shape')
        random_gradient_success = previewEntityId and boxShapeEntityId.invoke("Equal", previewEntityId)
        self.test_success = self.test_success and random_gradient_success
        if random_gradient_success:
            self.log("Random Noise Gradient component Preview Entity property set to Box Shape EntityId")

        # Verify the Inbound Gradient EntityId property on our Dither Gradient Modifier component has been set to our
        # Random Noise Gradient's EntityId
        inboundGradientEntityId = getEntityIdFromComponentProperty(ditherEntityId, 'Dither Gradient Modifier',
                                                                   'Configuration|Gradient|Gradient Entity Id')
        dither_gradient_success = inboundGradientEntityId and randomNoiseEntityId.invoke("Equal",
                                                                                         inboundGradientEntityId)
        self.test_success = self.test_success and dither_gradient_success
        if dither_gradient_success:
            self.log("Dither Gradient Modifier component Inbound Gradient property set to Random Noise Gradient "
                     "EntityId")

        # Verify the Inbound Gradient Mixer EntityId property on our Gradient Mixer component has been set to our
        # Dither Gradient Modifier's EntityId
        inboundGradientMixerEntityId = getEntityIdFromComponentProperty(gradientMixerEntityId, 'Gradient Mixer',
                                                                        'Configuration|Layers|[0]|Gradient|Gradient Entity Id')
        gradient_mixer_success = inboundGradientMixerEntityId and ditherEntityId.invoke("Equal",
                                                                                        inboundGradientMixerEntityId)
        self.test_success = self.test_success and gradient_mixer_success
        if gradient_mixer_success:
            self.log("Gradient Mixer component Inbound Gradient extendable property set to Dither Gradient Modifier "
                     "EntityId")

        # Stop listening for entity creation notifications
        handler.disconnect()


test = TestSlotConnectionsUpdateComponents()
test.run()
