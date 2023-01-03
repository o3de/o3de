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


createdEntityId = None
deletedEntityId = None


def ShapeNodes_EntityRemovedOnNodeDelete():
    """
    Summary:
    This test verifies that the Landscape Canvas node deletion properly cleans up entities in the Editor.

    Expected Behavior:
    Entities are removed when shape nodes are deleted from a graph.

    Test Steps:
     1) Open a simple level
     2) Open Landscape Canvas and create a new graph
     3) Drag each of the shape nodes to the graph area, and ensure a new entity is created
     4) Delete the nodes, and ensure the newly created entities are removed

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

    editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID

    def onEntityCreated(parameters):
        global createdEntityId
        createdEntityId = parameters[0]

    def onEntityDeleted(parameters):
        global deletedEntityId
        deletedEntityId = parameters[0]

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

    # Listen for entity creation/deletion notifications so we can verify the
    # Entity created when adding a new also gets deleted when the node is deleted
    handler = editor.EditorEntityContextNotificationBusHandler()
    handler.connect()
    handler.add_callback('OnEditorEntityCreated', onEntityCreated)
    handler.add_callback('OnEditorEntityDeleted', onEntityDeleted)

    # All of the shape nodes that we support
    shapes = [
        'BoxShapeNode',
        'CapsuleShapeNode',
        'CompoundShapeNode',
        'CylinderShapeNode',
        'PolygonPrismShapeNode',
        'SphereShapeNode',
        'TubeShapeNode',
        'DiskShapeNode',
        'AxisAlignedBoxShapeNode',
        'ReferenceShapeNode'
    ]

    # Create nodes for all the shapes we support and check if the Entity is created
    # and then deleted when the node is removed
    newGraph = graph.GraphManagerRequestBus(bus.Broadcast, 'GetGraph', newGraphId)
    x = 10.0
    y = 10.0
    for nodeName in shapes:
        nodePosition = math.Vector2(x, y)
        node = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName', newGraph, nodeName)
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, node, nodePosition)

        removed = graph.GraphControllerRequestBus(bus.Event, 'RemoveNode', newGraphId, node)

        # Verify that the created Entity for this node matches the Entity that gets
        # deleted when the node is removed
        Report.info(f"Node: {nodeName}")
        Report.result(Tests.entity_deleted, removed and createdEntityId.invoke("Equal", deletedEntityId))

    # Stop listening for entity creation/deletion notifications
    handler.disconnect()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(ShapeNodes_EntityRemovedOnNodeDelete)
