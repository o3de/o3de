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
C17123213: Quick Saving Changes to Slice
https://testrail.agscollab.com/index.php?/cases/view/17123213

"""
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.asset as asset
import azlmbr.entity as entity
import azlmbr.math as math
import azlmbr.slice as slice
from azlmbr.entity import EntityId

import Tests.ly_shared.pyside_utils as pyside_utils
import Tests.ly_shared.hydra_editor_utils as hydra
from re import compile as regex
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class QuickSaveSlice(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="quick_save_slice", args=["level"])

    def run_test(self):
        """
        Summary:
        Quick Saving Changes to Slice

        Expected:
        All changes made to the slice are saved

        Test Steps:
        1) Create or open a level
        2) Create a slice
        3) Create child entity
        4) Add a component to the slice.
        5) Quick save the slice
        6) Delete the existing slice before verifying if it is saved
        7) Reinstantiate the slice to verify if the changes are saved.
        8) Verify the saved changes

        Note:
            - This test file must be called from the Lumberyard Editor command terminal
            - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        SLICE_NAME = "TempSlice.slice"

        def search_entity(entity_name):
            searchFilter = entity.SearchFilter()
            searchFilter.names = [entity_name]
            entities = entity.SearchBus(bus.Broadcast, "SearchEntities", searchFilter)
            return entities

        # 1) Create or open any level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # 2) Create a slice
        # Create an Entity to turn into a slice.
        new_entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", EntityId())
        editor.EditorEntityAPIBus(bus.Event, "SetName", new_entity_id, "Entity2")
        success = slice.SliceRequestBus(bus.Broadcast, "CreateNewSlice", new_entity_id, SLICE_NAME)
        print(f"Slice Created: {success}")

        # 3) Create child entity
        child_entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", new_entity_id)
        editor.EditorEntityAPIBus(bus.Event, "SetName", child_entity_id, "Entity3")
        print(f"Child entity created: {child_entity_id.IsValid()}")

        # 4) Add a component to the slice.
        hydra.add_component("Mesh", new_entity_id)

        # 5) Quick save the slice
        editor_window = pyside_utils.get_editor_main_window()
        outliner_widget = pyside_utils.find_child_by_hierarchy(
            editor_window, ..., dict(windowTitle=regex("Entity Outliner.*")), ..., "m_objectList"
        ).parent()
        tree = pyside_utils.find_child_by_hierarchy(outliner_widget, ..., "m_objectTree")
        model_index = pyside_utils.find_child_by_pattern(tree, "Entity2")
        pyside_utils.item_view_index_mouse_click(tree, model_index)
        pyside_utils.trigger_context_menu_entry(tree, SLICE_NAME.lower())

        # 6) Delete the existing slice before verifying if it is saved
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntities', [new_entity_id, child_entity_id])

        # 7) Reinstantiate the slice to verify if the changes are saved.
        transform = math.Transform_CreateIdentity()
        position = math.Vector3(64.0, 64.0, 32.0)
        transform.SetPosition(position)
        asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", SLICE_NAME, math.Uuid(), False)
        slice.SliceRequestBus(bus.Broadcast, "InstantiateSliceFromAssetId", asset_id, transform)

        # 8) Verify the saved changes
        tree.expandAll()
        self.wait_for_condition(lambda: len(search_entity("Entity3")) > 0, 2.0)
        # Parent and child entities instantiated
        parent_id = search_entity("Entity2")[0]
        child_id = search_entity("Entity3")[0]
        # Parent child hierarchy
        parent_child_valid = parent_id.Equal(editor.EditorEntityInfoRequestBus(bus.Event, "GetParent", child_id))
        # Component in parent entity
        has_component = hydra.has_components(parent_id, ["Mesh"])
        changes_saved = parent_id.IsValid() and child_id.IsValid() and parent_child_valid and has_component
        print(f"Changes in the slice file are saved: {changes_saved}")


test = QuickSaveSlice()
test.run()
