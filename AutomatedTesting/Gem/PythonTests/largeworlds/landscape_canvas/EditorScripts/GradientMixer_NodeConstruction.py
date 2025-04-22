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
    preview_entity_set = (
        "Perlin Noise Gradient component Preview Entity property set to Box Shape EntityId",
        "Unexpected entity set in Perlin Noise Gradient Preview Entity property"
    )
    mixer_inbound_gradient_set_a = (
        "Gradient Mixer component Inbound Gradient extendable property set to Perlin Noise Gradient EntityId",
        "Unexpected entity set in Gradient Mixer's Inbound Gradient property"
    )
    mixer_inbound_gradient_set_b = (
        "Gradient Mixer component Inbound Gradient extendable property set to FastNoise Gradient EntityId",
        "Unexpected entity set in Gradient Mixer's Inbound Gradient property"
    )
    mixer_operation_a = (
        "Layer 1 Operation is set to Initialize",
        "Layer 1 Operation is not set to Initialize as expected"
    )
    mixer_operation_b = (
        "Layer 2 Operation is set to Average",
        "Layer 2 Operation is not set to Average as expected"
    )

newEntityId = None


def GradientMixer_NodeConstruction():
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

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.editor.graph as graph
    import azlmbr.landscapecanvas as landscapecanvas
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.paths

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.wait_utils import PrefabWaiter
    from editor_python_test_tools.utils import Report

    editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID

    def onEntityCreated(parameters):
        global newEntityId
        newEntityId = parameters[0]

    # Open an existing simple level
    hydra.open_base_level()

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
    PrefabWaiter.wait_for_propagation()
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
    previewEntityId = hydra.get_component_property_value(perlinNoiseComponent, 'Previewer|Preview Settings|Pin Preview to Shape')
    Report.result(Tests.preview_entity_set, previewEntityId and boxShapeEntityId.invoke("Equal", previewEntityId))

    # Verify the 1st Inbound Gradient EntityId property on our Gradient Mixer component has been set to our Perlin Noise
    # Gradient's EntityId
    inboundGradientEntityId = hydra.get_component_property_value(gradientMixerComponent,
                                                                 'Configuration|Layers|[0]|Gradient|Gradient Entity Id')
    Report.result(Tests.mixer_inbound_gradient_set_a, inboundGradientEntityId and perlinNoiseEntityId.invoke("Equal", inboundGradientEntityId))

    # Verify the 2nd Inbound Gradient EntityId property on our Gradient Mixer component has been set to our FastNoise
    # Gradient Modifier's EntityId
    inboundGradientEntityId2 = hydra.get_component_property_value(gradientMixerComponent,
                                                                  'Configuration|Layers|[1]|Gradient|Gradient Entity Id')
    Report.result(Tests.mixer_inbound_gradient_set_b, inboundGradientEntityId2 and fastNoiseEntityId.invoke("Equal", inboundGradientEntityId2))

    # Verify that Gradient Mixer Layer Operations are properly set
    mixer_operation_a = hydra.get_component_property_value(gradientMixerComponent, 'Configuration|Layers|[0]|Operation')
    mixer_operation_b = hydra.get_component_property_value(gradientMixerComponent, 'Configuration|Layers|[1]|Operation')
    Report.result(Tests.mixer_operation_a, mixer_operation_a == 0)
    Report.result(Tests.mixer_operation_b, mixer_operation_b == 6)

    # Stop listening for entity creation notifications
    handler.disconnect()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GradientMixer_NodeConstruction)
