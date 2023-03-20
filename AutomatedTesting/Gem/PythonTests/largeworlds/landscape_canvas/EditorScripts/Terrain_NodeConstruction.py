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
    level_components_added = (
        "Level components added correctly",
        "Failed to create level components"
    )
    aabb_configured = (
        "Successfully configured terrain bounds",
        "Failed to configure terrain bounds"
    )
    terrain_found = (
        "Found terrain at expected locations",
        "Failed to find terrain at expected locations"
    )


created_entity_id = None


def Terrain_NodeConstruction():
    """
    Summary:
    This test verifies that a Terrain Layer Spawner area can be constructed via Terrain nodes in Landscape Canvas.

    Expected Behavior:
    Terrain nodes/connections can construct a valid Terrain area.

    Test Steps:
     1) Open a simple level
     2) Open Landscape Canvas and create a new graph
     3) Add required Terrain Level components.
     4) Add a Terrain Layer Spawner and configure its Axis Aligned Box Shape.
     5) Add a new Constant Gradient node and configure.
     6) Add Terrain Height Gradient List/Terrain Surface Gradient List wrapped nodes to the Spawner node.
     7) Connect the Gradient List wrapped nodes to the configured Constant Gradient node.
     8) Validate terrain exists as constructed via nodes.
    """

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.editor.graph as graph
    import azlmbr.landscapecanvas as landscapecanvas
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.terrain as terrain

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.wait_utils import PrefabWaiter
    from editor_python_test_tools.editor_entity_utils import EditorEntity, EditorLevelEntity

    editor_id = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID

    def on_entity_created(parameters):
        global created_entity_id
        created_entity_id = parameters[0]

    # Open an existing simple level
    hydra.open_base_level()

    # Listen for entity creation notifications
    handler = editor.EditorEntityContextNotificationBusHandler()
    handler.connect()
    handler.add_callback("OnEditorEntityCreated", on_entity_created)

    # Open Landscape Canvas tool and verify
    general.open_pane("Landscape Canvas")
    Report.critical_result(Tests.lc_tool_opened, general.is_pane_visible("Landscape Canvas"))

    # Create a new graph in Landscape Canvas
    new_graph_id = graph.AssetEditorRequestBus(bus.Event, "CreateNewGraph", editor_id)
    Report.critical_result(Tests.new_graph_created, new_graph_id is not None)

    # Reposition the automatically created entity to origin
    root_entity = EditorEntity(created_entity_id)
    root_entity.set_world_translation(math.Vector3(0.0, 0.0, 0.0))

    # Make sure the graph we created is in Landscape Canvas
    graph_registered = graph.AssetEditorRequestBus(bus.Event, "ContainsGraph", editor_id, new_graph_id)
    Report.result(Tests.graph_registered, graph_registered)

    # Add the required Terrain level components
    terrain_world_component = EditorLevelEntity.add_component("Terrain World")
    terrain_world_renderer = EditorLevelEntity.add_component("Terrain World Renderer")
    Report.critical_result(Tests.level_components_added,
                           terrain_world_component is not None and terrain_world_renderer is not None)

    # Add a Terrain Layer Spawner node to the graph
    new_graph = graph.GraphManagerRequestBus(bus.Broadcast, "GetGraph", new_graph_id)
    node_position = math.Vector2(10.0, 10.0)
    layer_spawner_node = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, "CreateNodeForTypeName",
                                                                              new_graph, "TerrainLayerSpawnerNode")
    graph.GraphControllerRequestBus(bus.Event, "AddNode", new_graph_id, layer_spawner_node, node_position)

    # Get component type Ids for all expected Terrain components
    component_names = ["Terrain Layer Spawner", "Axis Aligned Box Shape", "Terrain Height Gradient List",
                       "Terrain Surface Gradient List", "Constant Gradient"]
    component_type_ids = hydra.get_component_type_id_map(component_names)

    # Get Axis Aligned Box Shape component info
    aabb_type_id = component_type_ids["Axis Aligned Box Shape"]
    aabb_component_outcome = editor.EditorComponentAPIBus(bus.Broadcast, "GetComponentOfType", created_entity_id,
                                                          aabb_type_id)
    assert aabb_component_outcome.IsSuccess(), "Failed to get Axis Aligned Box Shape component info"
    aabb_component = aabb_component_outcome.GetValue()

    # Adjust the bounds of the Axis Aligned Box Shape
    box_shape_dimensions = math.Vector3(512.0, 512.0, 64.0)
    aabb_outcome = editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", aabb_component,
                                                "Axis Aligned Box Shape|Box Configuration|Dimensions",
                                                box_shape_dimensions)
    Report.result(Tests.aabb_configured, aabb_outcome.IsSuccess())
    PrefabWaiter.wait_for_propagation()

    # Add a Constant Gradient node to the graph
    node_position = math.Vector2(110.0, 110.0)
    constant_gradient_node = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast,
                                                                                  "CreateNodeForTypeName", new_graph,
                                                                                  "ConstantGradientNode")
    graph.GraphControllerRequestBus(bus.Event, "AddNode", new_graph_id, constant_gradient_node, node_position)

    # Add Terrain Height Gradient List and Terrain Surface Gradient List wrapped nodes to the Terrain Spawner
    height_grad_list_node = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, "CreateNodeForTypeName",
                                                                                 new_graph,
                                                                                 "TerrainHeightGradientListNode")
    surface_grad_list_node = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, "CreateNodeForTypeName",
                                                                                  new_graph,
                                                                                  "TerrainSurfaceGradientListNode")
    graph.GraphControllerRequestBus(bus.Event, "WrapNode", new_graph_id, layer_spawner_node, height_grad_list_node)
    graph.GraphControllerRequestBus(bus.Event, "WrapNode", new_graph_id, layer_spawner_node, surface_grad_list_node)

    # Connect the Constant Gradient node to the Height and Surface Gradient Lists
    inbound_grad_slot = graph.GraphModelSlotId("InboundGradient")
    outbound_grad_slot = graph.GraphModelSlotId("OutboundGradient")

    graph.GraphControllerRequestBus(bus.Event, "AddConnectionBySlotId", new_graph_id, constant_gradient_node,
                                    outbound_grad_slot, height_grad_list_node, inbound_grad_slot)
    graph.GraphControllerRequestBus(bus.Event, "AddConnectionBySlotId", new_graph_id, constant_gradient_node,
                                    outbound_grad_slot, surface_grad_list_node, inbound_grad_slot)

    # Query 2 points we expect terrain to exist and one we don"t and validate
    terrain_exists = not terrain.TerrainDataRequestBus(bus.Broadcast, "GetIsHole", math.Vector3(128.0, 128.0, 0.0), 2)
    terrain_exists2 = not terrain.TerrainDataRequestBus(bus.Broadcast, "GetIsHole", math.Vector3(-64.0, -64.0, 0.0), 2)
    terrain_exists3 = not terrain.TerrainDataRequestBus(bus.Broadcast, "GetIsHole", math.Vector3(512.0, 512.0, 0.0), 2)

    Report.result(Tests.terrain_found, terrain_exists and terrain_exists2 and not terrain_exists3)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(Terrain_NodeConstruction)
