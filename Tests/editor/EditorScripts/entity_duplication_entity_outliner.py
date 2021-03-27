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
C17218882: Entity duplication in Entity Outliner
https://testrail.agscollab.com/index.php?/cases/view/17218882
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.entity as entity
import azlmbr.bus as bus
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets, QtTest
from PySide2.QtCore import Qt, QObject, QEvent, QPoint
from PySide2.QtGui import QContextMenuEvent
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper
from re import compile as regex


class TestEntityDuplication(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="entity_duplication_entity_outliner: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Verify if the entities in the Entity Outliner can be duplicated.

        Expected Behavior:
        Entities can be duplicated and can be undone.

        Test Steps:
        1) Open a new level
        2) Create a new entity by clicking "New Entity"
        3) Duplicate entity using shortcut CTRL+D
        4) Undo entity duplication using CTRL+Z
        5) Duplicate entity using Right click "Duplicate" Action

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def get_entity_count_name(entity_name="Entity2"):
            searchFilter = entity.SearchFilter()
            searchFilter.names = [entity_name]
            entities = entity.SearchBus(bus.Broadcast, "SearchEntities", searchFilter)
            return len(entities)

        # 1) Open a new level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        # 2) Create a new entity by clicking "New Entity"
        general.clear_selection()
        editor_window = pyside_utils.get_editor_main_window()

        outliner_widget = pyside_utils.find_child_by_hierarchy(editor_window, 
            ...,
            dict(windowTitle=regex("Entity Outliner.*")),
            ...,
            "m_objectList").parent()
        tree = pyside_utils.find_child_by_hierarchy(outliner_widget, ..., "m_objectTree")

        action = pyside_utils.find_child_by_pattern(outliner_widget, "Create Entity")
        action.trigger()
        if get_entity_count_name() == 1:
            print("First entity created")

        # 3) Duplicate entity using shortcut CTRL+D
        for index in range(3):
            QtTest.QTest.keyPress(outliner_widget, Qt.Key_D, Qt.ControlModifier)
            success = lambda: get_entity_count_name() == index+2
            self.wait_for_condition(success)
            print(f"Entity duplicated using shortcut - {index+1} success: {success()}")

        # 4) Undo entity duplication using CTRL+Z
        QtTest.QTest.keyPress(outliner_widget, Qt.Key_Z, Qt.ControlModifier)
        success = lambda: get_entity_count_name() == 3
        self.wait_for_condition(success)
        print(f"Entity duplication Undo success: {success()}")

        # 5) Duplicate entity using Right click "Duplicate" Action
        general.select_object("Entity2")
        for index in range(3):
            index_to_duplicate = pyside_utils.find_child_by_pattern(tree, "Entity2")
            pyside_utils.trigger_context_menu_entry(tree, "Duplicate", index=index_to_duplicate)
            success = lambda: get_entity_count_name() == index+4
            self.wait_for_condition(success)
            print(f"Entity duplication using right click - {index+1} success: {success()}")


test = TestEntityDuplication()
test.run()
