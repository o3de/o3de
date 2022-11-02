"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects used when testing and manipulating the O3DE UI
"""
import pyside_utils
from editor_python_test_tools.QtPyCommon import QtPyCommon
from editor_python_test_tools.QtPyScriptCanvasEditor import QtPyScriptCanvasEditor
from editor_python_test_tools.QtPyAssetEditor import QtPyAssetEditor
import azlmbr.legacy.general as general
from editor_python_test_tools.utils import TestHelper
from consts.asset_editor import (ASSET_EDITOR_UI)
from consts.scripting import (SCRIPT_CANVAS_UI)
from consts.general import (WAIT_TIME_SEC_3)


class Tests():
    script_canvas_editor_opened = ("Script Canvas Editor opened successfully", "Failed to open Script Canvas Editor")
    script_canvas_editor_closed = ("Script Canvas Editor closed successfully", "Failed to close Script Canvas Editor")



class QtPyO3DEEditor(QtPyCommon):
    """
    Container class for QtPy objects used in testing/manipulating the UI

    """

    def __init__(self):
        super().__init__()
        self.editor_main_window = pyside_utils.get_editor_main_window()
        self.sc_editor = None
        self.asset_editor = None

    def open_script_canvas(self):
        """
        Function for instantiating the core script canvas QtPy object and its associated other script canvas tools.

        returns reference to the QtPy script canvas editor object
        """
        general.open_pane(SCRIPT_CANVAS_UI)
        result = TestHelper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI), WAIT_TIME_SEC_3)

        assert result, "Failed to open Script Canvas Editor"

        self.sc_editor = QtPyScriptCanvasEditor(self.editor_main_window)

        return self.sc_editor

    def close_script_canvas(self) -> None:
        """
        This function will close the script canvas UI. It will not clear the reference to the sc_editor

        returns: None
        """

        general.close_pane(SCRIPT_CANVAS_UI)
        result = TestHelper.wait_for_condition(lambda: general.is_pane_visible(SCRIPT_CANVAS_UI) is False, WAIT_TIME_SEC_3)

        assert result, "Failed to close Script Canvas Editor"

    def open_asset_editor(self):
        """
        Function for opening the asset editor pane then instantiating the asset editor QtPy object

        returns a reference to the QtPy asset editor object
        """

        general.open_pane(ASSET_EDITOR_UI)

        self.asset_editor = QtPyAssetEditor(self.editor_main_window)
        TestHelper.wait_for_condition(lambda: self.asset_editor is not None, WAIT_TIME_SEC_3)

        assert self.asset_editor is not None, "Failed to instantiate QtPyAssetEditor."

        return self.asset_editor

    def close_asset_editor(self) -> None:
        """
        Function for closing the asset editor UI. Does not clear the reference to asset editor QtPy object

        returns: None
        """

        general.close_pane(ASSET_EDITOR_UI)

        result = TestHelper.wait_for_condition(lambda: general.is_pane_visible(ASSET_EDITOR_UI) is False,
                                               WAIT_TIME_SEC_3)

        assert result, "Failed to close Asset Editor"
