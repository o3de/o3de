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
        "Random Noise Gradient component Preview Entity property set to Box Shape EntityId",
        "Unexpected entity set in Random Noise Gradient Preview Entity property"
    )
    dither_inbound_gradient_set = (
        "Dither Gradient Modifier component Inbound Gradient property set to Random Noise Gradient EntityId",
        "Unexpected entity set in Dither Gradient's Inbound Gradient property"
    )
    mixer_inbound_gradient_set = (
        "Gradient Mixer component Inbound Gradient extendable property set to Dither Gradient Modifier EntityId",
        "Unexpected entity set in Gradient Mixer's Inbound Gradient property"
    )


newEntityId = None


def SlotConnections_UpdateComponentReferences():
    """
    Summary:
    This test verifies that the Landscape Canvas slot connections properly update component references.

    Expected Behavior:
    A reference created through slot connections in Landscape Canvas is reflected in the Entity Inspector.

    Test Steps:
     1) Open an existing level
     2) Open Landscape Canvas and create a new graph
     3) Several nodes are added to a graph, and connections are set between the nodes
     4) Component references are verified via Entity Inspector

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

    PrefabWaiter.wait_for_propagation()

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
                                                       'Previewer|Preview Settings|Pin Preview to Shape')
    random_gradient_success = previewEntityId and boxShapeEntityId.invoke("Equal", previewEntityId)
    Report.result(Tests.preview_entity_set, random_gradient_success)

    # Verify the Inbound Gradient EntityId property on our Dither Gradient Modifier component has been set to our
    # Random Noise Gradient's EntityId
    inboundGradientEntityId = getEntityIdFromComponentProperty(ditherEntityId, 'Dither Gradient Modifier',
                                                               'Configuration|Gradient|Gradient Entity Id')
    dither_gradient_success = inboundGradientEntityId and randomNoiseEntityId.invoke("Equal",
                                                                                     inboundGradientEntityId)
    Report.result(Tests.dither_inbound_gradient_set, dither_gradient_success)

    # Verify the Inbound Gradient Mixer EntityId property on our Gradient Mixer component has been set to our
    # Dither Gradient Modifier's EntityId
    inboundGradientMixerEntityId = getEntityIdFromComponentProperty(gradientMixerEntityId, 'Gradient Mixer',
                                                                    'Configuration|Layers|[0]|Gradient|Gradient Entity Id')
    gradient_mixer_success = inboundGradientMixerEntityId and ditherEntityId.invoke("Equal",
                                                                                    inboundGradientMixerEntityId)
    Report.result(Tests.mixer_inbound_gradient_set, gradient_mixer_success)

    # Stop listening for entity creation notifications
    handler.disconnect()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(SlotConnections_UpdateComponentReferences)
