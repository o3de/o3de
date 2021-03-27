"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C1564610: LMB and RMB mouse functionality
https://testrail.agscollab.com/index.php?/cases/view/1564610
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.entity as entity
import azlmbr.bus as bus
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestLeftAndRightMouseButtons(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="left_and_right_mouse_buttons: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Use left mouse button to select items in Editor and
        Right mouse button to open the item.

        Expected Behavior:
        LMB interaction is correct and accurate.
        RMB functions normally and appropriate context menus are opened.

        Test Steps:
        1) Open a new level
        2) Test LMB and RMB on Viewport of editor to open context menus and select items.
            2.1) Create camera entity from view
            2.2) Create entity
            2.3) Create layer

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def get_entity_count_name(entity_name):
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

        # 2) Test RMB and LMB on Viewport of editor to open context menus and select items.
        # Note: pyside_utils.trigger_context_menu_entry(widget, pattern, pos=None) does same 
        # functionality as Right click to open context menu and left click to select options in menu.

        app = QtWidgets.QApplication.instance()
        editor_window = pyside_utils.get_editor_main_window()
        viewport = editor_window.findChildren(QtWidgets.QWidget, "renderOverlay")[0]

        # 2.1) Create camera entity from view
        pyside_utils.trigger_context_menu_entry(viewport, "Create camera entity from view")
        if get_entity_count_name("Camera1"):
            print("Create camera entity from view option is selected using mouse buttons")
        # 2.2) Create entity
        pyside_utils.trigger_context_menu_entry(viewport, "Create entity")
        if get_entity_count_name("Entity3"):
            print("Create entity option is selected using mouse buttons")
        # 2.3) Create layer
        pyside_utils.trigger_context_menu_entry(viewport, "Create layer")
        if get_entity_count_name("Layer4*"):
            print("Create layer option is selected using mouse buttons")


test = TestLeftAndRightMouseButtons()
test.run()
