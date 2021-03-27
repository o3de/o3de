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
C6308162: Tool Stability: Entity Inspector
https://testrail.agscollab.com/index.php?/cases/view/6308162
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as EntityId
import azlmbr.math as math
import azlmbr.asset as asset
from PySide2 import QtWidgets, QtTest, QtCore
from PySide2.QtCore import Qt
import Tests.ly_shared.hydra_editor_utils as hydra
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestToolStabilityEntityInspector(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="tool_stability_entity_inspector: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Tool Stability: Entity Inspector

        Test Steps:
        1) Open level
        2) Create an Entity with Mesh component
        3) View the entity in the Viewport and Take screenshot
        4) Click on the filter icon in Entity Outliner and check the entities filtered by "Mesh" in Entity Outliner

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def on_focus_changed(old, new):
            print("focus changed")
            popup = QtWidgets.QApplication.activePopupWidget()
            if popup:
                while tree.indexBelow(tree.currentIndex()) != QtCore.QModelIndex():
                    QtTest.QTest.keyClick(tree, Qt.Key_Down, Qt.NoModifier)
                    if tree.currentIndex().data(Qt.DisplayRole) == "Mesh":
                        break
                tree.model().setData(tree.currentIndex(), 2, Qt.CheckStateRole)
                outliner_tree = outliner_main_window.findChildren(QtWidgets.QTreeView, "m_objectTree")[0]
                # make sure only one entity with mesh component is filtered
                QtTest.QTest.keyClick(outliner_tree, Qt.Key_Down, Qt.NoModifier)
                if tree.currentIndex().data(Qt.DisplayRole) == "Mesh" and outliner_tree.indexBelow(outliner_tree.currentIndex()) == QtCore.QModelIndex():
                    print("Entity with Mesh component is filtered by Entity outliner")

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # 2) Create an Entity with Mesh component
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        entity_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
        )
        general.clear_selection()
        entity_name = editor.EditorEntityInfoRequestBus(bus.Event, "GetName", entity_id)
        general.select_object(entity_name)
        Entity = hydra.Entity("Entity", entity_id)
        Entity.components = []
        Entity.components.append(hydra.add_component("Mesh", entity_id))

        def get_asset_by_path(path):
            return asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", path, math.Uuid(), False)

        # Add any valid mesh asset
        hydra.get_set_test(
            Entity,
            0,
            "MeshComponentRenderNode|Mesh asset",
            get_asset_by_path(os.path.join("Objects", "SamplesAssets", "mover_display_smooth.cgf")),
        )

        # 3) View the entity in the Viewport and Take screenshot
        self.take_viewport_screenshot(125.00, 129.01, 34.90, 0.00, 0.00, 0.00)
        screenshot_path = os.path.join("Cache", "SamplesProject", "pc", "user", "screenshots", "screenshot0000.jpg")
        if os.path.isfile(screenshot_path):
            print("Screenshot Taken")

        # 4) Click on the filter icon in Entity Outliner and check the entities filtered by "Mesh" in Entity Outliner
        general.idle_wait(2.0)
        editor_window = pyside_utils.get_editor_main_window()
        main_window = editor_window.findChild(QtWidgets.QMainWindow)
        entity_outliner = pyside_utils.find_child_by_property(
            main_window, QtWidgets.QDockWidget, "objectName", "Entity Outliner (PREVIEW)"
        )
        outliner_main_window = entity_outliner.findChild(QtWidgets.QMainWindow)
        tool_button = (
            outliner_main_window.findChild(QtWidgets.QWidget)
            .findChild(QtWidgets.QWidget, "OutlinerWidgetUI")
            .findChild(QtWidgets.QFrame, "m_searchWidget")
            .findChild(QtWidgets.QFrame, "textSearchContainer")
            .findChild(QtWidgets.QToolButton, "assetTypeSelector")
        )
        try:
            menu = tool_button.findChild(QtWidgets.QMenu)
            tree = menu.findChild(QtWidgets.QTreeView)
            app = QtWidgets.QApplication.instance()
            line_edit = menu.findChild(QtWidgets.QLineEdit, "filteredSearchWidget")
            line_edit.setFocus()
            line_edit.setText("Mesh")
            app.focusChanged.connect(on_focus_changed)
            tree.setFocus()
            tool_button.click()
            general.idle_wait(1.0)

        finally:
            app.focusChanged.disconnect(on_focus_changed)


test = TestToolStabilityEntityInspector()
test.run_test()