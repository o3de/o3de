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
C2669525: Adding and deleting entities to a slice are able to be quick pushed to slice
https://testrail.agscollab.com/index.php?/cases/view/2669525
"""

import os
import sys

import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.math as math
import azlmbr.asset as asset
import azlmbr.editor as editor
import azlmbr.slice as slice
import azlmbr.entity as EntityId
import Tests.ly_shared.pyside_utils as pyside_utils
from re import compile as regex
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class QuickPushSliceTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="quick_push_slice", args=["level"])

    def run_test(self):
        """
        Summary:
        Adding and deleting entities to a slice are able to be quick pushed to slice

        Expected Behavior:
        The slice is saved and the second instance of the slice is updated with the 
        deletion of Child 1 and a addition of Child 2.

        Test Steps:
        1) Open a new level
        2) Create 2 entities as parent and child
        3) Create slice from the parent entity
        4) Wait until slice is created
        5) Instantiate a newly created slice
        6) Delete child on original slice
        7) Add a child to the original slice entity
        8) Save slice overrides
        9) Verify if the instantiated slice has a new child

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        SLICE_NAME = "temp_slice.slice"

        def path_is_valid_asset(asset_path):
            asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", asset_path, math.Uuid(), False)
            return asset_id.invoke("IsValid")

        # 1) Open a new level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        # 2) Create 2 entities as parent and child
        parent_id = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", EntityId.EntityId())
        child_id = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", parent_id)
        editor.EditorEntityAPIBus(bus.Event, "SetName", child_id, "child1")

        # 3) Create slice from the parent entity
        success = slice.SliceRequestBus(bus.Broadcast, "CreateNewSlice", parent_id, SLICE_NAME)
        print(f"Slice Created: {success}")

        # 4) Wait until slice is created
        self.wait_for_condition(lambda: path_is_valid_asset(SLICE_NAME), 3.0)

        # 5) Instantiate a newly created slice
        transform = math.Transform_CreateIdentity()
        position = math.Vector3(64.0, 64.0, 32.0)
        transform.SetPosition(position)
        asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", SLICE_NAME, math.Uuid(), False)
        slice.SliceRequestBus(bus.Broadcast, "InstantiateSliceFromAssetId", asset_id, transform)

        # 6) Delete child on original slice
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", [child_id])

        # 7) Add a child to the original slice entity
        child_2_id = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", parent_id)
        editor.EditorEntityAPIBus(bus.Event, "SetName", child_2_id, "child2")

        # 8) Save slice overrides
        editor_window = pyside_utils.get_editor_main_window()
        outliner_widget = pyside_utils.find_child_by_hierarchy(
            editor_window, ..., dict(windowTitle=regex("Entity Outliner.*")), ..., "m_objectList"
        ).parent()
        tree = pyside_utils.find_child_by_hierarchy(outliner_widget, ..., "m_objectTree")
        # Model index for the original slice
        self.wait_for_condition(lambda: pyside_utils.get_item_view_index(tree, 2) is not None, 2.0)
        model_index_1 = pyside_utils.get_item_view_index(tree, 1)
        pyside_utils.trigger_context_menu_entry(tree, SLICE_NAME.lower(), index=model_index_1)

        # 9) Verify if the instantiated slice has a new child
        # model index for instantiated slice
        model_index_0 = pyside_utils.get_item_view_index(tree, 0)
        self.wait_for_condition(lambda: pyside_utils.find_child_by_pattern(model_index_0, "child1") is None, 2.0)
        if pyside_utils.find_child_by_pattern(model_index_0, "child2"):
            print("Newly created child is updated in the instantiated slice")
        if pyside_utils.find_child_by_pattern(model_index_0, "child1") is None:
            print("Child deletion is updated in the instantiated slice")


test = QuickPushSliceTest()
test.run()
