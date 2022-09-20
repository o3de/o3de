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

    def click_menu_option(self, window, option_text):
        """
        function for clicking an option from a Qt menu object. This function bypasses menu groups or categories. for example,
        if you want to click the Open option from the "File" category provide "Open" as your menu text instead of "File" then "Open".

        param window: the qt window object where the menu option is located
        param option_text: the label string used in the menu option that you want to click

        returns none
        """
        action = pyside_utils.find_child_by_pattern(window, {"text": option_text, "type": QtWidgets.QAction})
        action.trigger()

    def expand_qt_container_rows(self, object_name):
        """
        function used for expanding qt container rows with expandable children. May need to refactor this to replace
        self.findChildren with container.findChildren

        param object_name: qt object name as a string

        returns: none
        """
        children = self.findChildren(QtWidgets.QFrame, object_name)
        for child in children:
            check_box = child.findChild(QtWidgets.QCheckBox)
            if check_box and not check_box.isChecked():
                QtTest.QTest.mouseClick(check_box, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier)