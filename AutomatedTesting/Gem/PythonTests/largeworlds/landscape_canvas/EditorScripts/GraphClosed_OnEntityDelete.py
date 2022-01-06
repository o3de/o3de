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
    graph_registered = (
        "Graph registered with Landscape Canvas",
        "Failed to register graph"
    )
    graph_closed = (
        "Graph closed on entity delete",
        "Graph is still open after entity delete"
    )


newRootEntityId = None


def GraphClosed_OnEntityDelete():
    """
    Summary:
    This test verifies that Landscape Canvas graphs are auto-closed when the corresponding entity is deleted.

    Expected Behavior:
    When a Landscape Canvas root entity is deleted, the corresponding graph automatically closes.

    Test Steps:
     1) Open a simple level
     2) Open Landscape Canvas and create a new graph
     3) Delete the automatically created entity
     4) Verify the open graph is closed

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.editor.graph as graph
    import azlmbr.legacy.general as general

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID

    def onEntityCreated(parameters):
        global newRootEntityId
        newRootEntityId = parameters[0]

    # Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # Open Landscape Canvas tool and verify
    general.open_pane('Landscape Canvas')
    Report.critical_result(Tests.lc_tool_opened, general.is_pane_visible('Landscape Canvas'))

    # Listen for entity creation notifications so we can store the top-level Entity created
    # when a new graph is created, and then delete it to test if the graph is closed
    handler = editor.EditorEntityContextNotificationBusHandler()
    handler.connect()
    handler.add_callback('OnEditorEntityCreated', onEntityCreated)

    # Create a new graph in Landscape Canvas and verify
    newGraphId = graph.AssetEditorRequestBus(bus.Event, 'CreateNewGraph', editorId)
    graphIsOpen = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId)
    Report.result(Tests.graph_registered, graphIsOpen)

    # Delete the top-level Entity created by the new graph
    editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntityById', newRootEntityId)

    # We need to delay here because the closing of the graph due to Entity deletion
    # is actually queued in order to workaround an undo/redo issue
    # Alternatively, we could add a notifications bus for AssetEditorRequests
    # that could trigger when graphs are opened/closed and then do the check there
    general.idle_enable(True)
    general.idle_wait(1.0)

    # Verify that the corresponding graph is no longer open
    graphIsClosed = not graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId)
    Report.result(Tests.graph_closed, graphIsClosed)

    # Stop listening for entity creation notifications
    handler.disconnect()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GraphClosed_OnEntityDelete)
