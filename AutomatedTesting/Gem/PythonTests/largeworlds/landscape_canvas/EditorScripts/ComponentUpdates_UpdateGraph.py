"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    slice_instantiated = (
        "Slice instantiated successfully",
        "Failed to instantiate slice"
    )
    lc_entity_found = (
        "LandscapeCanvas entity found",
        "Failed to find LandscapeCanvas entity"
    )
    spawner_entity_found = (
        "BushSpawner entity found",
        "Failed to find BushSpawner entity"
    )
    dist_filter_component_found = (
        "Vegetation Distribution Filter component on BushSpawner entity found",
        "Failed to find Distribution Filter component on BushSpawner entity"
    )
    existing_graph_opened = (
        "Opened existing graph from slice",
        "Failed to open existing graph"
    )
    dist_filter_node_found = (
        "Vegetation Distribution Filter node found on graph",
        "Failed to find Distribution Filter node on graph"
    )
    alt_filter_component_found = (
        "Vegetation Altitude Filter component on BushSpawner entity found",
        "Failed to find Altitude Filter component on BushSpawner entity"
    )
    alt_filter_node_found = (
        "Vegetation Altitude Filter node found on graph",
        "Failed to find Altitude Filter node on graph"
    )
    dist_filter_component_removed = (
        "Vegetation Distribution Filter component removed from BushSpawner entity",
        "Failed to remove Distribution Filter component from BushSpawner entity"
    )
    dist_filter_node_removed = (
        "Vegetation Distribution Filter node removed from graph",
        "Failed to remove Distribution Filter node from graph"
    )
    child_entity_added = (
        "New entity successfully added as a child of the BushSpawner entity",
        "New entity added with an unexpected parent"
    )
    box_shape_component_found = (
        "Box Shape component on Box entity found",
        "Failed to find Box Shape component on Box entity"
    )
    box_shape_node_found = (
        "Box Shape node found on graph",
        "Failed to find Box Shape node on graph"
    )


