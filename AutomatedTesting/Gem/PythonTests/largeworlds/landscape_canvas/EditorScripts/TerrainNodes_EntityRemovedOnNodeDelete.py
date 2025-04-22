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
    entity_deleted = (
        "Entity was deleted when node was removed",
        "Entity was not deleted as expected when node was removed"
    )


created_entity_id = None
deleted_entity_id = None


def TerrainNodes_EntityRemovedOnNodeDelete():
    """
    Summary:
    This test verifies that entities created automatically by Landscape Canvas are properly removed when the node
    that created them is removed.

    Expected Behavior:
    Entities are properly deleted when corresponding nodes are removed.

    Test Steps:
     1) Open a simple level
     2) Open Landscape Canvas and create a new graph
     3) Drag each of the terrain nodes to the graph area, and ensure the newly created entity is deleted when the node
     is removed.
    """

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.editor.graph as graph
    import azlmbr.landscapecanvas as landscapecanvas
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report

    editor_id = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID

    def on_entity_created(parameters):
        global created_entity_id
        created_entity_id = parameters[0]

    def on_entity_deleted(parameters):
        global deleted_entity_id
        deleted_entity_id = parameters[0]

    # Open an existing simple level
    hydra.open_base_level()

    # Open Landscape Canvas tool and verify
    general.open_pane("Landscape Canvas")
    Report.critical_result(Tests.lc_tool_opened, general.is_pane_visible("Landscape Canvas"))

    # Create a new graph in Landscape Canvas
    new_graph_id = graph.AssetEditorRequestBus(bus.Event, "CreateNewGraph", editor_id)
    Report.critical_result(Tests.new_graph_created, new_graph_id is not None)

    # Make sure the graph we created is in Landscape Canvas
    graph_registered = graph.AssetEditorRequestBus(bus.Event, "ContainsGraph", editor_id, new_graph_id)
    Report.result(Tests.graph_registered, graph_registered)

    # Listen for entity creation/deletion notifications, so we can check if the entity created
    # from adding terrain nodes is appropriately removed along with the node
    handler = editor.EditorEntityContextNotificationBusHandler()
    handler.connect()
    handler.add_callback("OnEditorEntityCreated", on_entity_created)
    handler.add_callback("OnEditorEntityDeleted", on_entity_deleted)

    # All the unwrapped Terrain nodes that can be added to a graph
    areas = [
        "TerrainMacroMaterialNode",
        "TerrainLayerSpawnerNode",
        "TerrainSurfaceMaterialsListNode"
    ]

    # Create nodes for all the terrain components we support and check if the Entity is created and then deleted when
    # the node is removed
    new_graph = graph.GraphManagerRequestBus(bus.Broadcast, "GetGraph", new_graph_id)
    x = 10.0
    y = 10.0
    for node_name in areas:
        node_position = math.Vector2(x, y)
        node = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, "CreateNodeForTypeName", new_graph, node_name)
        graph.GraphControllerRequestBus(bus.Event, "AddNode", new_graph_id, node, node_position)

        removed = graph.GraphControllerRequestBus(bus.Event, "RemoveNode", new_graph_id, node)

        # Verify that the created Entity for this node matches the Entity that gets
        # deleted when the node is removed
        Report.info(f"Node: {node_name}")
        Report.result(Tests.entity_deleted, removed and created_entity_id.invoke("Equal", deleted_entity_id))

    # Stop listening for entity creation notifications
    handler.disconnect()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(TerrainNodes_EntityRemovedOnNodeDelete)
