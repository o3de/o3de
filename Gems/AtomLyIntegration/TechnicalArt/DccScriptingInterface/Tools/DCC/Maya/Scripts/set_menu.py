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
    DccScriptingInterface:: SDK//maya//scripts//set_menu.py

This module creates and manages a DCCsi mainmenu
"""
# -------------------------------------------------------------------------
# -- Standard Python modules
# none

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
    from azpy.dcc.maya.helpers import maya_materials_conversion
    from azpy.o3de.renderer.materials import material_utilities
    from azpy.o3de.renderer.materials import o3de_material_generator
    from azpy.dcc.maya.helpers import convert_aiStandardSurface_material
    from azpy.dcc.maya.helpers import convert_aiStandard_material
    from azpy.dcc.maya.helpers import convert_stingray_material
    from SDK.Python import general_utilities
    from SDK.Maya.Scripts.Python.maya_menu_items import materials_helper

    reload(maya_materials_conversion)
    reload(material_utilities)
    reload(o3de_material_generator)
    reload(convert_aiStandardSurface_material)
    reload(convert_aiStandard_material)
    reload(convert_stingray_material)
    reload(general_utilities)
    reload(materials_helper)
    materials_helper.MaterialsHelper('convert')
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
    pm.menuItem(label='Conversion Utilities',
                subMenu=True,
                parent=_custom_tools_menu,
                tearOff=True)

    # Conversion Section Menu Items
    pm.menuItem(label='Export Scene Materials', command=export_scene_materials)

    return _custom_tools_menu

# ==========================================================================
# Run as LICENSE
#==========================================================================
if __name__ == '__main__':

    _custom_menu = set_main_menu()