def ComponentUpdates_UpdateGraph():
    """
    Summary:
    This test verifies that the Landscape Canvas graphs update properly when components are added/removed outside of
    Landscape Canvas.

    Expected Behavior:
    Graphs properly reflect component changes made to entities outside of Landscape Canvas.

    Test Steps:
        1. Open Level
        2. Find LandscapeCanvas named entity
        3. Ensure Vegetation Distribution Component is present on the BushSpawner entity
        4. Open graph and ensure Distribution Filter wrapped node is present
        5. Delete the Vegetation Distribution Filter component from the BushSpawner entity via Entity Inspector
        6. Ensure the Vegetation Distribution Filter component was deleted from the BushSpawner entity and node is
        no longer present in the graph
        7. Add Vegetation Altitude Filter to the BushSpawner entity through Entity Inspector
        8. Ensure Altitude Filter was added to the BushSpawner node in the open graph
        9. Add a new entity with unique name as a child of the Landscape Canvas entity
        10. Add a Box Shape component to the new child entity
        11. Ensure Box Shape node is present on the open graph

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.asset as asset
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.entity as entity
    import azlmbr.legacy.general as general
    import azlmbr.landscapecanvas as landscapecanvas
    import azlmbr.math as math
    import azlmbr.slice as slice

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Open a simple level and instantiate LC_BushFlowerBlender.slice
    helper.init_idle()
    helper.open_level("Physics", "Base")
    transform = math.Transform_CreateIdentity()
    position = math.Vector3(64.0, 64.0, 32.0)
    transform.invoke('SetPosition', position)
    test_slice_path = os.path.join("Slices", "LC_BushFlowerBlender.slice")
    test_slice_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", test_slice_path, math.Uuid(),
                                                 False)
    test_slice = slice.SliceRequestBus(bus.Broadcast, 'InstantiateSliceFromAssetId', test_slice_id, transform)
    Report.critical_result(Tests.slice_instantiated, test_slice.IsValid())

    # Find root entity in the loaded level
    search_filter = entity.SearchFilter()
    search_filter.names = ["LandscapeCanvas"]

    # Allow a few seconds for matching entity to be found
    helper.wait_for_condition(lambda: len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)) > 0, 5.0)
    lc_matching_entities = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
    slice_root_id = lc_matching_entities[0]         #Entity with Landscape Canvas component
    Report.critical_result(Tests.lc_entity_found, slice_root_id.IsValid())

    # Find the BushSpawner entity
    search_filter.names = ["BushSpawner"]
    spawner_matching_entities = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
    spawner_id = spawner_matching_entities[0]       #Entity with Vegetation Layer Spawner component
    Report.critical_result(Tests.spawner_entity_found, spawner_id.IsValid())

    # Get needed component type ids
    distribution_filter_type_id = hydra.get_component_type_id("Vegetation Distribution Filter")
    altitude_filter_type_id = hydra.get_component_type_id("Vegetation Altitude Filter")
    box_shape_type_id = hydra.get_component_type_id("Box Shape")

    # Verify the BushSpawner entity has a Distribution Filter
    has_distribution_filter = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', spawner_id,
                                                           distribution_filter_type_id)
    Report.critical_result(Tests.dist_filter_component_found, has_distribution_filter)

    # Open Landscape Canvas and the existing graph
    general.open_pane('Landscape Canvas')
    open_graph = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'OnGraphEntity', slice_root_id)
    Report.critical_result(Tests.existing_graph_opened, open_graph.IsValid())

    # Verify that Distribution Filter node is present on the graph
    spawner_distribution_filter_component = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', spawner_id,
                                                                         distribution_filter_type_id)
    spawner_distribution_filter_component_id = spawner_distribution_filter_component.GetValue()
    distribution_filter_node = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast,
                                                                         'GetNodeMatchingEntityComponentInGraph',
                                                                         open_graph,
                                                                         spawner_distribution_filter_component_id)
    Report.critical_result(Tests.dist_filter_node_found, distribution_filter_node is not None)

    # Add a Vegetation Altitude Filter component to the BushSpawner entity, and verify the node is added to the graph
    spawner_altitude_filter_component = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', spawner_id,
                                                                     altitude_filter_type_id)
    spawner_altitude_filter_component_id = spawner_altitude_filter_component.GetValue()
    editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentOfType', spawner_id, altitude_filter_type_id)
    has_altitude_filter = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', spawner_id,
                                                       altitude_filter_type_id)
    altitude_filter_node = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'GetNodeMatchingEntityComponentInGraph',
                                                                     open_graph, spawner_altitude_filter_component_id)
    Report.result(Tests.dist_filter_component_found, has_altitude_filter)
    Report.result(Tests.dist_filter_node_found, altitude_filter_node is not None)

    # Remove the Distribution Filter
    editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [spawner_distribution_filter_component_id])
    general.idle_wait(1.0)

    # Verify the Distribution Filter was successfully removed from entity and the node was likewise removed
    has_distribution_filter = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', spawner_id,
                                                           distribution_filter_type_id)
    Report.result(Tests.dist_filter_component_removed, not has_distribution_filter)

    distribution_filter_node = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast,
                                                                         'GetAllNodesMatchingEntityComponent',
                                                                         spawner_distribution_filter_component_id)
    Report.result(Tests.dist_filter_node_removed, not distribution_filter_node)

    # Add a new child entity of BushSpawner entity
    box_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', spawner_id)
    Report.result(Tests.child_entity_added, editor.EditorEntityInfoRequestBus(bus.Event, 'GetParent', box_id) ==
                  spawner_id)

    # Add a Box Shape component to the new entity and verify it was properly added
    editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentOfType', box_id, box_shape_type_id)
    has_box_shape = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', box_id,
                                                 box_shape_type_id)
    Report.result(Tests.box_shape_component_found, has_box_shape)

    # Verify the Box Shape node appears on the graph
    box_shape_component = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', box_id,
                                                       box_shape_type_id)

    box_shape_component_id = box_shape_component.GetValue()
    box_shape_node = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'GetAllNodesMatchingEntityComponent',
                                                               box_shape_component_id)
    Report.result(Tests.box_shape_node_found, box_shape_node is not None)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(ComponentUpdates_UpdateGraph)
