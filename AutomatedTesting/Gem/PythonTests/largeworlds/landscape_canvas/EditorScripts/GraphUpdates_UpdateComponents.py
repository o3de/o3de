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
    existing_graph_opened = (
        "Opened existing graph from prefab",
        "Failed to open existing graph"
    )
    node_removed = (
        "Rotation Modifier node was removed",
        "Failed to remove Rotation Modifier node"
    )
    component_removed = (
        "Rotation Modifier component was removed from entity",
        "Rotation Modifier component is still present on entity"
    )
    entity_deleted = (
        "BushSpawner entity was deleted",
        "Failed to delete BushSpawner entity"
    )
    entity_reference_updated = (
        "Gradient Entity Id reference was properly updated",
        "Gradient Entity Id reference was not updated properly"
    )


def GraphUpdates_UpdateComponent():
    """
    Summary:
    This test verifies that components are properly updated as nodes are added/removed/updated.
    Expected Behavior:
    Landscape Canvas node CRUD properly updates component entities.
    Test Steps:
        1. Open Level.
        2. Open the graph on BushFlowerBlender.prefab
        3. Find the Rotation Modifier node on the BushSpawner entity
        4. Delete the Rotation Modifier node
        5. Ensure the Vegetation Rotation Modifier component is removed from the BushSpawner entity
        6. Delete the Vegetation Layer Spawner node from the graph
        7. Ensure BushSpawner entity is deleted
        8. Change connection from second Rotation Modifier node to a different Gradient
        9. Ensure Gradient reference on component is updated
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

    import automatedtesting_shared.landscape_canvas_utils as lc
    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Open a simple level and instantiate BushFlowerBlender.prefab
    hydra.open_base_level()
    transform = math.Transform_CreateIdentity()
    position = math.Vector3(64.0, 64.0, 32.0)
    transform.invoke('SetPosition', position)
    test_prefab_path = os.path.join("Assets", "Prefabs", "BushFlowerBlender.prefab")
    prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'InstantiatePrefab', test_prefab_path,
                                                  entity.EntityId(), position)
    Report.critical_result(Tests.prefab_instantiated, prefab_result.IsSuccess())

    # Search for root entity to ensure prefab is loaded and focused
    search_filter = entity.SearchFilter()
    search_filter.names = ["LandscapeCanvas"]
    helper.wait_for_condition(lambda: len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)) > 0, 5.0)
    prefab_lc_root = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)[0]
    azlmbr.prefab.PrefabFocusPublicRequestBus(bus.Broadcast, "FocusOnOwningPrefab", prefab_lc_root)

    # Find needed entities in the loaded level
    bush_spawner_id = hydra.find_entity_by_name('BushSpawner')
    flower_spawner_id = hydra.find_entity_by_name('FlowerSpawner')
    inverted_perlin_noise_id = hydra.find_entity_by_name('Invert')

    # Open Landscape Canvas and the existing graph
    general.open_pane('Landscape Canvas')
    open_graph_id = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'OnGraphEntity', prefab_lc_root)
    Report.critical_result(Tests.existing_graph_opened, open_graph_id.IsValid())

    # Find the Rotation Modifier node on the BushSpawner entity
    rotation_modifier_node = lc.find_nodes_matching_entity_component('Vegetation Rotation Modifier', bush_spawner_id)

    # Remove the Rotation Modifier node
    graph.GraphControllerRequestBus(bus.Event, 'RemoveNode', open_graph_id, rotation_modifier_node[0])
    general.idle_wait_frames(2)

    # Verify node was removed
    rotation_modifier_node = lc.find_nodes_matching_entity_component('Vegetation Rotation Modifier', bush_spawner_id)
    Report.result(Tests.node_removed, not rotation_modifier_node)

    # Verify the component was removed from the BushSpawner entity
    has_rotation_modifier = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', bush_spawner_id,
                                                         hydra.get_component_type_id('Vegetation Rotation Modifier'))
    Report.result(Tests.component_removed, not has_rotation_modifier)

    # Find the Vegetation Layer Spawner node on the BushSpawner entity
    layer_spawner_node = lc.find_nodes_matching_entity_component('Vegetation Layer Spawner', bush_spawner_id)

    # Remove the Vegetation Layer Spawner node and verify the corresponding entity is deleted
    graph.GraphControllerRequestBus(bus.Event, 'RemoveNode', open_graph_id, layer_spawner_node[0])
    general.idle_wait_frames(2)
    layer_spawner_node = lc.find_nodes_matching_entity_component('Vegetation Layer Spawner', bush_spawner_id)
    bush_spawner_id = hydra.find_entity_by_name('BushSpawner')
    Report.result(Tests.entity_deleted, not layer_spawner_node and not bush_spawner_id)

    # Connect the FlowerSpawner's Rotation Modifier node to the Invert Gradient Modifier node
    rotation_modifier_node = lc.find_nodes_matching_entity_component('Vegetation Rotation Modifier', flower_spawner_id)
    invert_node = lc.find_nodes_matching_entity_component('Invert Gradient Modifier', inverted_perlin_noise_id)

    inbound_gradient_z_slot = graph.GraphModelSlotId('InboundGradientZ')
    outbound_gradient_slot = graph.GraphModelSlotId('OutboundGradient')
    graph.GraphControllerRequestBus(bus.Event, 'AddConnectionBySlotId', open_graph_id, invert_node[0],
                                    outbound_gradient_slot, rotation_modifier_node[0], inbound_gradient_z_slot)

    general.idle_wait_frames(2)     # Add a small wait to ensure component property has time to update

    # Verify the Gradient Entity Id reference on the Rotation Modifier component was properly set
    rotation_type_id = hydra.get_component_type_id('Vegetation Rotation Modifier')
    rotation_outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', flower_spawner_id,
                                                    rotation_type_id)
    rotation_component = rotation_outcome.GetValue()
    gradient_reference = hydra.get_component_property_value(rotation_component,
                                                            'Configuration|Rotation Z|Gradient|Gradient Entity Id')
    Report.result(Tests.entity_reference_updated, gradient_reference == inverted_perlin_noise_id)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GraphUpdates_UpdateComponent)
