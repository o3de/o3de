# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! @brief: help_menu.py: Setup a standard Help item in the menubar for PySide2 GUIs"""

# built in's
import os
import logging
from pathlib import Path
import logging as _logging

# 3rd Party
from PySide2 import QtWidgets
from PySide2.QtWidgets import QMenuBar, QMenu, QAction
from PySide2 import QtGui
from PySide2 import QtCore
from PySide2.QtCore import Slot, QObject, QUrl
from shiboken2 import wrapInstance, getCppPointer

# -------------------------------------------------------------------------
#  global scope
from DccScriptingInterface.azpy.shared.ui import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.help_menu'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.info(f'Initializing: {_MODULENAME}')

from DccScriptingInterface.globals import *

# default o3de bug report link, for all tools
O3DE_BUG_REPORT_URL = "https://github.com/o3de/o3de/issues/new?assignees=&labels=needs-triage%2Cneeds-sig%2Ckind%2Fbug&template=bug_template.md&title=Bug+Report"

# default dcc app help page, default is for maya
DCCSI_APP_HELP_URL = "https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Maya/readme.md"

# default generic help url (should be replaced per-tool)
HELP_URL = 'https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/readme.md'
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class HelpMenu():
    """
    Setup a standard Help item in the menubar for PySide2 GUIs

    INPUTS:
    @param parent_widget: the class instance of the QMainWindow, or QWidget object
        note: parent_widget must have it's own menuBar

    @param dcc_app_label: Label for the tools parent dcc app help
    @param dcc_app_url: URL for the tools parent dcc app help link
35
    @param tool_label: the menu label for the tool specific help item
    @param tool_help_page: the http:// url path to the tool specific help page

    Example usage:
    # self.help_menu = azpy.shared.ui.help_menu.HelpMenu(self, 'Help...', https://some.site.com/azpy')

    """

    # ----------------------------------------------------------------------
    def __init__(self, parent_widget,

                 tool_label: str = 'Tool Help...',
                 tool_help_url: str = HELP_URL,

                 dcc_app_label: str = 'DCC Help...',
                 dcc_app_url: str = DCCSI_APP_HELP_URL,

                 bug_label: str = 'Report a Tool Bug...',
                 bug_url: str = O3DE_BUG_REPORT_URL):

        """Constructor"""

        # parent widget with a menuBar
        self.parent_widget = parent_widget

        # retreive and store the parent widget's menuBar
        try:
            self.menubar = self.parent_widget.menuBar
        except AttributeError as e:
            _LOGGER.warning(f'parnet: {self.parent_widget}, has no menuBar widget')
            _LOGGER.error(f'{e} , traceback =', exc_info=True)

        # top level help menu
        self.help_menu = QtWidgets.QMenu(self.menubar)
        self.menubar.addMenu(self.help_menu)
        self.help_menu.setObjectName("help_menu")
        self.help_menu.setTitle("Help")

        self.tool_label = tool_label
        self.tool_help_url = tool_help_url
        self.set_specific_tool_help(self.tool_label)

        self.dcc_app_label = dcc_app_label
        self.dcc_app_url = dcc_app_url
        self.set_generic_tool_help(self.dcc_app_label)

        self.bug_label = bug_label
        self.bug_url = bug_url
        self.set_tool_bug_report(self.bug_label)

    # --method-set---------------------------------------------------------
    def set_specific_tool_help(self, label, obj_name='tool_action_help'):
        """"""
        self.tool_action_help = QtWidgets.QAction(self.parent_widget)
        self.tool_action_help.setObjectName(obj_name)
        self.help_menu.addAction(self.tool_action_help)
        self.menubar.addAction(self.help_menu.menuAction())
        self.tool_action_help.setText(label)
        self.parent_widget.connect(self.tool_action_help, QtCore.SIGNAL("triggered()"), self.tool_help_display)

    def set_generic_tool_help(self, label, obj_name='azpy_tool_action_help'):
        """"""
        self.azpy_tool_action_help = QtWidgets.QAction(self.parent_widget)
        self.azpy_tool_action_help.setObjectName(obj_name)
        self.help_menu.addAction(self.azpy_tool_action_help)
        self.menubar.addAction(self.help_menu.menuAction())
        self.azpy_tool_action_help.setText(label)
        self.parent_widget.connect(self.azpy_tool_action_help, QtCore.SIGNAL("triggered()"), self.dccsi_help_displaydcc_app_help_display)

    def set_tool_bug_report(self, label, obj_name='tool_action_bug_report'):
        """"""
        self.tool_action_bug_report = QtWidgets.QAction(self.parent_widget)
        self.tool_action_bug_report.setObjectName(obj_name)
        self.help_menu.addAction(self.tool_action_bug_report)
        self.menubar.addAction(self.help_menu.menuAction())
        self.tool_action_bug_report.setText(label)
        self.parent_widget.connect(self.tool_action_bug_report, QtCore.SIGNAL("triggered()"), self.bug_report_display)


    def tool_help_display(self):
        return QtGui.QDesktopServices.openUrl(QUrl(self.tool_help_url, QUrl.TolerantMode))

    def dccsi_help_displaydcc_app_help_display(self):
        return QtGui.QDesktopServices.openUrl(QUrl(self.dcc_app_url, QUrl.TolerantMode))

    def bug_report_display(self):
        return QtGui.QDesktopServices.openUrl(QUrl(self.bug_url, QUrl.TolerantMode))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class TestMainWindow(QtWidgets.QMainWindow):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setup_ui()

    def setup_ui(self):
        self.setWindowTitle('PySide2-HelpMenu-Test')

        # Setup Help Menu
        self.help_menu = HelpMenu(self, 'PySide2-Test Help...', 'http://dccSI.com/NewTool')

        # main widget
        self.main_widget = QtWidgets.QWidget(self)
        self.setCentralWidget(self.main_widget)

        # layout initialize
        self.global_layout = QtWidgets.QVBoxLayout()
        self.main_widget.setLayout(self.global_layout)

        # Add Widgets
        self.spinbox = QtWidgets.QSpinBox()
        self.spinbox.setValue(30)
        layout = QtWidgets.QFormLayout()
        layout.addRow('Parameter', self.spinbox)
        self.button = QtWidgets.QPushButton('Execute')

        # global layout setting
        self.global_layout.addLayout(layout)
        self.global_layout.addWidget(self.button)
    # ---------------------------------------------------------------------
    def closeEvent(self, event):
        """Event which is run when window closes"""

        _LOGGER.debug("Exiting: {0}".format(_MODULENAME))
    # ---------------------------------------------------------------------
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""
    import sys

    _LOGGER.debug("Starting Test: {0} ...".format(_MODULENAME))
    app = QtWidgets.QApplication(sys.argv)
    mainWin = TestMainWindow()
    mainWin.show()

    del _LOGGER
    sys.exit(app.exec_())







