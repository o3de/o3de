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
def export_scene_materials(args):
    # It is unclear why a False argument is being passed when called from the menu item below, but needed to add args
    # for function to fire

    _LOGGER.info('Exporting Scene Materials')

    from importlib import reload
    from DccScriptingInterface.azpy.dcc.maya.helpers import maya_materials_conversion
    from DccScriptingInterface.azpy.o3de.renderer.materials import material_utilities
    from DccScriptingInterface.azpy.o3de.renderer.materials import o3de_material_generator
    from DccScriptingInterface.azpy.dcc.maya.helpers import convert_aiStandardSurface_material
    from DccScriptingInterface.azpy.dcc.maya.helpers import convert_aiStandard_material
    from DccScriptingInterface.azpy.dcc.maya.helpers import convert_stingray_material
    from DccScriptingInterface.azpy import general_utils
    from DccScriptingInterface.Tools.DCC.Maya.Scripts.Python.scene_exporter import export_tool

    reload(maya_materials_conversion)
    reload(material_utilities)
    reload(o3de_material_generator)
    reload(convert_aiStandardSurface_material)
    reload(convert_aiStandard_material)
    reload(convert_stingray_material)
    reload(general_utils)
    reload(export_tool)
    export_tool.SceneExporter('convert')
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

    # Conversion Section (sub-menu)
    _LOGGER.info('A')
    pm.menuItem(label='Scene Export',
                subMenu=True,
                parent=_custom_tools_menu,
                tearOff=True)

    # Conversion Section Menu Items
    pm.menuItem(label='Export Scene FBX Models and Materials', command=export_scene_materials)

    return _custom_tools_menu

# ==========================================================================
# Run as LICENSE
#==========================================================================
if __name__ == '__main__':

    _custom_menu = set_main_menu()
