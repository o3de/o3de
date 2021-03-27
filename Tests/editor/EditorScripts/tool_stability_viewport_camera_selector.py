"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


C6308168: Tool Stability & Workflow: Viewport Camera Selector
https://testrail.agscollab.com/index.php?/cases/view/6308168
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtWidgets
import Tests.ly_shared.hydra_editor_utils as hydra
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class ToolStabilityViewportCameraSelectorTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="tool_stability_viewport_camera_selector", args=["level"])

    def run_test(self):
        """
        Summary:
        Tool Stability & Workflow: Viewport Camera Selector. This can be verified by creating mutiple entities
        with Camera component and verifying their functionality by clicking each of them in the Viewport Camera
        Selector tool

        Expected Behavior:
        The Viewport Camera Selector can be opened.
        Cameras added to a level will populate the Veiwport Camera Selector.
        Selecting cameras in the Viewport Camera Selector will change the perspective camera in the Viewport.
        The Viewport Camera Selector can be closed.


        Test Steps:
        1) Open level
        2) Create first Camera component entity
        3) Create second Camera component entity at different position
        4) Open Viewport Camera Selector
        5) Click on both the Cameras in Viewport Camera Selector and verify view position
        6) Close Viewport Camera Selector


        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        CAM_POSITION_1 = (512.0, 512.0, 34.0)
        CAM_POSITION_2 = (100.0, 100.0, 34.0)

        def create_camera_verify(entity_name, camera_position):
            general.set_current_view_position(*camera_position)
            # Trigger the action
            pyside_utils.trigger_context_menu_entry(view_port, "Create camera entity from view")
            # Verify if the entity is created with the name Camera# (Ex: Camera1)
            camera_entity_id = general.find_editor_entity(entity_name)
            assert camera_entity_id.isValid(), f"Entity {entity_name} with Camera component is not created"
            # Verify if the entity has Camera component added
            assert hydra.has_components(camera_entity_id, ["Camera"]), "Entity do not have a Camera component"

        def select_camera_verify_position(camera_index, expected_position):
            # Get the model index of the camera in the list view of Viewport Camera Selector
            model_index = pyside_utils.find_child_by_pattern(list_view, f"Camera{camera_index}")
            assert model_index, f"Camera{camera_index} not found in Camera View Selctor"
            # Click on the camera
            pyside_utils.item_view_index_mouse_click(list_view, model_index)
            self.wait_for_condition(
                lambda: general.get_current_view_position().x == expected_position[0]
                and general.get_current_view_position().y == expected_position[1]
                and general.get_current_view_position().z == expected_position[2],
                1.0,
            )
            current_position = general.get_current_view_position()
            assert (
                current_position.x == expected_position[0]
                and current_position.y == expected_position[1]
                and current_position.z == expected_position[2]
            ), f"View position incorrect for Camera {camera_index}"

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # Get the editor window, main window, viewport objects
        editor_window = pyside_utils.get_editor_main_window()
        main_window = editor_window.findChild(QtWidgets.QMainWindow)
        view_port = main_window.findChildren(QtWidgets.QWidget, "renderOverlay")[0]

        # 2) Create first Camera component entity
        create_camera_verify("Camera1", CAM_POSITION_1)
        print("done")
        # 3) Create second Camera component entity at different position
        create_camera_verify("Camera2", CAM_POSITION_2)

        # 4) Open Viewport Camera Selector
        if not general.is_pane_visible("Viewport Camera Selector"):
            general.open_pane("Viewport Camera Selector")
        # Get the Camera Selector object and the list of cameras
        camera_selector = editor_window.findChildren(QtWidgets.QDockWidget, "Viewport Camera Selector")[0]
        list_view = camera_selector.findChildren(QtWidgets.QListView)[0]

        # 5) Click on both the Cameras in Viewport Camera Selector and verify view position
        select_camera_verify_position(1, CAM_POSITION_1)
        select_camera_verify_position(2, CAM_POSITION_2)

        # 6) Close Viewport Camera Selector
        camera_selector.close()
        self.wait_for_condition(lambda: not general.is_pane_visible("Viewport Camera Selector"), 1.0)
        if not general.is_pane_visible("Viewport Camera Selector"):
            print("Viewport Camera Selector is closed")


test = ToolStabilityViewportCameraSelectorTest()
test.run()
