"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C17219007: Camera Component
https://testrail.agscollab.com/index.php?/cases/view/17219007
"""

import os
import sys

import azlmbr.legacy.general as general
import azlmbr.math as math
import Tests.ly_shared.pyside_utils as pyside_utils
import Tests.ly_shared.hydra_editor_utils as editor_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestCameraComponentHelper(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="camera_component: ", args=["level"])

    def run_test(self):
        """
        Summary:
            Ensure that the camera component behaves properly in the editor and the
            proper camera settings are being populated.

        Expected Behavior:
            Camera fov is visible in the Viewport
            The Camera entity is populated in the Viewport Selector
            The Viewport perspective is changed to the camera
            The Viewport perspective is switched back to the default editor camera

        Test Steps:
            1) Create or open any level
            2) Create an entity with a Camera component
            3) Click "Be this camera"
            4) Save the position of the floating camera viewport and compare against entity
            5) Click "Return to default editor camera"
            6) Save the position of the current viewport and compare against entity

        Note:
            - This test file must be called from the Lumberyard Editor command terminal
            - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def get_button(button_text):
            return pyside_utils.find_child_by_pattern(entity_inspector, button_text)

        # 1) Create or open any level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        editor_window = pyside_utils.get_editor_main_window()
        entity_inspector = editor_window.findChildren(QtWidgets.QDockWidget, "Entity Inspector")[0]

        # 2) Create an entity with a Camera component
        entity_postition = math.Vector3(200.0, 200.0, 32.0)
        components_to_add = ["Camera"]

        test_entity = editor_utils.Entity("Temp_Entity")
        test_entity.create_entity(entity_postition, components_to_add)
        if test_entity.id.isValid():
            print("Entity created with camera component")

        # 3) Click "Be this camera"
        button_text = "Be this camera"
        self.wait_for_condition(lambda: get_button(button_text) is not None, 2.0)
        button = get_button(button_text)
        if button: 
            button.click()
            print("Button 'Be this camera' clicked")

        # 4) Save the position of the floating camera viewport and compare against entity
        self.wait_for_condition(lambda: general.get_current_view_position() == entity_postition, 1.0)
        camera_position = general.get_current_view_position()
        assert camera_position == entity_postition, "Camera positions did not change upon clicking 'Be this camera'"
        print("Camera has shifted after clicking 'Be this camera' button")

        # 5) Click "Return to default editor camera"
        button_text = "Return to default editor camera"
        self.wait_for_condition(lambda: get_button(button_text) is not None, 2.0)
        button = get_button(button_text)
        if button: 
            button.click()
            print("Button 'Return to default editor camera' clicked")

        # 6) Save the position of the current viewport and compare against entity
        self.wait_for_condition(lambda: general.get_current_view_position() != entity_postition, 1.0)
        camera_position = general.get_current_view_position()
        assert camera_position != entity_postition, "Camera positions did not change upon clicking 'Return to default editor camera'"
        print("Camera has shifted after clicking 'Return to default editor camera' button")


test = TestCameraComponentHelper()
test.run()
