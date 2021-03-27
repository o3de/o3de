"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


C17219006: Create Camera Entity from view
https://testrail.agscollab.com/index.php?/cases/view/17219006
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
import Tests.ly_shared.hydra_editor_utils as hydra
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class CreateCameraComponentFromViewTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="create_camera_comp_from_view", args=["level"])

    def run_test(self):
        """
        Summary:
        Create Camera Entity from view - This can be verified by checking if the entity is created with
        Camera component and if the Camera is listed in Viewport Camera Selector

        Expected Behavior:
        An entity is created with a Camera Component attached from the current viewport perspective.
        The camera entity is added to the Viewport Camera Selector.
        The Camera entity is labeled "Camera #" where # is replaced by the #number of camera entities in the level.

        Test Steps:
        1) Open level
        2) Trigger the action 'Create camera entity from view' from viewport context menu
        3) Verify if the entity is created with the name Camera# (Ex: Camera1)
        4) Verify if the entity has Camera component added
        5) Verify if Camera1 is listed in the Viewport Camera Selector
        6) Close Viewport Camera Selector

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
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # 2) Trigger the action 'Create camera entity from view' from viewport context menu
        # Get the editor window, main window, viewport objects
        editor_window = pyside_utils.get_editor_main_window()
        main_window = editor_window.findChild(QtWidgets.QMainWindow)
        view_port = pyside_utils.find_child_by_hierarchy(main_window, ..., "renderOverlay")
        view_port = main_window.findChildren(QtWidgets.QWidget, "renderOverlay")[0]

        # Trigger the action
        pyside_utils.trigger_context_menu_entry(view_port, "Create camera entity from view")

        # 3) Verify if the entity is created with the name Camera# (Ex: Camera1)
        camera_entity_id = general.find_editor_entity("Camera1")
        assert camera_entity_id.isValid(), "Entity with Camera component is not created"

        # 4) Verify if the entity has Camera component added
        assert hydra.has_components(camera_entity_id, ["Camera"]), "Entity do not have a Camera component"

        # 5) Verify if Camera1 is listed in the Viewport Camera Selector
        # Open Viewport Camera Selector if not opened already
        if not general.is_pane_visible("Viewport Camera Selector"):
            general.open_pane("Viewport Camera Selector")
        if general.is_pane_visible("Viewport Camera Selector"):
            print("Viewport Camera Selector is opened")

        # Get the Camera Selector object and the list of cameras
        camera_selector = editor_window.findChildren(QtWidgets.QDockWidget, "Viewport Camera Selector")[0]
        list_view = camera_selector.findChildren(QtWidgets.QListView)[0]

        # Verify if the Camera1 is present in the list
        assert pyside_utils.find_child_by_pattern(
            list_view, "Camera1"
        ), "Camera1 is not listed in the Viewport Camera Selector"

        # 6) Close Viewport Camera Selector
        camera_selector.close()
        if not general.is_pane_visible("Viewport Camera Selector"):
            print("Viewport Camera Selector is closed")


test = CreateCameraComponentFromViewTest()
test.run()
