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
import PySide2.QtCore as QtCore
import PySide2.QtGui as QtGui
import PySide2.QtWidgets as QtWidgets

# -------------------------------------------------------------------------
#  global scope
from DccScriptingInterface.azpy.shared.ui import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.help_menu'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.info(f'Initializing: {_MODULENAME}')

from DccScriptingInterface.globals import *

# TODO: implement this
# checks the run configuration to determine if we are running in maya
_G_MAYA = False
try:
    import maya.cmds as mc
    _G_MAYA = True
except:
    pass

DCCSI_MAYA_HELP_URL = "https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Maya/readme.md"

O3DE_BUG_REPORT_URL = "https://github.com/o3de/o3de/issues/new?assignees=&labels=needs-triage%2Cneeds-sig%2Ckind%2Fbug&template=bug_template.md&title=Bug+Report"
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class HelpMenu():
    """
    Setup a standard Help item in the menubar for PySide2 GUIs

    INPUTS:
    main_window = the class instance of the QMainWindow
    tool_label = the menu label for the tool's help item
    tool_help_page = the http:// path to the tool specific help page

    Here's an example
    # self.help_menu = azpy.shared.ui.help_menu.setup(self, 'Help...', 'https://some.site.com/azpy')

    """

    # ----------------------------------------------------------------------
    def __init__(self, parent_widget, tool_label, tool_help_page):
        """Constructor"""

        self.parent_widget = parent_widget
        # store mainwindow menubar, we can attach the Help menu to this
        self.menubar = self.parent_widget.menuBar
        self.tool_label = tool_label
        self.tool_help_page = tool_help_page

        self.help_menu = QtWidgets.QMenu(self.menubar)
        self.menubar.addMenu(self.help_menu)
        self.help_menu.setObjectName("help_menu")
        self.help_menu.setTitle("Help")

        self.specific_tool_help_setup()
        self.generic_tool_help_setup()
        self.tool_bug_report_setup()

    # ----------------------------------------------------------------------

    def specific_tool_help_setup(self):
        """"""
        self.tool_action_help = QtWidgets.QAction(self.parent_widget)
        self.tool_action_help.setObjectName("tool_action_help")
        self.help_menu.addAction(self.tool_action_help)
        self.menubar.addAction(self.help_menu.menuAction())
        self.tool_action_help.setText(self.tool_label)
        self.parent_widget.connect(self.tool_action_help, QtCore.SIGNAL("triggered()"), self.tool_help_display)

    # ----------------------------------------------------------------------
    def generic_tool_help_setup(self):
        """"""
        self.azpy_tool_action_help = QtWidgets.QAction(self.parent_widget)
        self.azpy_tool_action_help.setObjectName("azpy_tool_action_help")
        self.help_menu.addAction(self.azpy_tool_action_help)
        self.menubar.addAction(self.help_menu.menuAction())
        self.azpy_tool_action_help.setText("DCCsi help...")
        self.parent_widget.connect(self.azpy_tool_action_help, QtCore.SIGNAL("triggered()"), self.azpy_tool_help_display)

    # ----------------------------------------------------------------------
    def tool_bug_report_setup(self):
        """"""
        self.tool_action_bug_report = QtWidgets.QAction(self.parent_widget)
        self.tool_action_bug_report.setObjectName("tool_action_bug_report")
        self.help_menu.addAction(self.tool_action_bug_report)
        self.menubar.addAction(self.help_menu.menuAction())
        self.tool_action_bug_report.setText("Report a Tool Bug...")
        self.parent_widget.connect(self.tool_action_bug_report, QtCore.SIGNAL("triggered()"), self.bug_report_display)

    # ----------------------------------------------------------------------

    def tool_help_display(self):
        """"""
        if _G_MAYA:
            mc.showHelp(self.tool_help_page, absolute=True)
        else:
            _LOGGER.debug('This command, {0}: currently only works when running in Maya.'.format('tool_help_display'))
        pass

    # ----------------------------------------------------------------------
    def azpy_tool_help_display(self):
        """"""
        if _G_MAYA:
            mc.showHelp(DCCSI_MAYA_HELP_URL, absolute=True)
        else:
            _LOGGER.debug('This command, {0}: currently only works when running in Maya.'.format('azpy_tool_help_display'))
        pass

    # ----------------------------------------------------------------------
    def bug_report_display(self):
        """"""
        if _G_MAYA:
            mc.showHelp(O3DE_BUG_REPORT_URL, absolute=True)
        else:
            _LOGGER.debug('This command, {0}: currently only works when running in Maya.'.format('bug_report_display'))
        pass
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
    # ----------------------------------------------------------------------

    def closeEvent(self, event):
        """Event which is run when window closes"""

        _LOGGER.debug("Exiting: {0}".format(_TOOL_TAG))
    # ----------------------------------------------------------------------
# -------------------------------------------------------------------------


if __name__ == '__main__':
    """Run this file as main"""
    import sys

    _LOGGER.debug("{0} :: if __name__ == '__main__':".format(_TOOL_TAG))
    _LOGGER.debug("Starting App: {0} ...".format(_TOOL_TAG))
    app = QtWidgets.QApplication(sys.argv)
    mainWin = TestMainWindow()
    mainWin.show()

    del _LOGGER
    sys.exit(app.exec_())







