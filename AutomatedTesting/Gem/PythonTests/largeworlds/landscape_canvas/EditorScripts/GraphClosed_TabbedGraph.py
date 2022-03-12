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
    graph_open = (
        "Graph is open in Landscape Canvas",
        "Graph is not open in Landscape Canvas"
    )
    tabbed_graph_closed = (
        "Tabbed graph closed independently",
        "Closing tabbed graph resulted in unexpected graphs closing"
    )


def GraphClosed_TabbedGraphClosesIndependently():
    """
    Summary:
    This test verifies that Landscape Canvas tabbed graphs can be independently closed.

    Expected Behavior:
    Closing a tabbed graph only closes the appropriate graph.

    Test Steps:
     1) Open a simple level
     2) Open Landscape Canvas and create several new graphs
     3) Close one of the open graphs
     4) Ensure the graph properly closed, and other open graphs remain open

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

    editor_id = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID

    def create_new_graph():
        new_graph_id = graph.AssetEditorRequestBus(bus.Event, 'CreateNewGraph', editor_id)
        return new_graph_id

    def is_graph_open(graph_id):
        graph_open = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editor_id, graph_id)
        return graph_open

    # Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # Open Landscape Canvas tool and verify
    general.open_pane('Landscape Canvas')
    Report.critical_result(Tests.lc_tool_opened, general.is_pane_visible('Landscape Canvas'))

    # Create 3 new graphs in Landscape Canvas and make sure the graphs we created are open in Landscape Canvas
    graph1 = create_new_graph()
    Report.result(Tests.new_graph_created, graph1 is not None)
    Report.result(Tests.graph_open, is_graph_open(graph1))

    graph2 = create_new_graph()
    Report.result(Tests.new_graph_created, graph2 is not None)
    Report.result(Tests.graph_open, is_graph_open(graph2))

    graph3 = create_new_graph()
    Report.result(Tests.new_graph_created, graph3 is not None)
    Report.result(Tests.graph_open, is_graph_open(graph3))

    # Close a single graph and verify it was properly closed and other graphs remain open
    graph.AssetEditorRequestBus(bus.Event, 'CloseGraph', editor_id, graph2)
    Report.result(Tests.tabbed_graph_closed, not is_graph_open(graph2) and is_graph_open(graph1) and
                  is_graph_open(graph3))


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GraphClosed_TabbedGraphClosesIndependently)
