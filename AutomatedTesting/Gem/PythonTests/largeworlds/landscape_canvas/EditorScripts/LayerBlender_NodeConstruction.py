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


class TestLayerBlenderNodeConstruction(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="LayerBlenderNodeConstruction", args=["level"])

    def run_test(self):
        """
        Summary:
        This test verifies a Layer Blender vegetation setup can be constructed through Landscape Canvas.

        Expected Behavior:
        Entities contain all required components and component references after creating nodes and setting connections
        on a Landscape Canvas graph.

        Test Steps:
         1) Create a new level
         2) Open Landscape Canvas and create a new graph
         3) Add all necessary nodes to the graph and set connections to form a Layer Blender setup
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

        # Add a Vegetation Layer Spawner node to the graph
        newGraph = graph.GraphManagerRequestBus(bus.Broadcast, 'GetGraph', newGraphId)
        layerSpawnerNode = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName',
                                                                                newGraph, 'SpawnerAreaNode')
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, layerSpawnerNode, math.Vector2(positionX, positionY))
        layerSpawnerEntityId = newEntityId

        positionX += offsetX
        positionY += offsetY

        # Add a Vegetation Layer Blocker node to the graph
        layerBlockerNode = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName',
                                                                                newGraph, 'BlockerAreaNode')
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, layerBlockerNode, math.Vector2(positionX, positionY))
        layerBlockerEntityId = newEntityId

        positionX += offsetX
        positionY += offsetY

        # Add a Vegetation Layer Blender node to the graph
        layerBlenderNode = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName',
                                                                                newGraph, 'AreaBlenderNode')
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, layerBlenderNode, math.Vector2(positionX, positionY))
        layerBlenderNodeEntityId = newEntityId

        positionX += offsetX
        positionY += offsetY

        outboundAreaSlotId = graph.GraphModelSlotId('OutboundArea')
        inboundAreaSlotId = graph.GraphModelSlotId('InboundArea')
        inboundAreaSlotId2 = graph.GraphControllerRequestBus(bus.Event, 'ExtendSlot', newGraphId, layerBlenderNode,
                                                             'InboundArea')

        # Connect slots on our nodes to construct a Vegetation Layer Blender hierarchy
        graph.GraphControllerRequestBus(bus.Event, 'AddConnectionBySlotId', newGraphId, layerSpawnerNode, outboundAreaSlotId,
                                        layerBlenderNode, inboundAreaSlotId)
        graph.GraphControllerRequestBus(bus.Event, 'AddConnectionBySlotId', newGraphId, layerBlockerNode, outboundAreaSlotId,
                                        layerBlenderNode, inboundAreaSlotId2)

        # Delay to allow all the underlying component properties to be updated after the slot connections are made
        general.idle_wait(1.0)

        # Get component info
        layerBlenderTypeId = hydra.get_component_type_id("Vegetation Layer Blender")
        vegetationLayerBlenderOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType',
                                                                     layerBlenderNodeEntityId, layerBlenderTypeId)
        layerBlenderComponent = vegetationLayerBlenderOutcome.GetValue()

        # Verify the Vegetation Areas properties on our Vegetation Layer Blender component have been set to our area EntityIds
        area1EntityId = hydra.get_component_property_value(layerBlenderComponent, 'Configuration|Vegetation Areas|[0]')
        self.test_success = self.test_success and area1EntityId and layerSpawnerEntityId.invoke("Equal", area1EntityId)
        if area1EntityId and layerSpawnerEntityId.invoke("Equal", area1EntityId):
            self.log("Vegetation Layer Blender component Vegetation Areas[0] property set to Vegetation Layer Spawner EntityId")

        area2EntityId = hydra.get_component_property_value(layerBlenderComponent, 'Configuration|Vegetation Areas|[1]')
        self.test_success = self.test_success and area2EntityId and layerBlockerEntityId.invoke("Equal", area2EntityId)
        if area2EntityId and layerBlockerEntityId.invoke("Equal", area2EntityId):
            self.log("Vegetation Layer Blender component Vegetation Areas[1] property set to Vegetation Layer Blocker EntityId")

        # Stop listening for entity creation notifications
        handler.disconnect()


test = TestLayerBlenderNodeConstruction()
test.run()
