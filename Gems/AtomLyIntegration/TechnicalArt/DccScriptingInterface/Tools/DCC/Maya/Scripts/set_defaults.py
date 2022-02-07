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
    DccScriptingInterface:: SDK//maya//scripts//set_pref_defaults.py

This module manages a predefined set of prefs for maya
"""
# -------------------------------------------------------------------------
# -- Standard Python modules
import os
import sys
# -- External Python modules

# -- DCCsi Extension Modules
import azpy
from azpy.constants import *

# -- maya imports
import maya.cmds as mc
import maya.mel as mm

# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

#  global space
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_MODULENAME = r'DCCsi.SDK.Maya.Scripts.set_defaults'

_LOGGER = azpy.initialize_logger(_MODULENAME, default_log_level=int(20))
_LOGGER.debug('Invoking:: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def set_defaults(units='meter'):
    """This method will make defined settings changes to Maya prefs, 
    to better configure maya to work with Lumberyard"""
    # To Do: make this data-driven env/settings, game teams should be able
    # to opt out and/or set their prefered configuration.
    
    _LOGGER.debug('set_defaults_lumberyard() fired')

    # set up default units ... this should be moved to bootstrap config
    _LOGGER.info('Default, 1 Linear Game Unit in Lumberyard == 1 Meter'
                 ' in Maya content. Setting default linear units to Meters'
                 ' (user can change to other units in the preferences)')

    result = mc.currentUnit(linear=units)

    # set up grid defaults
    _LOGGER.info('Setting Grid defaults, to match default unit scale.'
                 '(user can change grid config manually')
    try:
        mc.grid(size=32, spacing=1, divisions=10)
    except Exception as e:
        _LOGGER.warning('{0}'.format(e))

    # viewFit
    _LOGGER.info('Changing default mc.viewFit')
    try:
        mc.viewFit()
    except Exception as e:
        _LOGGER.warning('{0}'.format(e))
        
    # some mel commands
    _LOGGER.info('Changing sersp camera clipping planes')
    try:
        mm.eval(str(r'setAttr "perspShape.nearClipPlane" 0.01;'))
        mm.eval(str(r'setAttr "perspShape.farClipPlane" 1000;'))
    except Exception as e:
        _LOGGER.warning('{0}'.format(e))

    # set up fixPaths
    _LOGGER.info('~ Setting up fixPaths in default scene')

    return 0
# -------------------------------------------------------------------------
