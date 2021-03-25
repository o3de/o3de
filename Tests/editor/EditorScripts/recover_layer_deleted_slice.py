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
C6130896 : Recover a layer containing a deleted slice.
https://testrail.agscollab.com/index.php?/cases/view/6130896
"""

import os
import sys

import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.layers as layers
import azlmbr.slice as slice
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestRecoverLayerDeletedSlice(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="recover_layer_deleted_slice: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Recover a layer containing a deleted slice.

        Expected Behavior:
        The layer is not recovered and a error is given:
        This layer contains a reference to a slice that can't be loaded. This layer can't be recovered.

        Test Steps:
         1) Open level
         2) Create a layer on the level as Layer1
         3) Create an entity as Entity1
         4) Make Entity1 a slice named Slice1
         5) Make Slice1 a child of Layer1
         6) Save the level
         7) Delete Layer1 from the level
         8) On disk find Slice1 and delete it
         9) Find Layer1 and select the "Recover layer" option for Layer1 in the Asset Browser


        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        SLICE_NAME = "Slice1.slice"

        def get_entity_count_name(entity_name):
            searchFilter = entity.SearchFilter()
            searchFilter.names = [entity_name]
            entities = entity.SearchBus(bus.Broadcast, "SearchEntities", searchFilter)
            return len(entities)

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        # 2) Create a layer on the level as Layer1
        layer_id = layers.EditorLayerComponent_CreateLayerEntityFromName("Layer1")
        if get_entity_count_name("Layer1*"):
            print("Layer1 is created")

        # 3) Create an entity as Entity1
        entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", entity.EntityId())
        editor.EditorEntityAPIBus(bus.Event, "SetName", entity_id, "Entity1")
        if get_entity_count_name("Entity1*"):
            print("Entity1 is created")

        # 4) Make Entity1 a slice named Slice1
        success = slice.SliceRequestBus(bus.Broadcast, "CreateNewSlice", entity_id, SLICE_NAME)
        print(f"Slice Created: {success}")

        # 5) Make Slice1 a child of Layer1
        editor.EditorEntityAPIBus(bus.Event, "SetParent", entity_id, layer_id)
        actual_parent_id = editor.EditorEntityInfoRequestBus(bus.Event, "GetParent", entity_id)
        if actual_parent_id.ToString() == layer_id.ToString():
            print("Parent of Entity1 is : Layer1")

        # 6) Save the level
        general.save_level()

        # 7) Delete Layer1 from the level
        general.idle_wait(1.0)
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntities ', [layer_id])

        # 8) On disk find Slice1 and delete it
        os.remove(os.path.join("SamplesProject", "Slice1.slice"))

        # 9) Find Layer1 and select the "Recover layer" option for Layer1 in the Asset Browser
        editor_window = pyside_utils.get_editor_main_window()
        general.open_pane("Asset Browser")
        asset_browser = editor_window.findChildren(QtWidgets.QDockWidget, "Asset Browser")[0]
        search_bar = asset_browser.findChildren(QtWidgets.QLineEdit, "textSearch")[0]
        search_bar.setText("Layer1.layer")
        asset_browser_tree = asset_browser.findChildren(QtWidgets.QTreeView, "m_assetBrowserTreeViewWidget")[0]
        asset_browser_tree.expandAll()
        model_index = pyside_utils.find_child_by_pattern(asset_browser_tree, "Layer1.layer")
        asset_browser_tree.setCurrentIndex(model_index)
        pyside_utils.trigger_context_menu_entry(asset_browser_tree, "Recover layer", index=model_index)

test = TestRecoverLayerDeletedSlice()
test.run()
