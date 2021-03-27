"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
C5436953: Loading a level with un-pushed slice changes does not effect editor stability
https://testrail.agscollab.com/index.php?/cases/view/5436953
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.entity as entity
import azlmbr.bus as bus
import azlmbr.math as math
import azlmbr.asset as asset
import azlmbr.editor as editor
import azlmbr.slice as slice
import Tests.ly_shared.hydra_editor_utils as hydra
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class LoadLevelUnpushedSliceTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="load_level_unpushed_slice_changes", args=["level"])

    def run_test(self):
        """
        Summary:
        Loading a level with un-pushed slice changes does not effect editor stability

        Expected Behavior:
        The editor remains stable, level loads, and the slice still has pushable changes.

        Test Steps:
        1) Open a new level
        2) Create an entity and add a component to it
        3) Create slice from the entity
        4) Delete component from the entity
        5) Save the level
        6) Reload the same level
        7) Verify changes after reload
        8) Instantiate slice and verify if it has a component

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        SLICE_NAME = "TestSlice.slice"

        def search_entity(entity_name):
            searchFilter = entity.SearchFilter()
            searchFilter.names = [entity_name]
            entities = entity.SearchBus(bus.Broadcast, "SearchEntities", searchFilter)
            return entities

        # 1) Open a new level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        # 2) Create an entity and add a component to it
        new_entity = hydra.Entity("entity")
        new_entity.create_entity(math.Vector3(64.0, 64.0, 32.0), ["Mesh"])

        # 3) Create slice from the entity
        success = slice.SliceRequestBus(bus.Broadcast, "CreateNewSlice", new_entity.id, SLICE_NAME)
        print(f"Slice Created: {success}")

        # 4) Delete component from the entity
        success = editor.EditorComponentAPIBus(bus.Broadcast, "RemoveComponents", [new_entity.components[0]])
        print(f"Component removed successfully: {success}")

        # 5) Save the level
        general.save_level()

        # 6) Reload the same level
        general.reload_current_level()

        # 7) Verify changes after reload
        # Entity should not have the component
        entity_id = search_entity("entity")[0]
        print(f"Entity found after level reload: {entity_id.IsValid()}")
        print(
            f"Level saved even if there are unpushed changes in slice: {not hydra.has_components(entity_id, ['Mesh'])}"
        )

        # 8) Instantiate slice and verify if it has a component
        # Delete existing slice initially
        general.delete_object("entity")
        # Instantiate slice
        transform = math.Transform_CreateIdentity()
        position = math.Vector3(64.0, 64.0, 32.0)
        transform.SetPosition(position)
        asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", SLICE_NAME, math.Uuid(), False)
        slice.SliceRequestBus(bus.Broadcast, "InstantiateSliceFromAssetId", asset_id, transform)
        self.wait_for_condition(lambda: len(search_entity("entity")) > 0, 2.0)
        entity_id = search_entity("entity")[0]
        print(f"Unpushed changes in Slice are not saved: {hydra.has_components(entity_id, ['Mesh'])}")


test = LoadLevelUnpushedSliceTest()
test.run()
