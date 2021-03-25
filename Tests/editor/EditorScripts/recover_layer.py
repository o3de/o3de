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
C6130788: Recover a layer
https://testrail.agscollab.com/index.php?/cases/view/6130788
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.editor as editor
import azlmbr.bus as bus
import azlmbr.layers as layers

import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets, QtTest, QtCore
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestRecoverLayer(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="recover_layer: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Create two layers, delete them and ensure they can be recovered correctly.

        Expected Behavior:
        Layers can be recovered.

        Test Steps:
        1) Create two new new levels
        2) Create two layers, one with "Save as binary" activated.
        3) Save the level.
        4) Delete the layers.
        5) Save the level again.
        6) Load the other level.
        7) Reload the first level.
        8) Reload the layers using the Asset Browser.
        9) Ensure the layers have recovered.
        9) Ensure the save options are correct.
        10) Undo,  check the layers are removed
        11) Redo, check the layers are restored.

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def fail_test(msg):
            print(msg)
            print("Test failed.")
            sys.exit()

        def on_focus_changed_save_as(old, new):
            active_widget = QtWidgets.QApplication.activeModalWidget()
            save_level_as_dialogue = active_widget.findChild(QtWidgets.QWidget, "LevelFileDialog")
            if save_level_as_dialogue.windowTitle() == "Save Level As ":
                print("The 'Save Level As' dialog appeared")
            level_name = save_level_as_dialogue.findChild(QtWidgets.QLineEdit, "nameLineEdit")
            level_name.setText("test_level_2")
            button_box = save_level_as_dialogue.findChild(QtWidgets.QDialogButtonBox, "buttonBox")
            button_box.button(QtWidgets.QDialogButtonBox.Ok).click()

            general.idle_wait(3.0)  # wait for new level to load

        level_to_load = ""

        def get_inspector_window():
            nonlocal editor_window
            inspector_dock = editor_window.findChild(QtWidgets.QWidget, "Entity Inspector")
            entity_inspector = inspector_dock.findChild(QtWidgets.QMainWindow)

            child_widget = entity_inspector.findChild(QtWidgets.QWidget)
            if child_widget is None:
                fail_test("Failed to find inspector window")

            general.idle_wait(1.0)
            comp_list_contents = (
                child_widget.findChild(QtWidgets.QScrollArea, "m_componentList")
                    .findChild(QtWidgets.QWidget, "qt_scrollarea_viewport")
                    .findChild(QtWidgets.QWidget, "m_componentListContents")
            )

            if comp_list_contents is None or len(comp_list_contents.children()) < 1:
                return None

            return comp_list_contents.children()[1]

        def get_binary_save_state_for_layer(layer_name):
            layer_id = general.find_editor_entity(layer_name)
            if not layer_id.isValid():
                fail_test("Failed to find " + " after recovery")

            editor.ToolsApplicationRequestBus(bus.Broadcast, 'SetSelectedEntities', [layer_id])

            layer_inspector = get_inspector_window()
            if layer_inspector is None:
                fail_test("Failed to find inspector window")

            frame = layer_inspector.findChild(QtWidgets.QFrame, "Save as binary")
            check_box = frame.findChild(QtWidgets.QCheckBox)

            return check_box.isChecked()

        def set_binary_save_state_for_layer(layer_name, state):
            layer_id = general.find_editor_entity(layer_name)
            if not layer_id.isValid():
                fail_test("Failed to find " + " after recovery")

            editor.ToolsApplicationRequestBus(bus.Broadcast, 'SetSelectedEntities', [layer_id])

            layer_inspector = get_inspector_window()
            if layer_inspector is None:
                fail_test("Failed to find inspector window")

            frame = layer_inspector.findChild(QtWidgets.QFrame, "Save as binary")
            check_box = frame.findChild(QtWidgets.QCheckBox)

            check_box.setChecked(state)

        EditorTestHelper.after_level_load(self)
        editor_window = pyside_utils.get_editor_main_window()

        # Create new level.
        result = general.create_level_no_prompt("test_level_1", 1024, 1, 4096, True)
        if result != 0:
            fail_test("Failed to create level")

        general.idle_wait(1.0)

        # Create two layers
        layer1_id = layers.EditorLayerComponent_CreateLayerEntityFromName('Layer1')
        if layer1_id.isValid():
            print("Layer1 created.")
        else:
            fail_test("Layer1 creation failed.")

        layer2_id = layers.EditorLayerComponent_CreateLayerEntityFromName('Layer2')
        if layer2_id.isValid():
            print("Layer2 created.")
        else:
            fail_test("Layer2 creation failed.")

        # Set the binary save option on layer 1.
        set_binary_save_state_for_layer("Layer1", True)

        # Save as a different level name
        try:
            editor_window = pyside_utils.get_editor_main_window()
            app = QtWidgets.QApplication.instance()
            app.focusChanged.connect(on_focus_changed_save_as)
            action = pyside_utils.get_action_for_menu_path(editor_window, "File", "Save As")
            action.trigger()

        finally:
            app.focusChanged.disconnect(on_focus_changed_save_as)

        print("Saved level 2 with layers")

        # Delete the layers.
        general.idle_wait(1.0)
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntities', [layer1_id, layer2_id])

        # Save level 2 again.
        general.save_level()

        print("Saved level 2 without layers")

        # Reopen level 1.
        general.open_level_no_prompt("test_level_1")
        if editor.EditorToolsApplicationRequestBus(bus.Broadcast, "GetCurrentLevelName") != "test_level_1":
            fail_test("Failed to reload test_level_1")
        print("Reopened level 1")

        # Reopen level 2.
        general.open_level_no_prompt("test_level_2")
        if editor.EditorToolsApplicationRequestBus(bus.Broadcast, "GetCurrentLevelName") != "test_level_2":
            fail_test("Failed to reload test_level_2")
        print("Reopened level 2")

        # Recover the layers
        game_folder = editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'GetGameFolder')
        layer1_path = game_folder + "\\Levels\\test_level_2\\Layers\\Layer1.layer"
        layer2_path = game_folder + "\\Levels\\test_level_2\\Layers\\Layer2.layer"

        layers.EditorLayerComponent_RecoverLayer(layer1_path)
        layers.EditorLayerComponent_RecoverLayer(layer2_path)

        # Check the binary save states are correct.
        layer1_binary = get_binary_save_state_for_layer("Layer1")
        layer2_binary = get_binary_save_state_for_layer("Layer2")

        if not layer1_binary:
            fail_test("Layer1 save not set to binary.")
        if layer2_binary:
            fail_test("Layer2 save set to binary.")

        # Undo layer recovery.
        general.undo()
        general.undo()

        id1 = general.find_editor_entity("Layer1")
        id2 = general.find_editor_entity("Layer2")

        if id1.isValid() or id2.isValid():
            fail_test("Failed to undo layer recovery.")

        # Redo layer recovery.
        general.redo()
        general.redo()

        id1 = general.find_editor_entity("Layer1")
        id2 = general.find_editor_entity("Layer2")

        if not id1.isValid() or not id2.isValid():
            fail_test("Failed to redo layer recovery.")

        print("Recover Layer test complete.")


test = TestRecoverLayer()
test.run()

