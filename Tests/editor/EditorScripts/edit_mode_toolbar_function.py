"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C1564083 : Edit Mode Toolbar
https://testrail.agscollab.com/index.php?/cases/view/1564083
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as EntityId
import Tests.ly_shared.pyside_utils as pyside_utils
import Tests.ly_shared.hydra_editor_utils as editor_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestEditModeToolbar(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="edit_mode_toolbar_function: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Interact with Edit mode toolbar options and verify if all the options are functional.

        Expected Behavior:
        All options available in the Edit Mode Toolbar function as intended.

        Test Steps:
         1) Create/Open a level
         2) Open the Edit mode Toolbar
         3) Use every option in the toolbar.


        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        editor_window = pyside_utils.get_editor_main_window()
        editors_toolbar = editor_window.findChild(QtWidgets.QToolBar, "EditMode")

        def get_entity_count(entity_id):
            searchFilter = EntityId.SearchFilter()
            searchFilter.names = [editor.EditorEntityInfoRequestBus(bus.Event, "GetName", entity_id)]
            entities = EntityId.SearchBus(bus.Broadcast, "SearchEntities", searchFilter)
            return len(entities)

        def get_tool_button(option):
            button = pyside_utils.find_child_by_pattern(editors_toolbar, option)
            if button.text() == option:
                return button

        def wait_condition_print(function, wait_time, print_message=""):
            self.wait_for_condition(function, wait_time)
            if function() and print_message != "":
                print(print_message)
        
        # 1) Create new level with an Entity
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        # 2) Open the Editors Toolbar
        if not editors_toolbar.isVisible():
            editors_toolbar.toggleViewAction().trigger()
        if editors_toolbar.isVisible():
            print("Editors tool bar opened successfully")

        # 3) Use every option in the toolbar.
        # i) Undo
        entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", EntityId.EntityId())
        self.wait_for_condition(lambda: get_tool_button("Undo").isEnabled(), 2.0)
        initial_count = get_entity_count(entity_id)
        get_tool_button("Undo").click()
        wait_condition_print(lambda: get_entity_count(entity_id) != initial_count, 2.0, "Undo tool button is functional")

        # ii) Redo
        self.wait_for_condition(lambda: get_tool_button("Redo").isEnabled(), 2.0)
        get_tool_button("Redo").click()
        wait_condition_print(lambda: get_entity_count(entity_id) == initial_count, 2.0, "Redo tool button is functional")

        # iii) Move
        move_button = get_tool_button("Move")
        if not move_button.isChecked(): move_button.click()
        if move_button.isChecked(): print("Move tool button is accesible")
            
        # iv) Rotate
        rotate_button = get_tool_button("Rotate")
        if not rotate_button.isChecked(): rotate_button.click()
        if rotate_button.isChecked(): print("Rotate tool button is accesible")

        # v) Scale
        scale_button = get_tool_button("Scale")
        if not scale_button.isChecked(): scale_button.click()
        if scale_button.isChecked(): print("Scale tool button is accesible")

        # vi) Play Game
        get_tool_button("Play Game").click()
        wait_condition_print(lambda : general.is_in_game_mode(), 3.0, "Play game tool button is responsive")
        general.exit_game_mode()

        # vii) Snap to grid
        # Wait for the snap_to_grid_text to become enabled since in the previous step we are leaving game mode,
        # so it might still be disabled momentarily
        snap_to_grid = get_tool_button("Snap To Grid")
        snap_grid_widget = snap_to_grid.parent()
        snap_to_grid.setChecked(True)
        snap_to_grid_text = snap_grid_widget.findChild(QtWidgets.QDoubleSpinBox).findChild(QtWidgets.QLineEdit)
        self.wait_for_condition(lambda: snap_to_grid_text.isEnabled())
        if snap_to_grid.isChecked() == snap_to_grid_text.isEnabled():
            print("Snap to grid tool button is responsive")

        # viii) Snap Angle
        # Wait for the snap_angle_text to become enabled since in the previous step we are leaving game mode,
        # so it might still be disabled momentarily
        snap_angle = get_tool_button("Snap angle")
        snap_angle_widget = snap_angle.parent()
        snap_angle.setChecked(True)
        snap_angle_text = snap_angle_widget.findChild(QtWidgets.QDoubleSpinBox).findChild(QtWidgets.QLineEdit)
        self.wait_for_condition(lambda: snap_angle_text.isEnabled())
        if snap_angle.isChecked() == snap_angle_text.isEnabled():
            print("Snap angle tool button is responsive")


test = TestEditModeToolbar()
test.run_test()
