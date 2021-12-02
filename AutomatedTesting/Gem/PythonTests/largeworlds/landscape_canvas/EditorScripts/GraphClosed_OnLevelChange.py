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
    graph_closed = (
        "Graph closed on level change",
        "Graph is still open after level change"
    )


def GraphClosed_OnLevelChange():
    """
    Summary:
    This test verifies that Landscape Canvas graphs are auto-closed when the currently open level changes.

    Expected Behavior:
    When a new level is loaded in the Editor, open Landscape Canvas graphs are automatically closed.

    Test Steps:
     1) Open a simple level
     2) Open Landscape Canvas and create a new graph
     3) Open a different level
     4) Verify the open graph is closed

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import azlmbr.bus as bus
    import azlmbr.editor.graph as graph
    import azlmbr.legacy.general as general

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID

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

    # Open a different level, which should close any open Landscape Canvas graphs
    general.open_level_no_prompt('WhiteBox/EmptyLevel')

    # Make sure the graph we created is now closed
    graphIsOpen = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, newGraphId)
    Report.result(Tests.graph_closed, not graphIsOpen)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GraphClosed_OnLevelChange)
