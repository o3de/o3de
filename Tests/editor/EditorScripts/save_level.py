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
C15167490: Save a level
https://testrail.agscollab.com/index.php?/cases/view/15167490
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.editor as editor
import azlmbr.entity as EntityId
import azlmbr.bus as bus
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestSaveALevel(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="save_level: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Create or open a level in editor. Create an entity and
        Save level with new name using File > Save As..

        Expected Behavior:
        The "Save Level As" dialog appears.
        A new level file is created and saved with the name provided.
        The level is saved.

        Test Steps:
        1) Create or open a level in the editor.
        2) Create an entity and Save Level with new name
        3) Make any changes to the level and click on save

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def on_focus_changed(old, new):
            active_widget = QtWidgets.QApplication.activeModalWidget()
            save_level_as_dialogue = active_widget.findChild(QtWidgets.QWidget, "LevelFileDialog")
            if save_level_as_dialogue.windowTitle() == "Save Level As ":
                print("The 'Save Level As' dialog appeared")
            level_name = save_level_as_dialogue.findChild(QtWidgets.QLineEdit, "nameLineEdit")
            level_name.setText("tmp_new_level")
            button_box = save_level_as_dialogue.findChild(QtWidgets.QDialogButtonBox, "buttonBox")
            button_box.button(QtWidgets.QDialogButtonBox.Ok).click()

            general.idle_wait(3.0) #wait for new level lo load

        def on_action_triggered(action):
            print(f"{action} Action triggered")

        # 1) Create or open a level in the editor.
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(4.0)

        # 2) Create an entity and Save Level with new name
        newEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId.EntityId())
        if newEntityId.IsValid():
            print("Entity created")
        try:
            editor_window = pyside_utils.get_editor_main_window()
            app = QtWidgets.QApplication.instance()
            app.focusChanged.connect(on_focus_changed)
            action = pyside_utils.get_action_for_menu_path(editor_window, "File", "Save As")
            action.triggered.connect(on_action_triggered("Save As"))
            action.trigger()
            action.triggered.disconnect(on_action_triggered("Save As"))

        finally:
            app.focusChanged.disconnect(on_focus_changed)
        
        # Verify if the new level is loaded
        general.idle_wait(2.0)
        if editor.EditorToolsApplicationRequestBus(bus.Broadcast, "GetCurrentLevelName") == "tmp_new_level":
            print("A new level file is created and saved with the name provided.: SUCCESS")
        else:
            print("A new level file is created and saved with the name provided.: FAILED")

        # 3) Make any changes to the level and click on save
        new_entity = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId.EntityId())
        if new_entity.IsValid():
            print("Made a change in level")
        editor_window = pyside_utils.get_editor_main_window()
        action = pyside_utils.get_action_for_menu_path(editor_window, "File", "Save")
        action.triggered.connect(on_action_triggered("Save"))
        action.trigger()
        action.triggered.disconnect(on_action_triggered("Save"))
    

test = TestSaveALevel()
test.run()