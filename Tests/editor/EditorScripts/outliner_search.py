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
C17218816: Entity Outliner Searching
https://testrail.agscollab.com/index.php?/cases/view/17218816
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.bus as bus
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets, QtTest
from PySide2.QtCore import Qt, QObject, QEvent, QPoint
from PySide2.QtGui import QContextMenuEvent, QCursor
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper

class TestEntityOutlinerSearching(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="outliner_search: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Verify the search function of the Entity Outliner works properly

        Expected Behavior:
        Typing text on the Entity Outliner search field limits the number of entities shown in the tree view

        Test Steps:
        1) Create new level
        2) Create some test entities
        3) Write in the Entity Outliner search field (different combinations) and validate

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )
        general.idle_wait(3.0)
        
        # Grab the Outliner
        
        app = QtWidgets.QApplication.instance()
        editor_window = pyside_utils.get_editor_main_window()
        main_window = editor_window.findChild(QtWidgets.QMainWindow)
        entity_outliner = main_window.findChild(QtWidgets.QDockWidget, "Entity Outliner (PREVIEW)")
        outliner_main_window = entity_outliner.findChild(QtWidgets.QMainWindow)
        tree = outliner_main_window.findChild(QtWidgets.QTreeView, "m_objectTree")
        search = outliner_main_window.findChild(QtWidgets.QLineEdit, "textSearch")

        # Store number of starting entities, just in case
        startingEntityCount = tree.model().rowCount()

        # 2) Create entities

        def CreateEntityWithName(name):
            entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())
            editor.EditorEntityAPIBus(bus.Event, 'SetName', entityId, name)

        CreateEntityWithName("Test_01")
        CreateEntityWithName("Test_02")
        CreateEntityWithName("Test_03")
        CreateEntityWithName("AnotherTest")
        CreateEntityWithName("Entity01")
        CreateEntityWithName("Entity02")
        
        # Count rows in Outliner
        fullEntityCount = tree.model().rowCount()

        if((fullEntityCount - startingEntityCount) == 6):
            print("Test Entities were set up correctly")
        
        # 3) Write in the Entity Outliner search field (different combinations)
        
        # Write "Test_0" in search box
        search.setText("Test_0")

        entityCount = tree.model().rowCount()
        if(entityCount == 3):
            print("Searching Test_0 returns 3 entities")

        # Write "Test" in search box
        search.setText("Test")

        entityCount = tree.model().rowCount()
        if(entityCount == 4):
            print("Searching Test returns 4 entities")

        # Write "asdfgh" in search box
        search.setText("asdfgh")

        entityCount = tree.model().rowCount()
        if(entityCount == 0):
            print("Searching asdfgh returns 0 entities")

        # Write "" in search box
        search.setText("")

        entityCount = tree.model().rowCount()
        if(entityCount == fullEntityCount):
            print("Emptying the search returns all entities")

test = TestEntityOutlinerSearching()
test.run()
