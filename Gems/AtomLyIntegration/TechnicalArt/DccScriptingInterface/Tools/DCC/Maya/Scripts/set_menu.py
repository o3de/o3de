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
"""
Module Documentation:
    DccScriptingInterface//Tools//DCC//maya//scripts//set_menu.py

This module creates and manages a DCCsi mainmenu
"""
# -------------------------------------------------------------------------
# -- Standard Python modules
import logging as _logging

# -- External Python modules
# none

# -- DCCsi Extension Modules
import DccScriptingInterface.azpy
from DccScriptingInterface.Tools.DCC.Maya.constants import OBJ_DCCSI_MAINMENU
from DccScriptingInterface.Tools.DCC.Maya.constants import TAG_DCCSI_MAINMENU

# -- maya imports
import pymel.core as pm
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
#  global scope
from DccScriptingInterface.Tools.DCC.Maya import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.set_menu'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.info(f'Initializing: {_MODULENAME}')

from DccScriptingInterface.globals import *
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def menu_cmd_test():
    _LOGGER.info('test_func(), is TESTING main menu')
    return
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
def set_main_menu(obj_name=OBJ_DCCSI_MAINMENU, label=TAG_DCCSI_MAINMENU):
    _main_window = pm.language.melGlobals['gMainWindow']

    _menu_obj = obj_name
    _menu_label = label

    # check if it already exists and remove (so we don't duplicate)
    if pm.menu(_menu_obj, label=_menu_label, exists=True, parent=_main_window):
        pm.deleteUI(pm.menu(_menu_obj, e=True, deleteAllItems=True))

    # create the main menu object
    _custom_tools_menu = pm.menu(_menu_obj,
                                 label=_menu_label,
                                 parent=_main_window,
                                 tearOff=True)

    # make a dummpy sub-menu
    pm.menuItem(label='Menu Item Stub',
                subMenu=True,
                parent=_custom_tools_menu,
                tearOff=True)

    # make a dummy menu item to test
    pm.menuItem(label='Test', command=pm.Callback(menu_cmd_test))
    return _custom_tools_menu

# ==========================================================================
# Run as LICENSE
#==========================================================================
if __name__ == '__main__':

    _custom_menu = set_main_menu()
