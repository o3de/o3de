"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    prefab_instantiated = (
        "Test prefab instantiated properly",
        "Failed to instantiate test prefab"
    )
    lc_entity_found = (
        "LandscapeCanvas entity found",
        "Failed to find LandscapeCanvas entity"
    )
    spawner_entity_found = (
        "TerrainSpawner entity found",
        "Failed to find TerrainSpawner entity"
    )
    heightmap_entity_found = (
        "HeightmapGradient entity found",
        "Failed to find HeightmapGradient entity"
    )
    existing_graph_opened = (
        "Successfully opened graph from existing spawner setup",
        "Failed to open graph from existing spawner setup"
    )


def Terrain_ExistingSetups_GraphSuccessfully():
    """
    Summary:
    This test verifies that a Terrain Spawner that is manually setup and saved to a prefab is graphed properly in
    Landscape Canvas once opened.

    Expected Behavior:
    All expected nodes are present, expected nodes are wrapped, and all expected node connections are present/accurate.

    Test Steps:
     1) Open a simple level.
     2) Instantiate an existing Terrain spawner prefab.
     3) Open the prefab's Landscape Canvas graph.
     4) Validate the contents of the graph.
    """

    import os

    import azlmbr.bus as bus
    import azlmbr.editor.graph as graph
    import azlmbr.landscapecanvas as landscapecanvas
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.prefab as prefab
    import azlmbr.entity as entity

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report

    # Open a simple level and instantiate LandscapeCanvasTerrain.prefab
    hydra.open_base_level()
    position = math.Vector3(256.0, 256.0, 16.0)
    test_prefab_path = os.path.join("Assets", "Prefabs", "LandscapeCanvasTerrain.prefab")
    prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, "InstantiatePrefab", test_prefab_path,
                                                  entity.EntityId(), position)
    Report.critical_result(Tests.prefab_instantiated, prefab_result.IsSuccess())

    # Find the necessary entities/components to open the LC graph and validate nodes are graphed properly
    prefab_root_entity = EditorEntity.find_editor_entity("LandscapeCanvas", must_be_unique=True)
    Report.critical_result(Tests.lc_entity_found, prefab_root_entity.id.IsValid())
    terrain_spawner_entity = EditorEntity.find_editor_entity("TerrainSpawner", must_be_unique=True)
    Report.critical_result(Tests.spawner_entity_found, terrain_spawner_entity.id.IsValid())
    heightmap_entity = EditorEntity.find_editor_entity("HeightmapGradient", must_be_unique=True)
    Report.critical_result(Tests.heightmap_entity_found, heightmap_entity.id.IsValid())

    terrain_spawner_component = terrain_spawner_entity.get_components_of_type(["Terrain Layer Spawner"])[0]
    aabb_component = terrain_spawner_entity.get_components_of_type(["Axis Aligned Box Shape"])[0]
    shape_ref_component = heightmap_entity.get_components_of_type(["Shape Reference"])[0]
    gradient_component = heightmap_entity.get_components_of_type(["Perlin Noise Gradient"])[0]
    surf_grad_list_component = terrain_spawner_entity.get_components_of_type(["Terrain Surface Gradient List"])[0]
    height_grad_list_component = terrain_spawner_entity.get_components_of_type(["Terrain Height Gradient List"])[0]

    # Open the existing Landscape Canvas graph
    general.open_pane("Landscape Canvas")
    open_graph = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, "OnGraphEntity", prefab_root_entity.id)
    Report.critical_result(Tests.existing_graph_opened, open_graph.IsValid())

    # Validate the proper nodes are present
    spawner_nodes = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, "GetAllNodesMatchingEntityComponent",
                                                              terrain_spawner_component.id)
    aabb_nodes = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, "GetAllNodesMatchingEntityComponent",
                                                           aabb_component.id)
    shape_ref_nodes = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, "GetAllNodesMatchingEntityComponent",
                                                                shape_ref_component.id)
    gradient_nodes = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, "GetAllNodesMatchingEntityComponent",
                                                               gradient_component.id)
    surf_grad_list_nodes = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast,
                                                                     "GetAllNodesMatchingEntityComponent",
                                                                     surf_grad_list_component.id)
    height_grad_list_nodes = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast,
                                                                       "GetAllNodesMatchingEntityComponent",
                                                                       height_grad_list_component.id)
    nodes = {
        "ReferenceShapeNode": shape_ref_nodes,
        "AxisAlignedBoxShapeNode": aabb_nodes,
        "TerrainSurfaceGradientListNode": surf_grad_list_nodes,
        "TerrainHeightGradientListNode": height_grad_list_nodes,
        "TerrainLayerSpawnerNode": spawner_nodes,
        "PerlinNoiseGradientNode": gradient_nodes
    }
    for node_name in nodes.keys():
        node_found = (
            f"{node_name} found",
            f"Failed to find {node_name}"
        )
        Report.result(node_found, len(nodes[node_name]) == 1)

    # Get the necessary slot ids for validating node connections
    bounds_slot = graph.GraphModelSlotId("Bounds")
    inbound_shape_slot = graph.GraphModelSlotId("InboundShape")
    inbound_grad_slot = graph.GraphModelSlotId("InboundGradient")
    outbound_grad_slot = graph.GraphModelSlotId("OutboundGradient")

    # Validate the proper connections exist between nodes
    def are_slots_connected(node_1, node_1_slot, node_2, node_2_slot):
        connected = graph.GraphControllerRequestBus(bus.Event, "AreSlotsConnected", open_graph, nodes[node_1][0],
                                                    node_1_slot, nodes[node_2][0], node_2_slot)
        connection_found = (
            f"Found connection between {node_1} and {node_2}",
            f"Failed to find connection between {node_1} and {node_2}"
        )
        Report.result(connection_found, connected)

    are_slots_connected("PerlinNoiseGradientNode", outbound_grad_slot, "TerrainHeightGradientListNode",
                        inbound_grad_slot)
    are_slots_connected("PerlinNoiseGradientNode", outbound_grad_slot, "TerrainSurfaceGradientListNode",
                        inbound_grad_slot)
    are_slots_connected("AxisAlignedBoxShapeNode", bounds_slot, "ReferenceShapeNode", inbound_shape_slot)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(Terrain_ExistingSetups_GraphSuccessfully)
