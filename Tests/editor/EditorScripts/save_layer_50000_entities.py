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
C3712672: Save a Layer with 50,000 entities.
https://testrail.agscollab.com/index.php?/cases/view/3712672
"""

import os

import azlmbr.legacy.general as general
import azlmbr.entity as entity
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.layers as layers
import azlmbr.paths
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtTest, QtWidgets
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class SaveLayer50000EntitiesTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="save_layer_50000_entities", args=["level"])

    def run_test(self):
        """
        Summary:
        Save a Layer with 50,000 entities and verify if the editor is stable.

        Expected Behavior:
        The level and layer are saved.
        The Editor remains stable.

        Test Steps:
        1) Open a new level
        2) Delete the DefaultLevelSetup initially to make the selection of entities easier
        3) Create a new layer
        4) Initially create 2000 entities using 'Duplicate' action
        5) Duplicate the already created entities

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def search_entity(entity_name):
            searchFilter = entity.SearchFilter()
            searchFilter.names = [entity_name]
            entities = entity.SearchBus(bus.Broadcast, "SearchEntities", searchFilter)
            return len(entities)

        def layer_saved():
            return os.path.exists(os.path.join(azlmbr.paths.devroot, "SamplesProject", "Levels", self.args["level"], "Layers", "Layer.layer"))

        # 1) Open a new level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        editor_window = pyside_utils.get_editor_main_window()
        outliner_widget = editor_window.findChildren(QtWidgets.QDockWidget, "Entity Outliner (PREVIEW)")[0].widget()

        # 2) Delete the DefaultLevelSetup initially to make the selection of entities easier
        default_entity_id = general.find_editor_entity("DefaultLevelSetup")
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", [default_entity_id])

        # 3) Create a new layer
        layer_id = layers.EditorLayerComponent_CreateLayerEntityFromName("Layer")
        print(f"New layer created: {layer_id.isValid()}")
        editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", layer_id)
        general.select_object("Entity2")
        action = pyside_utils.get_action_for_menu_path(editor_window, "Edit", "Select All")

        # 4) Initially create 2000 entities using 'Duplicate' action
        for _ in range(2000):
            QtTest.QTest.keyPress(outliner_widget, Qt.Key_D, Qt.ControlModifier)

        # 5) Duplicate the already created entities
        action.trigger()
        for _ in range(25):
            QtTest.QTest.keyPress(outliner_widget, Qt.Key_D, Qt.ControlModifier)
        print(f"50000 entities created: {search_entity('Entity2')>=50000}")

        # 6) Save/Verify if layer is saved
        general.clear_selection()
        general.select_object("Layer")
        action = pyside_utils.get_action_for_menu_path(editor_window, "File", "Save")
        action.trigger()
        self.wait_for_condition(layer_saved, 250.0)
        print(f"Layer saved: {layer_saved()}")


test = SaveLayer50000EntitiesTest()
test.run()
