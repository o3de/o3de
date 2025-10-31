# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------
"""This module creates a DccScriptingInterface menu in Lumberyard.
This module is designed to only run in Lumberyard and py3.7+"""
# -- Standard Python modules
import sys
import os
import importlib.util

# -- External Python modules
from pathlib import Path

# -- DCCsi Extension Modules
import azpy

# Lumberyard extension modules
import azlmbr
import azlmbr.bus
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# some basic DCCsi module setup, DCCsi bootstrap already performed some
from dynaconf import settings
settings.setenv()

# -------------------------------------------------------------------------
_log_level = int(settings.DCCSI_LOGLEVEL)
if settings.DCCSI_GDEBUG:
    _log_level = int(10)  # force debug level

_MODULENAME = r'DCCsi.SDK.Lumberyard.Scripts.set_menu'

_LOGGER = azpy.initialize_logger(_MODULENAME, default_log_level=_log_level)
_LOGGER.debug(f'Invoking:: {0}.'.format({_MODULENAME}))

# early attach WingIDE debugger (can refactor to include other IDEs later)
if settings.DCCSI_DEV_MODE:
    from azpy.env_bool import env_bool
    if not env_bool('DCCSI_DEBUGGER_ATTACHED', False):
        # if not already attached lets do it here
        from azpy.test.entry_test import connect_wing
        foo = connect_wing()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# validate pyside before continuing
try:
    azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, 'IsActive')
    params = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, 'GetQtBootstrapParameters')
    params is not None and params.mainWindowId != 0
    from PySide2 import QtWidgets
except Exception as e:
    _LOGGER.error(f'Pyside not available, exception: {e}')
    raise e

# keep going, import the other PySide2 bits we will use
from PySide2 import QtGui
from PySide2.QtCore import Slot
from shiboken2 import wrapInstance, getCppPointer
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_ly_mainwindow():
    widget_main_window = QtWidgets.QWidget.find(params.mainWindowId)
    widget_main_window = wrapInstance(int(getCppPointer(widget_main_window)[0]), QtWidgets.QMainWindow)
    return widget_main_window
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class SampleUI(QtWidgets.QDialog):
    """Lightweight UI Test Class created a button"""
    def __init__(self, parent, title='Not Set'):
        super(SampleUI, self).__init__(parent)
        self.setWindowTitle(title)
        self.initUI()

    def initUI(self):
        mainLayout = QtWidgets.QHBoxLayout()
        testBtn = QtWidgets.QPushButton("I am just a Button man!")
        mainLayout.addWidget(testBtn)
        self.setLayout(mainLayout)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_tag_str = 'DccScriptingInterface'
_LOGGER.info(f'Creating {_tag_str} Menu')
# Lumberyards main window
widget_main_window = get_ly_mainwindow()

# create our own menuBar
dccsi_menu = widget_main_window.menuBar().addMenu(f"&{_tag_str}")

# nest a menu for util/tool launching
dccsi_launch_menu = dccsi_menu.addMenu("Launch Utility")

# (1) add the first utility: Substance Builder
# to do, make adding this a data-driven dynaconf option configured per-project?
action_launch_sub_builder = dccsi_launch_menu.addAction("Substance Builder")

@Slot() 
def clicked_launch_sub_builder():
    # breadcrumbs
    if settings.DCCSI_GDEBUG:
        debug_msg = "Clicked action_launch_sub_builder"
        print(debug_msg)
        _LOGGER.debug(debug_msg)

    _SUB_BUILDER_PATH = Path(settings.PATH_DCCSIG,
                             'SDK',
                             'Substance',
                             'builder')
    _SUB_BUILDER_BOOTSTRAP = Path(_SUB_BUILDER_PATH, 'bootstrap.py')

    # bootstrap is a generic name and we have many of them
    # this ensures we are loading a specific one
    _spec_bootstrap = importlib.util.spec_from_file_location("dccsi.sdk.substance.builder.bootstrap",
                                                            _SUB_BUILDER_BOOTSTRAP)
    _app_bootstrap = importlib.util.module_from_spec(_spec_bootstrap)
    _spec_bootstrap.loader.exec_module(_app_bootstrap)
    
    while 1:  # simple PySide2 test, set to 0 to disable
        ui = SampleUI(parent=widget_main_window, title='Atom: Substance Builder')
        ui.show()
        break
    return

# Add click event to menu bar
action_launch_sub_builder.triggered.connect(clicked_launch_sub_builder)
# -----------------------------------------------------------------

# add launching the dcc material converter util to the menu
# to do, make adding this a data-driven dynaconf option?
action_launch_dcc_mat_converter = dccsi_launch_menu.addAction("DCC Material Converter")

@Slot()
def clicked_launch_dcc_mat_converter():
    if settings.DCCSI_GDEBUG:
        debug_msg = "Clicked action_launch_dcc_mat_converter"
        print(debug_msg)
        _LOGGER.debug(debug_msg)
    
    while 1:  # simple PySide2 test, set to 0 to disable
        ui = SampleUI(parent=widget_main_window, title='Atom: DCC Material Converter')
        ui.show()
        break

    return

# Add click event to menu bar
action_launch_dcc_mat_converter.triggered.connect(clicked_launch_dcc_mat_converter)
                

