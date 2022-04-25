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
"""
Module Documentation:
    DccScriptingInterface:: SDK//maya//scripts//set_menu.py

This module creates and manages a DCCsi mainmenu
"""
# -------------------------------------------------------------------------
# -- Standard Python modules
from functools import partial

# -- External Python modules
# none

# -- DCCsi Extension Modules
import azpy
from constants import OBJ_DCCSI_MAINMENU
from constants import TAG_DCCSI_MAINMENU

# -- maya imports
import pymel.core as pm

# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

#  global space
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_MODULENAME = r'DCCsi.SDK.Maya.Scripts.set_menu'

_LOGGER = azpy.initialize_logger(_MODULENAME, default_log_level=int(20))
_LOGGER.debug('Invoking:: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# MATERIALS HELPER --------------------------------------------------------

def materials_helper_launch(operation):
    MaterialsHelper(operation)


class MaterialsHelper(QtWidgets.QWidget):
    def __init__(self, operation, parent=None):
        super().__init__(parent)

        self.setParent(mayaMainWindow)
        self.setWindowFlags(QtCore.Qt.Window)
        self.setGeometry(500, 500)
        self.setObjectName('MaterialsHelper')
        self.setWindowTitle('Materials Helper')
        self.isTopLevel()

        self.operation = operation
        self.set_window()

    def set_window(self):
        _LOGGER.info(f'Set window fired in Materials Conversion... Operation: {self.operation}')

# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def set_main_menu(obj_name=OBJ_DCCSI_MAINMENU, label=TAG_DCCSI_MAINMENU):
    _main_window = pm.language.melGlobals['gMainWindow']
    
    _menu_obj = obj_name
    _menu_label = label

    # check if it already exists and remove (so we don't duplicate)
    if pm.menu(_menu_obj, label=_menu_label, exists=True, parent=_main_window):
        pm.deleteUI(pm.menu(_menu_obj, e=True, deleteAllItems=True))

    # Main menu object
    _custom_tools_menu = pm.menu(_menu_obj,
                                 label=_menu_label,
                                 parent=_main_window,
                                 tearOff=True)

    # +++++++++++++++++--->>
    # Materials Helper ---->>
    # +++++++++++++++++--->>

    pm.menuItem(label='Materials Helper',
                subMenu=True,
                parent=_custom_tools_menu,
                tearOff=True)
    
    # Sub-menu items
    pm.menuItem(label='Query Material Settings', command=pm.Callback(partial(self.materials_helper_launch, 'query')))
    pm.menuItem(label='Export Material to O3DE', command=pm.Callback(partial(self.materials_helper_launch, 'convert')))

    return _custom_tools_menu

# ==========================================================================
# Run as LICENSE
# ==========================================================================


if __name__ == '__main__':
    _custom_menu = set_main_menu()
