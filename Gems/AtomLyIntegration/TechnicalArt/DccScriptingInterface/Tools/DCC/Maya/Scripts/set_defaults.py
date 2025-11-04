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
    DccScriptingInterface:: SDK//maya//scripts//set_pref_defaults.py

This module manages a predefined set of prefs for maya
"""
# -------------------------------------------------------------------------
# -- Standard Python modules
import os
import sys
import logging as _logging

# -- External Python modules
# none

# -- DCCsi Extension Modules
import DccScriptingInterface.azpy
from DccScriptingInterface.azpy.constants import *

# -- maya imports
import maya.cmds as mc
import maya.mel as mm

# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
#  global scope
from DccScriptingInterface.Tools.DCC.Maya import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.set_defaults'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.info(f'Initializing: {_MODULENAME}')

from DccScriptingInterface.globals import *
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
