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
import azlmbr.landscapecanvas as landscapecanvas
import azlmbr.math as math
import azlmbr.slice as slice
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper


class TestComponentUpdatesUpdateGraph(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="ComponentUpdatesUpdateGraph", args=["level"])

    def run_test(self):
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

        # Find root entity in the loaded level
        search_filter = entity.SearchFilter()
        search_filter.names = ["LandscapeCanvas"]

        # Allow a few seconds for matching entity to be found
        self.wait_for_condition(lambda: len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)) > 0, 5.0)
        lc_matching_entities = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
        slice_root_id = lc_matching_entities[0]         #Entity with Landscape Canvas component
        self.test_success = self.test_success and slice_root_id.IsValid()
        if slice_root_id.IsValid():
            self.log("LandscapeCanvas entity found")

        # Find the BushSpawner entity
        search_filter.names = ["BushSpawner"]
        spawner_matching_entities = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
        spawner_id = spawner_matching_entities[0]       #Entity with Vegetation Layer Spawner component
        self.test_success = self.test_success and spawner_id.IsValid()
        if spawner_id.IsValid():
            self.log("BushSpawner entity found")

        # Get needed component type ids
        distribution_filter_type_id = hydra.get_component_type_id("Vegetation Distribution Filter")
        altitude_filter_type_id = hydra.get_component_type_id("Vegetation Altitude Filter")
        box_shape_type_id = hydra.get_component_type_id("Box Shape")

        # Verify the BushSpawner entity has a Distribution Filter
        has_distribution_filter = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', spawner_id,
                                                               distribution_filter_type_id)
        self.test_success = self.test_success and has_distribution_filter
        if has_distribution_filter:
            self.log("Vegetation Distribution Filter on BushSpawner entity found")

        # Open Landscape Canvas and the existing graph
        general.open_pane('Landscape Canvas')
        open_graph = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'OnGraphEntity', slice_root_id)
        self.test_success = self.test_success and open_graph.IsValid()
        if open_graph.IsValid():
            self.log("Graph opened")

        # Verify that Distribution Filter node is present on the graph
        spawner_distribution_filter_component = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', spawner_id,
                                                                             distribution_filter_type_id)
        spawner_distribution_filter_component_id = spawner_distribution_filter_component.GetValue()
        distribution_filter_node = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast,
                                                                             'GetNodeMatchingEntityComponentInGraph',
                                                                             open_graph,
                                                                             spawner_distribution_filter_component_id)
        self.test_success = self.test_success and distribution_filter_node is not None
        if distribution_filter_node is not None:
            self.log("Distribution Filter node found on graph")
        else:
            self.log("Distribution Filter node not found on graph")

        # Add a Vegetation Altitude Filter component to the BushSpawner entity, and verify the node is added to the graph
        spawner_altitude_filter_component = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', spawner_id,
                                                                         altitude_filter_type_id)
        spawner_altitude_filter_component_id = spawner_altitude_filter_component.GetValue()
        editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentOfType', spawner_id, altitude_filter_type_id)
        has_altitude_filter = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', spawner_id,
                                                           altitude_filter_type_id)
        altitude_filter_node = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'GetNodeMatchingEntityComponentInGraph',
                                                                         open_graph, spawner_altitude_filter_component_id)
        self.test_success = self.test_success and has_altitude_filter and altitude_filter_node is not None
        if has_altitude_filter:
            self.log("Vegetation Altitude Filter on BushSpawner entity found")

        if altitude_filter_node is not None:
            self.log("Altitude Filter node found on graph")
        else:
            self.log("Altitude Filter node not found on graph")

        # Remove the Distribution Filter
        editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [spawner_distribution_filter_component_id])
        general.idle_wait(1.0)

        # Verify the Distribution Filter was successfully removed from entity and the node was likewise removed
        has_distribution_filter = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', spawner_id,
                                                               distribution_filter_type_id)
        self.test_success = self.test_success and not has_distribution_filter
        if not has_distribution_filter:
            self.log("Vegetation Distribution Filter removed from BushSpawner entity")

        distribution_filter_node = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast,
                                                                             'GetAllNodesMatchingEntityComponent',
                                                                             spawner_distribution_filter_component_id)
        self.test_success = self.test_success and not distribution_filter_node
        if distribution_filter_node:
            self.log("Distribution Filter node is still present on the graph")
        else:
            self.log("Distribution Filter node was removed from the graph")

        # Add a new child entity of BushSpawner entity
        box_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', spawner_id)
        if editor.EditorEntityInfoRequestBus(bus.Event, 'GetParent', box_id) == spawner_id:
            self.log("New entity successfully added as a child of the BushSpawner entity")
        else:
            self.log("New entity added with an unexpected parent")

        # Add a Box Shape component to the new entity and verify it was properly added
        editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentOfType', box_id, box_shape_type_id)
        has_box_shape = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', box_id,
                                                     box_shape_type_id)
        self.test_success = self.test_success and has_box_shape
        if has_box_shape:
            self.log("Box Shape on Box entity found")

        # Verify the Box Shape node appear on the graph
        box_shape_component = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', box_id,
                                                           box_shape_type_id)

        box_shape_component_id = box_shape_component.GetValue()
        box_shape_node = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'GetAllNodesMatchingEntityComponent',
                                                                   box_shape_component_id)
        self.test_success = self.test_success and box_shape_node is not None
        if box_shape_node is not None:
            self.log("Box Shape node found on graph")
        else:
            self.log("Box Shape node not found on graph")


test = TestComponentUpdatesUpdateGraph()
test.run()
