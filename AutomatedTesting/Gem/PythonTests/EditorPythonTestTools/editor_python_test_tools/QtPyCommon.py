"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Base class for QtPy classes. Contains commonly used constants and generic behavior used on multiple QtPy classes
"""

import pyside_utils
from PySide2 import QtWidgets, QtTest, QtCore


class QtPyCommon:
    """
    """
    def __init__(self):
        self.var = None

    def click_menu_bar_option(self, window: QtWidgets, option_text: str) -> None:
        """
        function for clicking a toolbar menu option from a Qt window object. This function bypasses menu categories.
        for example, if you want to click the Open option from the "File" toolbar menu provide "Open" as your menu text
        instead of "File" then "Open".

        param window: the qt window object where the menu option is located
        param option_text: the label string used in the menu option that you want to click

        returns None
        """
        action = pyside_utils.find_child_by_pattern(window, {"text": option_text, "type": QtWidgets.QAction})

        assert action is not None, "Unable to find QtWidgets type or menu option"

        action.trigger()

    def expand_qt_container_rows(self, object_name: str) -> None:
        """
        Function used for expanding qt container rows with expandable children. May need to refactor this to replace
        self.findChildren with container.findChildren

        param object_name: qt object name as a string

        returns: None
        """
        children = self.findChildren(QtWidgets.QFrame, object_name)

        assert children is not None, "Unable to find QtContainer children."

        for child in children:
            check_box = child.findChild(QtWidgets.QCheckBox)
            if check_box and not check_box.isChecked():
                QtTest.QTest.mouseClick(check_box, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier)