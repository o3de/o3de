"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from PySide2 import QtWidgets
import azlmbr.legacy.general as general

import editor_python_test_tools.pyside_utils as pyside_utils
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report


class Tests:
    action_found = "New Script event action found"
    asset_editor_opened = "Asset Editor opened"
    new_asset = "Asset Editor created with new asset"
    script_event = "New Script event created in Asset Editor"


GENERAL_WAIT = 0.5  # seconds


class TestAssetEditor_NewScriptEvent:
    """
    Summary:
     Verifying logic flow of the "+" button on the Script Canvas pane's Node Palette is as expected

    Expected Behavior:
     Clicking the "+" button and selecting "New Script Event" opens the Asset Editor with a new Script Event asset

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Close any existing AssetEditor window
     3) Get the SC window object
     4) Click on New Script Event on Node palette
     5) Verify if Asset Editor opened
     6) Verify if a new asset with Script Canvas category is opened
     7) Close Script Canvas and Asset Editor


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    @pyside_utils.wrap_async
    async def run_test(self):
        # 1) Open Script Canvas window (Tools > Script Canvas)
        general.idle_enable(True)
        general.open_pane("Script Canvas")
        helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)

        # 2) Close any existing AssetEditor window
        general.close_pane("Asset Editor")
        helper.wait_for_condition(lambda: not general.is_pane_visible("Asset Editor"), 5.0)

        # 3) Get the SC window object
        editor_window = pyside_utils.get_editor_main_window()
        sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
        node_palette = sc.findChild(QtWidgets.QDockWidget, "NodePalette")
        frame = node_palette.findChild(QtWidgets.QFrame, "searchCustomization")
        button = frame.findChild(QtWidgets.QToolButton)
        pyside_utils.click_button_async(button)

        # 4) Click on New Script Event on Node palette
        menu = None

        def menu_has_focus():
            nonlocal menu
            for fw in [
                QtWidgets.QApplication.activePopupWidget(),
                QtWidgets.QApplication.activeModalWidget(),
                QtWidgets.QApplication.focusWidget(),
                QtWidgets.QApplication.activeWindow(),
            ]:
                print(fw)
                if fw and isinstance(fw, QtWidgets.QMenu) and fw.isVisible():
                    menu = fw
                    return True
            return False

        await pyside_utils.wait_for_condition(menu_has_focus, GENERAL_WAIT)
        action = await pyside_utils.wait_for_action_in_menu(menu, {"text": "New Script Event"})
        Report.info(f"{Tests.action_found}: {action is not None}")
        action.trigger()
        pyside_utils.queue_hide_event(menu)

        # 5) Verify if Asset Editor opened
        result = helper.wait_for_condition(lambda: general.is_pane_visible("Asset Editor"), GENERAL_WAIT)
        Report.info(f"{Tests.asset_editor_opened}: {result}")

        # 6) Verify if a new asset with Script Canvas category is opened
        asset_editor = editor_window.findChild(QtWidgets.QDockWidget, "Asset Editor")
        row_container = asset_editor.findChild(QtWidgets.QWidget, "ContainerForRows")
        # NOTE: QWidget ContainerForRows will have frames of Name, Category, ToolTip etc.
        # To validate if a new script event file is generated, we check for
        # QFrame Category and its value
        categories = row_container.findChildren(QtWidgets.QFrame, "Category")
        Report.info(f"{Tests.new_asset}: {len(categories)>0}")
        result = False
        for frame in categories:
            line_edit = frame.findChild(QtWidgets.QLineEdit)
            result = True if (line_edit and line_edit.text() == "Script Events") else False
        Report.info(f"{Tests.script_event}: {result}")

        # 7) Close Script Canvas and Asset Editor
        general.close_pane("Script Canvas")
        general.close_pane("Asset Editor")


test = TestAssetEditor_NewScriptEvent()
test.run_test()
