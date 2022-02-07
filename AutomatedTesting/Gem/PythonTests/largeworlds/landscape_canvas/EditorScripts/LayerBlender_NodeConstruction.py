"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    lc_tool_opened = (
        "Landscape Canvas tool opened",
        "Failed to open Landscape Canvas tool"
    )
    new_graph_created = (
        "Successfully created new graph",
        "Failed to create new graph"
    )
    graph_registered = (
        "Graph registered with Landscape Canvas",
        "Failed to register graph"
    )
    blender_first_layer_set = (
        "Spawner entity set as the first layer of the Vegetation Layer Blender component",
        "Unexpected entity set as the first layer of the Vegetation Layer Blender component"
    )
    blender_second_layer_set = (
        "Blocker entity set as the second layer of the Vegetation Layer Blender component",
        "Unexpected entity set as the second layer of the Vegetation Layer Blender component"
    )


newEntityId = None


def LayerBlender_NodeConstruction():
    """
    Summary:
    This test verifies a Layer Blender vegetation setup can be constructed through Landscape Canvas.

    Expected Behavior:
    Entities contain all required components and component references after creating nodes and setting connections
    on a Landscape Canvas graph.

    Test Steps:
     1) Open a simple level
     2) Open Landscape Canvas and create a new graph
     3) Add all necessary nodes to the graph and set connections to form a Layer Blender setup
     4) Verify all components and component references were properly set during graph construction

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.editor.graph as graph
    import azlmbr.landscapecanvas as landscapecanvas
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID

    def onEntityCreated(parameters):
        global newEntityId
        newEntityId = parameters[0]

    # Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # Open Landscape Canvas tool and verify
    general.open_pane('Landscape Canvas')
    Report.critical_result(Tests.lc_tool_opened, general.is_pane_visible('Landscape Canvas'))

    # Create a new graph in Landscape Canvas
    newGraphId = graph.AssetEditorRequestBus(bus.Event, 'CreateNewGraph', editorId)
    Report.critical_result(Tests.new_graph_created, newGraphId is not None)

    # Make sure the graph we created is in Landscape Canvas
    graph_registered = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId)
    Report.result(Tests.graph_registered, graph_registered)

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
    Report.result(Tests.blender_first_layer_set, area1EntityId and layerSpawnerEntityId.invoke("Equal", area1EntityId))

    area2EntityId = hydra.get_component_property_value(layerBlenderComponent, 'Configuration|Vegetation Areas|[1]')
    Report.result(Tests.blender_second_layer_set, area2EntityId and layerBlockerEntityId.invoke("Equal", area2EntityId))

    # Stop listening for entity creation notifications
    handler.disconnect()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(LayerBlender_NodeConstruction)
