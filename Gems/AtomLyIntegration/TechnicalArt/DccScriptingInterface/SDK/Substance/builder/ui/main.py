# -*- coding: utf-8 -*-
# !/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

import os
import sys

from PySide2.QtCore import QFile, QSize
from PySide2.QtUiTools import QUiLoader
from PySide2.QtWidgets import QApplication, QSizePolicy
from PySide2.QtWidgets import QMainWindow, QWidget, QVBoxLayout, QHBoxLayout

_MODULE_DIR_PATH = os.path.dirname(os.path.abspath(__file__))
_UI_FILEPATH = "{0}\\\\sbs_builder_widget.ui".format(_MODULE_DIR_PATH)
_PROGRAM_NAME_VERSION = 'Substance Builder'


class UiLoader(QUiLoader):
    def __init__(self, baseInstance):
        super(UiLoader, self).__init__(baseInstance)
        self._baseInstance = baseInstance

    def createWidget(self, classname, parent=None, name=""):
        widget = super(UiLoader, self).createWidget(
            classname, parent, name)

        if parent is None:
            return self._baseInstance
        else:
            setattr(self._baseInstance, name, widget)
            return widget


class MainWindow(QMainWindow):
    def __init__(self, ui_file=_UI_FILEPATH, parent=None):
        super().__init__(parent)

        # Setup central widget and layout
        self.central_widget = QWidget()
        self.central_layout = QVBoxLayout(self.central_widget)
        self.central_layout.setContentsMargins(8, 8, 8, 8)
        self.setCentralWidget(self.central_widget)
        self.setup_layout(ui_file)
        self.setMinimumSize(QSize(675, 825))

    def setup_layout(self, ui_file):
        self.layout = QHBoxLayout()
        self.widget = MyWidget(ui_file, self)
        self.widget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.layout.addWidget(self.widget)
        self.central_layout.addLayout(self.layout)


class MyWidget(QWidget):
    def __init__(self, ui_file=_UI_FILEPATH, parent=None):
        super().__init__(parent)
        loader = UiLoader(parent)
        file = QFile(ui_file)
        file.open(QFile.ReadOnly)
        loader.load(file, self)
        file.close()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    mainWindow = MainWindow(_UI_FILEPATH)
    mainWindow.setWindowTitle(_PROGRAM_NAME_VERSION)
    mainWindow.show()
    sys.exit(app.exec_())
