"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------
"""Generated from O3DE PythonGem Template"""

import azlmbr
from shiboken2 import wrapInstance, getCppPointer
from PySide2 import QtCore, QtWidgets, QtGui
from PySide2.QtCore import QEvent, Qt
from PySide2.QtWidgets import QAction, QDialog, QHeaderView, QLabel, QLineEdit, QPushButton, QSplitter, QTreeWidget, QTreeWidgetItem, QWidget, QAbstractButton

# Once PySide2 has been bootstrapped, register our GenericDialog with the Editor

class GenericDialog(QDialog):
    def __init__(self, parent=None):
        super(GenericDialog, self).__init__(parent)
        self.setWindowTitle("Custom Dialog")
