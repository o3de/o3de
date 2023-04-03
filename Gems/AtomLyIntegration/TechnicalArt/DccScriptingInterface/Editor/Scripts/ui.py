#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! brief

UI methods for DccScriptingInterface Editor framework

:file: DccScriptingInterface\\editor\\scripts\\ui.py
"""
# standard imports
import os
import subprocess
from pathlib import Path
import logging as _logging
#PySide2 imports
from PySide2 import QtWidgets
from PySide2.QtWidgets import QMenuBar, QMenu, QAction
from PySide2 import QtGui
from PySide2.QtCore import Slot, QObject, QUrl
from shiboken2 import wrapInstance, getCppPointer
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.Editor.Scripts import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.ui'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

# this accesses common global state, e.g. DCCSI_GDEBUG (is True or False)
from DccScriptingInterface.globals import *

import DccScriptingInterface.config as dccsi_core_config
_settings_core = dccsi_core_config.get_config_settings(enable_o3de_python=True,
                                                       enable_o3de_pyside2=False,
                                                       set_env=True)
# -------------------------------------------------------------------------


# - slot ------------------------------------------------------------------
# as the list of slots/actions grows, refactor into sub-modules
@Slot()
def click_action_sampleui():
    """! Creates a standalone sample ui with button, this is provided for
    Technicl Artists learning, as one of the purposes of the dccsi is
    onboarding TAs to the editor extensibility experience.

    :return: returns the created ui
    """
    _LOGGER.debug(f'Clicked: click_action_sampleui')

    ui = SampleUI(parent=az_qt_helpers.get_editor_main_window(),
                  title='Dccsi: SampleUI')
    ui.show()
    return
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def add_action(parent: QMenu,
               title: str = "SampleUI",
               action_slot = click_action_sampleui) -> QAction:
    """! adds an action to the parent QMenu
    :param parent_menu: the parent Qmenu to add an action to
    :param title: The UI text str for the menu action
    :param action_slot: @Slot decorated method, see click_sampleui()
    :return: returns the created action
    """
    _LOGGER.debug(f"Creating '{title}' action for menu '{parent.title()}'")

    action = None

    action_list = parent.findChildren(QAction)
    for a in action_list:
        if a.text() == f"&{title}":
            action = a

    if not action:
        action = parent.addAction(f"&{title}")

        # click_sampleui signal
        action.triggered.connect(action_slot)

    return action
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def create_menu(parent: QMenu, title: str = 'StudioTools') -> QMenu:
    """! Creates a 'Studio Tools' menu for the DCCsi functionality
    :param parent: The parent QMenu (or QMenuBar)
    :param : The UI text str for the submenu
    :return: returns the created submenu
    """
    _LOGGER.debug(f"Creating a dccsi menu: '{title}'")

    dccsi_menu = None

    menu_list = parent.findChildren(QMenu)
    for m in menu_list:
        if m.title() == f"&{title}":
            dccsi_menu = m

    if not dccsi_menu:
        # create our own dccsi menu
        dccsi_menu = parent.addMenu(f"&{title}")

    return dccsi_menu
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def start_service(py_file: Path = None,
                  debug: bool = DCCSI_GDEBUG) -> subprocess:
    """! Common method to start the external application start script

    :param : The UI text str for the submenu
    :return: returns the created submenu
    """
    _LOGGER.debug(f'Starting Service: {py_file.parent.name}')

    cmd = [str(_settings_core.DCCSI_PY_BASE), str(py_file)]

    # there are at least three ways to go about this ...
    # the first, is to call the function
    # this doesn't work well with O3DE, because the environ is propogated
    # and o3de python and Qt both interfer with wing boot and operation

#     try:
#         #wing_proc = wing_start.call()
#         wing_proc = wing_start.popen()
#     except Exception as e:
#         _LOGGER.error(f'{e} , traceback =', exc_info=True)
#         return None
#     # this sort of works, seems to stall the editor until wing closes.

    # the second, we could try to execute another way ...
    # probably the same results as above, it's also probably not safe
#     py_file = Path(wing_config.settings.PATH_DCCSI_TOOLS_IDE_WING, 'start.py').resolve()
#     try:
#         wing_proc = exec(open(f"{py_file.as_posix()}").read())
#     except Exception as e:
#         _LOGGER.error(f'{e} , traceback =', exc_info=True)
#         return None
    # tries to execute but fails to do so correctly

    if debug:
        # This approach will block the editor but can return a callstack
        # tThis is valuable to debug boot issues with start.py code itself
        # You must manually enable this flag near beginning of this module
        p = subprocess.Popen(cmd,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = p.communicate()
        _LOGGER.debug(f'out: {str(out)}')
        _LOGGER.error(f'err: {str(err)}')
        _LOGGER.debug(f'returncode: {str(p.returncode)}')
        _LOGGER.debug('EXIT')

    else:
        # This is non-blocking, but if it fails can be difficult to debug
        p = subprocess.Popen(cmd)
        _LOGGER.debug('pid', p.pid)
        _LOGGER.debug('EXIT')

    return p
# -------------------------------------------------------------------------
