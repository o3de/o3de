"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Base class for QtPy classes. Contains commonly used constants and generic behavior used on multiple QtPy classes
"""

import pyside_utils
from PySide2 import QtWidgets, QtTest, QtCore
from enum import IntEnum


class CheckBoxStates(IntEnum):
    Off = 0
    On = 1


class QtPyCommon:
    """
    """

    def __init__(self):
        self.menu_bar = None

    def click_menu_bar_option(self, option_text: str) -> None:
        """
        function for clicking a menu bar option from a Qt window object. This function bypasses menu categories.
        for example, if you want to click the Open option from the "File" toolbar menu provide "Open" as your menu text
        instead of "File" then "Open".

        param option_text: the label string used in the menu option that you want to click

        returns None
        """
        action = pyside_utils.find_child_by_pattern(self.menu_bar, {"text": option_text, "type": QtWidgets.QAction})

        assert action is not None, "Unable to find QtWidgets type or menu option"

        action.trigger()

    def expand_qt_container_rows(self, qt_widget, category_name: str) -> None:
        """
        Function used for expanding qt containers with rows that have nested qt widgets

        param qt_widget: the qtwidget (typically a qframe) with a qwidget category we want to expand
        param object_name: qt object name as a string

        returns: None
        """
        children = qt_widget.findChildren(QtWidgets.QFrame, category_name)

        assert children is not None, f"Unable to find QtWidget container: {category_name}"

        for child in children:

            check_box = child.findChild(QtWidgets.QCheckBox)

            if check_box and not check_box.isChecked():
                check_box.click()
