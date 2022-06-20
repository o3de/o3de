"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.
SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    prefab_instantiated = (
        "Prefab instantiated successfully",
        "Failed to instantiate prefab"
    )
    lc_entity_found = (
        "Landscape Canvas entity found",
        "Failed to find Landscape Canvas entity"
    )
    spawner_entity_found = (
        "Spawner entity found",
        "Failed to find Spawner entity"
    )
    graph_opened = (
        "Graph successfully opened",
        "Graph failed to open"
    )
    spawner_node_found = (
        "Vegetation Layer Spawner node found on graph",
        "Failed to find Vegetation Layer Spawner node on graph"
    )
    spawner_node_removed = (
        "Vegetation Layer Spawner node was successfully removed",
        "Failed to remove Vegetation Layer Spawner node"
    )


def Edit_UndoNodeDelete_PrefabEntity():
    """
    Summary:
    This test verifies Editor stability after undoing the deletion of nodes on a prefab entity.
    Expected Behavior:
    Editor remains stable and free of crashes.
    Test Steps:
     1) Open a simple level
     2) Instantiate a prefab with a Landscape Canvas setup
     3) Find a specific node on the graph, and delete it
     4) Restore the node with Undo
    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.
    :return: None
    """

    import os

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.entity as entity
    import azlmbr.legacy.general as general
    import azlmbr.editor.graph as graph
    import azlmbr.landscapecanvas as landscapecanvas
    import azlmbr.math as math
    import azlmbr.prefab as prefab

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Open an existing simple level
    hydra.open_base_level()

    # Instantiate prefab
    transform = math.Transform_CreateIdentity()
    position = math.Vector3(64.0, 64.0, 32.0)
    transform.invoke('SetPosition', position)
    test_prefab_path = os.path.join("Assets", "Prefabs", "BushFlowerBlender.prefab")
    prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'InstantiatePrefab', test_prefab_path,
                                                  entity.EntityId(), position)
    Report.critical_result(Tests.prefab_instantiated, prefab_result.IsSuccess())

    # Find root entity in the loaded level
    search_filter = entity.SearchFilter()
    search_filter.names = ["LandscapeCanvas"]

    # Allow a few seconds for matching entity to be found
    helper.wait_for_condition(lambda: len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)) > 0, 5.0)
    lc_matching_entities = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
    prefab_root_id = lc_matching_entities[0]  # Entity with Landscape Canvas component
    Report.result(Tests.lc_entity_found, prefab_root_id.IsValid())

    # Find the BushSpawner entity
    search_filter.names = ["BushSpawner"]
    spawner_matching_entities = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
    spawner_id = spawner_matching_entities[0]  # Entity with Vegetation Layer Spawner component
    Report.result(Tests.spawner_entity_found, spawner_id.IsValid())

    # Open Landscape Canvas and the existing graph
    general.open_pane('Landscape Canvas')
    open_graph = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'OnGraphEntity', prefab_root_id)
    Report.result(Tests.graph_opened, open_graph.IsValid())

    # Get needed component type ids
    layer_spawner_type_id = hydra.get_component_type_id("Vegetation Layer Spawner")

    # Find the Vegetation Layer Spawner node on the BushSpawner entity
    layer_spawner_component = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', spawner_id,
                                                           layer_spawner_type_id)
    layer_spawner_component_component_id = layer_spawner_component.GetValue()
    layer_spawner_node = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'GetAllNodesMatchingEntityComponent',
                                                                   layer_spawner_component_component_id)
    Report.result(Tests.spawner_node_found, layer_spawner_node is not None)

    # Remove the Layer Spawner node
    graph.GraphControllerRequestBus(bus.Event, "RemoveNode", open_graph, layer_spawner_node[0])

    # Verify node was removed
    layer_spawner_node = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'GetAllNodesMatchingEntityComponent',
                                                                   layer_spawner_component_component_id)
    Report.result(Tests.spawner_node_removed, not layer_spawner_node)

    # Undo the Node deletion. This is required to be executed twice to hit the node removal.
    general.undo()
    general.undo()

    # self.log a line to the Console to verify the Editor is still active
    Report.info("Editor is still responsive")


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(Edit_UndoNodeDelete_PrefabEntity)
