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

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper


class TestUndoNodeDeleteSlice(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="UndoNodeDeleteSlice", args=["level"])

    def run_test(self):
        """
        Summary:
        This test verifies Editor stability after undoing the deletion of nodes on a slice entity.

        Expected Behavior:
        Editor remains stable and free of crashes.

        Test Steps:
         1) Create a new level
         2) Instantiate a slice with a Landscape Canvas setup
         3) Find a specific node on the graph, and delete it
         4) Restore the node with Undo

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
        self.wait_for_condition(lambda: len(entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)) > 0,
                                5.0)
        lc_matching_entities = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
        slice_root_id = lc_matching_entities[0]  # Entity with Landscape Canvas component
        self.test_success = self.test_success and slice_root_id.IsValid()
        if slice_root_id.IsValid():
            self.log("LandscapeCanvas entity found")

        # Find the BushSpawner entity
        search_filter.names = ["BushSpawner"]
        spawner_matching_entities = entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)
        spawner_id = spawner_matching_entities[0]  # Entity with Vegetation Layer Spawner component
        self.test_success = self.test_success and spawner_id.IsValid()
        if spawner_id.IsValid():
            self.log("BushSpawner entity found")
        
        # Open Landscape Canvas and the existing graph
        general.open_pane('Landscape Canvas')
        open_graph = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'OnGraphEntity', slice_root_id)
        self.test_success = self.test_success and open_graph.IsValid()
        if open_graph.IsValid():
            self.log("Graph opened")
        
        # Get needed component type ids
        layer_spawner_type_id = hydra.get_component_type_id("Vegetation Layer Spawner")
        
        # Find the Vegetation Layer Spawner node on the BushSpawner entity
        layer_spawner_component = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', spawner_id,
                                                               layer_spawner_type_id)
        layer_spawner_component_component_id = layer_spawner_component.GetValue()
        layer_spawner_node = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'GetAllNodesMatchingEntityComponent',
                                                                       layer_spawner_component_component_id)

        self.test_success = self.test_success and layer_spawner_node
        if layer_spawner_node:
            self.log("Vegetation Layer Spawner node found on graph")
        else:
            self.log("Vegetation Layer Spawner node not found")
        
        # Remove the Layer Spawner node
        graph.GraphControllerRequestBus(bus.Event, "RemoveNode", open_graph, layer_spawner_node[0])
        
        # Verify node was removed
        layer_spawner_node = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'GetAllNodesMatchingEntityComponent',
                                                                       layer_spawner_component_component_id)

        self.test_success = self.test_success and not layer_spawner_node
        if not layer_spawner_node:
            self.log("Vegetation Layer Spawner node was removed")
        else:
            self.log("Vegetation Layer Spawner node was not removed")
        
        # Undo the Node deletion. This is required to be executed twice to hit the node removal.
        general.undo()
        general.undo()
        
        # self.log a line to the Console to verify the Editor is still active
        self.log("Editor is still responsive")


test = TestUndoNodeDeleteSlice()
test.run()
