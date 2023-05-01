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
    dependencies_added = (
        "Node created new Entity with all required components",
        "Failed to create node with all required components"
    )


newEntityId = None


def TerrainNodes_DependentComponentsAdded():
    """
    Summary:
    This test verifies that the Landscape Canvas nodes can be added to a graph, and correctly create entities with
    proper dependent components.

    Expected Behavior:
    All expected component dependencies are met when adding an area node to a graph.

    Test Steps:
     1) Open a simple level
     2) Open Landscape Canvas and create a new graph
     3) Drag each of the terrain nodes to the graph area, and ensure the proper dependent components are added
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

    # Listen for entity creation notifications so we can check if the entity created
    # from adding these terrain area nodes has the main target Terrain Layer Component
    # as well as automatically adding all required dependency components
    handler = editor.EditorEntityContextNotificationBusHandler()
    handler.connect()
    handler.add_callback('OnEditorEntityCreated', onEntityCreated)

    # Terrain node mapping with the key being the node name and the value is the
    # expected Components that should be added to the Entity created for the node
    areas = {
        'TerrainMacroMaterialNode': [
            'Terrain Macro Material',
            'Axis Aligned Box Shape'
        ],
        'TerrainLayerSpawnerNode': [
            'Terrain Layer Spawner',
            'Axis Aligned Box Shape'
        ],
        'TerrainSurfaceMaterialsListNode': [
            'Terrain Surface Materials List',
            'Axis Aligned Box Shape'
        ],
    }

    # Retrieve a mapping of the TypeIds for all the components
    # we will be checking for
    componentNames = []
    for name in areas:
        componentNames.extend(areas[name])
    componentTypeIds = hydra.get_component_type_id_map(componentNames)

    # Create nodes for the terrain components that have additional required dependencies and check if
    # the Entity created by adding the node has the appropriate component and required
    # additional components added automatically to it
    newGraph = graph.GraphManagerRequestBus(bus.Broadcast, 'GetGraph', newGraphId)
    x = 10.0
    y = 10.0
    for nodeName in areas:
        nodePosition = math.Vector2(x, y)
        node = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName',
                                                                    newGraph, nodeName)
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, node, nodePosition)

        components = areas[nodeName]
        success = False
        for component in components:
            componentTypeId = componentTypeIds[component]
            success = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId,
                                                   componentTypeId)
            if not success:
                break
        Report.info(nodeName)
        Report.result(Tests.dependencies_added, success)

        x += 40.0
        y += 40.0

    # Stop listening for entity creation notifications
    handler.disconnect()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(TerrainNodes_DependentComponentsAdded)
