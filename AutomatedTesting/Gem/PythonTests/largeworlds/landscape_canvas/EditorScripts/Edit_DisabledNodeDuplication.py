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


newEntityId = None


def Edit_DisabledNodeDuplication():
    """
    Summary:
    This test verifies Editor stability after duplicating disabled Landscape Canvas nodes.

    Expected Behavior:
    Editor remains stable and free of crashes.

    Test Steps:
     1) Open a simple level
     2) Open Landscape Canvas and create a new graph
     3) Create several new nodes, disable the nodes via disabling/deleting components, and duplicate the nodes

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

    # Listen for entity creation notifications so when we add a new node
    # we can access the corresponding Entity that was created so that we
    # can disable/remove components on that Entity
    handler = editor.EditorEntityContextNotificationBusHandler()
    handler.connect()
    handler.add_callback('OnEditorEntityCreated', onEntityCreated)

    # Mapping of our Landscape Canvas nodes with corresponding dependent components
    # that we can disable/remove to reproduce the crash
    nodes = {
        'SpawnerAreaNode': 'Vegetation Asset List',
        'MeshBlockerAreaNode': 'Mesh',
        'BlockerAreaNode': 'Shape Reference',
        'FastNoiseGradientNode': 'Gradient Transform Modifier',
        'ImageGradientNode': 'Gradient Transform Modifier',
        'PerlinNoiseGradientNode': 'Gradient Transform Modifier',
        'RandomNoiseGradientNode': 'Gradient Transform Modifier'
    }

    # Retrieve a mapping of the TypeIds for all the components
    # we will be checking for
    componentNames = list(set(nodes.values())) # Convert to set then back to list to remove any duplicates
    componentTypeIds = hydra.get_component_type_id_map(componentNames)

    # Iterate through creating our nodes and then disabling/deleting required components
    # and then duplicating the node to reproduce the crash
    newGraph = graph.GraphManagerRequestBus(bus.Broadcast, 'GetGraph', newGraphId)
    x = 10.0
    y = 10.0
    for nodeName in nodes:
        nodePosition = math.Vector2(x, y)
        node = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName', newGraph, nodeName)
        graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, node, nodePosition)

        dependentComponentName = nodes[nodeName]
        componentTypeId = componentTypeIds[dependentComponentName]
        componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', newEntityId, componentTypeId)
        component = componentOutcome.GetValue()

        # First make sure we can duplicate a node with a dependent component that is disabled
        editor.EditorComponentAPIBus(bus.Broadcast, 'DisableComponents', [component])
        helper.wait_for_condition(lambda: not editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled',
                                                                           [component]), 1.0)
        graph.SceneRequestBus(bus.Event, 'DuplicateSelection', newGraphId) # This duplication would cause a crash without the fix
        Report.info("{node} duplicated with disabled component".format(node=nodeName))

        # Then, make sure we can duplicate the node with a dependent component that is deleted
        editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [component])
        helper.wait_for_condition(lambda: not editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType',
                                                                           componentTypeIds[dependentComponentName]), 1.0)
        graph.SceneRequestBus(bus.Event, 'DuplicateSelection', newGraphId) # This duplication would cause a crash without the fix
        Report.info("{node} duplicated with deleted component".format(node=nodeName))

        x += 40.0
        y += 40.0

    # Stop listening for entity creation notifications
    handler.disconnect()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(Edit_DisabledNodeDuplication)
