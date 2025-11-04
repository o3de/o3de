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
        "Opened existing graph from prefab",
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

    import azlmbr.bus as bus
    import azlmbr.entity as entity
    import azlmbr.legacy.general as general
    import azlmbr.landscapecanvas as landscapecanvas
    import azlmbr.math as math
    import azlmbr.prefab as prefab

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report

    # Open a simple level and instantiate BushFlowerBlender.prefab
    hydra.open_base_level()
    transform = math.Transform_CreateIdentity()
    position = math.Vector3(64.0, 64.0, 32.0)
    transform.invoke('SetPosition', position)
    test_prefab_path = os.path.join("Assets", "Prefabs", "BushFlowerBlender.prefab")
    prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, "InstantiatePrefab", test_prefab_path,
                                                  entity.EntityId(), position)
    Report.critical_result(Tests.prefab_instantiated, prefab_result.IsSuccess())

    # Find root entity in the loaded level
    prefab_root_entity = EditorEntity.find_editor_entity("LandscapeCanvas", must_be_unique=True)
    Report.critical_result(Tests.lc_entity_found, prefab_root_entity.id.IsValid())

    # Find the BushSpawner entity
    bush_spawner_entity = EditorEntity.find_editor_entity("BushSpawner", must_be_unique=True)
    Report.critical_result(Tests.spawner_entity_found, bush_spawner_entity.id.IsValid())

    # Verify the BushSpawner entity has a Distribution Filter
    has_distribution_filter = bush_spawner_entity.has_component("Vegetation Distribution Filter")
    Report.critical_result(Tests.dist_filter_component_found, has_distribution_filter)

    # Open Landscape Canvas and the existing graph
    general.open_pane("Landscape Canvas")
    open_graph = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, "OnGraphEntity", prefab_root_entity.id)
    Report.critical_result(Tests.existing_graph_opened, open_graph.IsValid())

    # Verify that Distribution Filter node is present on the graph
    spawner_dist_filter_component = bush_spawner_entity.get_components_of_type(["Vegetation Distribution Filter"])[0]
    spawner_dist_filter_nodes = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast,
                                                                          "GetAllNodesMatchingEntityComponent",
                                                                          spawner_dist_filter_component.id)
    Report.critical_result(Tests.dist_filter_node_found, len(spawner_dist_filter_nodes) > 0)

    # Add a Vegetation Altitude Filter component to the BushSpawner entity, and verify the node is added to the graph
    spawner_alt_filter_component = bush_spawner_entity.add_component("Vegetation Altitude Filter")
    has_altitude_filter = bush_spawner_entity.has_component("Vegetation Altitude Filter")
    altitude_filter_nodes = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast,
                                                                      "GetAllNodesMatchingEntityComponent",
                                                                      spawner_alt_filter_component.id)
    Report.result(Tests.alt_filter_component_found, has_altitude_filter)
    Report.result(Tests.alt_filter_node_found, len(altitude_filter_nodes) > 0)

    # Remove the Distribution Filter
    bush_spawner_entity.remove_component("Vegetation Distribution Filter")

    # Verify the Distribution Filter was successfully removed from entity and the node was likewise removed
    has_dist_filter = bush_spawner_entity.has_component("Vegetation Distribution Filter")
    Report.result(Tests.dist_filter_component_removed, not has_dist_filter)
    spawner_dist_filter_nodes = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast,
                                                                          "GetAllNodesMatchingEntityComponent",
                                                                          spawner_dist_filter_component.id)
    Report.result(Tests.dist_filter_node_removed, len(spawner_dist_filter_nodes) == 0)

    # Add a new child entity of BushSpawner entity
    box_entity = EditorEntity.create_editor_entity("Box Child", parent_id=bush_spawner_entity.id)
    Report.result(Tests.child_entity_added, box_entity.get_parent_id() == bush_spawner_entity.id)

    # Add a Box Shape component to the new entity and verify it was properly added
    box_shape_component = box_entity.add_component("Box Shape")
    has_box_shape = box_entity.has_component("Box Shape")
    Report.result(Tests.box_shape_component_found, has_box_shape)

    # Verify the Box Shape node appears on the graph
    box_shape_nodes = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, "GetAllNodesMatchingEntityComponent",
                                                                box_shape_component.id)
    Report.result(Tests.box_shape_node_found, len(box_shape_nodes) > 0)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(ComponentUpdates_UpdateGraph)
