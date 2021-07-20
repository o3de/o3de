"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

import azlmbr.asset as asset
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.legacy.general as general
import azlmbr.editor.graph as graph
import azlmbr.landscapecanvas as landscapecanvas
import azlmbr.math as math
import azlmbr.slice as slice
import azlmbr.paths

import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import automatedtesting_shared.landscape_canvas_utils as lc


class TestGraphUpdatesUpdateComponents(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="GraphUpdatesUpdateComponents", args=["level"])

    def run_test(self):
        """
        Summary:
        This test verifies that components are properly updated as nodes are added/removed/updated.

        Expected Behavior:
        Landscape Canvas node CRUD properly updates component entities.

        Test Steps:
            1. Open Level.
            2. Open the graph on LC_BushFlowerBlender.slice
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
        # Create a new empty level and instantiate LC_BushFlowerBlender.slice
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=128,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=128,
            use_terrain=False,
        )
        transform = math.Transform_CreateIdentity()
        position = math.Vector3(64.0, 64.0, 32.0)
        transform.invoke('SetPosition', position)
        test_slice_path = os.path.join("Slices", "LC_BushFlowerBlender.slice")
        test_slice_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", test_slice_path, math.Uuid(),
                                                     False)
        test_slice = slice.SliceRequestBus(bus.Broadcast, 'InstantiateSliceFromAssetId', test_slice_id, transform)
        self.test_success = self.test_success and test_slice.IsValid()
        if test_slice.IsValid():
            self.log("Slice spawned!")

        # Search for root entity to ensure slice is loaded
        search_filter = entity.SearchFilter()
        search_filter.names = ["LandscapeCanvas"]
        self.wait_for_condition(lambda: len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)) > 0, 5.0)

        # Find needed entities in the loaded level
        slice_root_id = hydra.find_entity_by_name('LandscapeCanvas')
        bush_spawner_id = hydra.find_entity_by_name('BushSpawner')
        flower_spawner_id = hydra.find_entity_by_name('FlowerSpawner')
        inverted_perlin_noise_id = hydra.find_entity_by_name('Invert')

        # Open Landscape Canvas and the existing graph
        general.open_pane('Landscape Canvas')
        open_graph_id = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'OnGraphEntity', slice_root_id)
        self.test_success = self.test_success and open_graph_id.IsValid()
        if open_graph_id.IsValid():
            self.log('Graph opened')

        # Find the Rotation Modifier node on the BushSpawner entity
        rotation_modifier_node = lc.find_nodes_matching_entity_component('Vegetation Rotation Modifier', bush_spawner_id)

        # Remove the Rotation Modifier node
        graph.GraphControllerRequestBus(bus.Event, 'RemoveNode', open_graph_id, rotation_modifier_node[0])

        # Verify node was removed
        rotation_modifier_node = lc.find_nodes_matching_entity_component('Vegetation Rotation Modifier', bush_spawner_id)
        self.test_success = self.test_success and not rotation_modifier_node
        if not rotation_modifier_node:
            self.log('Rotation Modifier node was removed')
        else:
            self.log('Rotation Modifier node was not removed')

        # Verify the component was removed from the BushSpawner entity
        has_rotation_modifier = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', bush_spawner_id,
                                                             hydra.get_component_type_id('Vegetation Rotation Modifier'))
        self.test_success = self.test_success and not has_rotation_modifier
        if not has_rotation_modifier:
            self.log('Rotation Modifier component was removed from entity')
        else:
            self.log('Rotation Modifier component is still present on entity')

        # Find the Vegetation Layer Spawner node on the BushSpawner entity
        layer_spawner_node = lc.find_nodes_matching_entity_component('Vegetation Layer Spawner', bush_spawner_id)

        # Remove the Vegetation Layer Spawner node and verify the corresponding entity is deleted
        graph.GraphControllerRequestBus(bus.Event, 'RemoveNode', open_graph_id, layer_spawner_node[0])
        bush_spawner_id = hydra.find_entity_by_name('BushSpawner')
        self.test_success = self.test_success and not bush_spawner_id
        if not bush_spawner_id:
            self.log('BushSpawner entity was deleted')
        else:
            self.log('Failed to delete BushSpawner entity')

        # Connect the FlowerSpawner's Rotation Modifier node to the Invert Gradient Modifier node
        rotation_modifier_node = lc.find_nodes_matching_entity_component('Vegetation Rotation Modifier', flower_spawner_id)
        invert_node = lc.find_nodes_matching_entity_component('Invert Gradient Modifier', inverted_perlin_noise_id)

        inbound_gradient_z_slot = graph.GraphModelSlotId('InboundGradientZ')
        outbound_gradient_slot = graph.GraphModelSlotId('OutboundGradient')
        graph.GraphControllerRequestBus(bus.Event, 'AddConnectionBySlotId', open_graph_id, invert_node[0],
                                        outbound_gradient_slot, rotation_modifier_node[0], inbound_gradient_z_slot)

        general.idle_wait(1.0)      # Add a small wait to ensure component property has time to update

        # Verify the Gradient Entity Id reference on the Rotation Modifier component was properly set
        rotation_type_id = hydra.get_component_type_id('Vegetation Rotation Modifier')
        rotation_outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', flower_spawner_id,
                                                        rotation_type_id)
        rotation_component = rotation_outcome.GetValue()
        gradient_reference = hydra.get_component_property_value(rotation_component,
                                                                'Configuration|Rotation Z|Gradient|Gradient Entity Id')
        gradient_reference_success = gradient_reference == inverted_perlin_noise_id
        self.test_success = self.test_success and gradient_reference_success
        if gradient_reference_success:
            self.log('Gradient Entity Id reference was properly updated')
        else:
            self.log(f'Gradient Entity Id was not updated properly: Expected {inverted_perlin_noise_id.ToString()} -- Got '
                  f'{gradient_reference.ToString()}.')


test = TestGraphUpdatesUpdateComponents()
test.run()
