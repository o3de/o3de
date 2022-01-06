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
    lc_component_added = (
        "Root entity created with the Landscape Canvas component",
        "Landscape Canvas component was not found on the root entity"
    )
    lc_tool_closed = (
        "Landscape Canvas tool closed",
        "Failed to close Landscape Canvas tool"
    )


new_root_entity_id = None


def NewGraph_CreatedSuccessfully():
    """
    Summary:
    This test verifies that new graphs can be created in Landscape Canvas.

    Expected Behavior:
    New graphs can be created, and proper entity is created to hold graph data with a Landscape Canvas component.

    Test Steps:
     1) Open a simple level
     2) Open Landscape Canvas and create a new graph
     3) Ensures the root entity created contains a Landscape Canvas component

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

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID

    def on_entity_created(parameters):
        global new_root_entity_id
        new_root_entity_id = parameters[0]

    # Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # Listen for entity creation notifications so we can check if the entity created
    # with the new graph has our Landscape Canvas component automatically added
    handler = editor.EditorEntityContextNotificationBusHandler()
    handler.connect()
    handler.add_callback("OnEditorEntityCreated", on_entity_created)

    # Open Landscape Canvas tool and verify
    general.open_pane('Landscape Canvas')
    Report.critical_result(Tests.lc_tool_opened, general.is_pane_visible('Landscape Canvas'))

    # Create a new graph in Landscape Canvas
    new_graph_id = graph.AssetEditorRequestBus(bus.Event, 'CreateNewGraph', editorId)
    Report.critical_result(Tests.new_graph_created, new_graph_id is not None)

    # Make sure the graph we created is in Landscape Canvas
    graph_registered = graph.AssetEditorRequestBus(bus.Event, 'ContainsGraph', editorId, new_graph_id)
    Report.result(Tests.graph_registered, graph_registered)

    # Check if the entity created when we create a new graph has the
    # Landscape Canvas component already added to it
    landscape_canvas_type_id = hydra.get_component_type_id("Landscape Canvas")
    success = editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", new_root_entity_id,
                                           landscape_canvas_type_id)
    Report.result(Tests.lc_component_added, success)

    # Close Landscape Canvas tool and verify
    general.close_pane("Landscape Canvas")
    Report.result(Tests.lc_tool_closed, not general.is_pane_visible("Landscape Canvas"))

    # Stop listening for entity creation notifications
    handler.disconnect()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(NewGraph_CreatedSuccessfully)
