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
C6317058: Viewport Entity Creation
https://testrail.agscollab.com/index.php?/cases/view/6317058
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.editor as editor
import azlmbr.components as components
import azlmbr.bus as bus
import azlmbr.math as math
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets, QtTest
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper

class ViewportEntityCreation(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="viewport_entity_creation: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Verify if creating new entities via the viewport context menu works correctly.

        Expected Behavior:
        Right clicking the viewport and selecting the option in the context menu creates an entity under the cursor.

        Test Steps:
        1) Open a new level
        2) Move the cursor on the viewport
        3) Copy the position on the level pointed at by the cursor from the viewport's toolbar
        4) Create a new entity via the context menu
        5) Verify the entity has been created at the right position

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
        
        # Grab the necessary widgets
        editor_window = pyside_utils.get_editor_main_window()
        main_window = editor_window.findChild(QtWidgets.QMainWindow)
        viewport_window = main_window.findChild(QtWidgets.QMainWindow)
        overlay = viewport_window.findChild(QtWidgets.QWidget, "renderOverlay")
        posCtrl = viewport_window.findChild(QtWidgets.QWidget, "m_posCtrl")
        labels = ["X", "Y", "Z"]
        pos = [0.0, 0.0, 0.0]

        # 2) Click on the Viewport to focus
        QtTest.QTest.mouseClick(overlay, Qt.LeftButton)

        # Trigger a Move to update position in viewport toolbar
        QtTest.QTest.mouseMove(overlay)

        # 3) Get position values
        for i, label in enumerate(labels):
            vector_element = posCtrl.findChild(QtWidgets.QWidget, label)
            line_edit = vector_element.findChild(QtWidgets.QLineEdit)
            pos[i] = float(line_edit.text())

        # 4) Trigger Context Menu and click Create Entity
        pyside_utils.trigger_context_menu_entry(overlay.parent(), "Create entity")

        # Get Selected Entity (it will be the newly created one)
        selectedTestEntityIds = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetSelectedEntities')
        newEntityId = selectedTestEntityIds[0]

        # Get Position of new Entity
        position = components.TransformBus(bus.Event, "GetWorldTranslation", newEntityId)

        # 5) Verify the positions match (tolerance threshold is 1.0 since this is based on mouseClick
        # right-click position which has some room for error)
        POSITION_TOLERANCE = 1.0
        if (math.Math_IsClose(pos[0], position.x, POSITION_TOLERANCE) and
            math.Math_IsClose(pos[1], position.y, POSITION_TOLERANCE) and 
            math.Math_IsClose(pos[2], position.z, POSITION_TOLERANCE)):
            print("New Entity created in the correct position")


test = ViewportEntityCreation()
test.run()
