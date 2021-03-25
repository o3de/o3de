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
C2654251: Adding and deleting entities to a slice should be included on save overrides by default
https://testrail.agscollab.com/index.php?/cases/view/2654251
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
from PySide2 import QtWidgets
from PySide2.QtCore import Qt
from re import compile as regex
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class SaveOverridesDefaultTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="save_overrides_default", args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Adding and deleting entities to a slice should be included on save overrides by default

        Expected Behavior:
        Observe that the Adding of Child 2 and the Deletion of Child 1 are both checked by default

        Test Steps:
        1) Open a new level
        2) Create 2 entities as parent and child
        3) Create slice from the parent entity
        4) Wait until slice is created
        5) Delete child on slice
        6) Add a new child to the slice
        7) Open Save Slice Overrides (Advanced) and verify if the changes are checked by default
        
        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        SLICE_NAME = "temp_slice.slice"
        self.focus_changed_flag = False

        def path_is_valid_asset(asset_path):
            asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", asset_path, math.Uuid(), False)
            return asset_id.invoke("IsValid")

        def on_focus_changed(old, new):
            active_modal_widget = QtWidgets.QApplication.activeModalWidget()
            if active_modal_widget and not self.focus_changed_flag:
                self.focus_changed_flag = True
                # verify the changes child1 (deleted) and child2 (added) are checked by default
                # if the Qt.CheckStateRole of the checkbox is 2, it means it is checked
                child1_deleted = pyside_utils.find_child_by_pattern(active_modal_widget, "child1 (deleted)").data(
                    Qt.CheckStateRole
                )
                print(f"Child 1 deleted change checked by default: {child1_deleted == 2}")
                child2_added = pyside_utils.find_child_by_pattern(active_modal_widget, "child2 (added)").data(
                    Qt.CheckStateRole
                )
                print(f"Child 2 added change checked by default: {child2_added == 2}")
                active_modal_widget.close()

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

        # 5) Delete child on slice
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", [child_id])

        # 6) Add a new child to the slice
        child_2_id = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", parent_id)
        editor.EditorEntityAPIBus(bus.Event, "SetName", child_2_id, "child2")

        # 7) Open Save Slice Overrides (Advanced) and verify if the changes are checked by default
        editor_window = pyside_utils.get_editor_main_window()
        outliner_widget = pyside_utils.find_child_by_hierarchy(
            editor_window, ..., dict(windowTitle=regex("Entity Outliner.*")), ..., "m_objectList"
        ).parent()
        tree = pyside_utils.find_child_by_hierarchy(outliner_widget, ..., "m_objectTree")
        self.wait_for_condition(lambda: pyside_utils.get_item_view_index(tree, 1) is not None)
        model_index = pyside_utils.get_item_view_index(tree, 0)
        app = QtWidgets.QApplication.instance()
        try:
            app.focusChanged.connect(on_focus_changed)
            await pyside_utils.trigger_context_menu_entry(tree, "Save Slice Overrides (Advanced)...", index=model_index)
        finally:
            app.focusChanged.disconnect(on_focus_changed)

test = SaveOverridesDefaultTest()
test.run()
