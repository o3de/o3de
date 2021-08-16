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
    component_added = (
        "New entity created with the expected component",
        "Expected component was not found on entity"
    )


newEntityId = None


def AreaNodes_EntityCreatedOnNodeAdd():
    """
    Summary:
    This test verifies that the Landscape Canvas nodes can be added to a graph, and correctly create entities.

    Expected Behavior:
    New entities are created when dragging area nodes to graph area.

    Test Steps:
     1) Open a simple level
     2) Open Landscape Canvas and create a new graph
     3) Drag each of the area nodes to the graph area, and ensure a new entity is created

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

    # Listen for entity creation notifications so we can check if the entity created
    # from adding vegetation area nodes has the appropriate Vegetation Layer Component
    handler = editor.EditorEntityContextNotificationBusHandler()
    handler.connect()
    handler.add_callback('OnEditorEntityCreated', onEntityCreated)

    # Vegetation Area mapping with the key being the node name and the value is the
    # expected Component that should be added to the Entity created for the node
    areas = {
        'AreaBlenderNode': 'Vegetation Layer Blender',
        'BlockerAreaNode': 'Vegetation Layer Blocker',
        'MeshBlockerAreaNode': 'Vegetation Layer Blocker (Mesh)',
        'SpawnerAreaNode': 'Vegetation Layer Spawner'
    }

    # Retrieve a mapping of the TypeIds for all the components
    # we will be checking for
    componentNames = []
    for name in areas:
        componentNames.append(areas[name])
    componentTypeIds = hydra.get_component_type_id_map(componentNames)

    # Create nodes for all the vegetation areas we support and check if the Entity created by
    # adding the node has the appropriate Component added automatically to it
    newGraph = graph.GraphManagerRequestBus(bus.Broadcast, 'GetGraph', newGraphId)
    x = 10.0
    y = 10.0
    for nodeName in areas:
        nodePosition = math.Vector2(x, y)
        node = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName', newGraph, nodeName)
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, node, nodePosition)

        areaComponent = areas[nodeName]
        componentTypeId = componentTypeIds[areaComponent]
        hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', newEntityId, componentTypeId)
        Report.info(f"Node: {nodeName} | Component: {areaComponent}")
        Report.result(Tests.component_added, hasComponent)

        x += 40.0
        y += 40.0

    # Stop listening for entity creation notifications
    handler.disconnect()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(AreaNodes_EntityCreatedOnNodeAdd)
