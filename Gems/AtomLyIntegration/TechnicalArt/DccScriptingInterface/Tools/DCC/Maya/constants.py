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
"""! @brief
Module Documentation:
    < DCCsi >:: Tools/DCC/Maya/constants.py

This module contains default values for commonly used constants & strings.
We can make an update here easily that is propagated elsewhere.
"""
# -------------------------------------------------------------------------
# built-ins
import sys
import os
import site
import timeit
import inspect
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_START = timeit.default_timer() # start tracking
# global scope
from DccScriptingInterface.Tools.DCC.Maya import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.constants'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
# Maya is frozen, that might cause an issue retreiving module paths?
_MODULE_PATH = Path(__file__)
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# dccsi imports here
from DccScriptingInterface import PATH_DCCSIG

from DccScriptingInterface.Tools.DCC.Maya import PATH_DCCSI_TOOLS_DCC_MAYA
from DccScriptingInterface.Tools.DCC.Maya import ENVAR_PATH_DCCSI_TOOLS_DCC_MAYA

from DccScriptingInterface.constants import ENVAR_PATH_DCCSI_TOOLS_DCC
from DccScriptingInterface.constants import ENVAR_PATH_DCCSI_TOOLS
from DccScriptingInterface.constants import ENVAR_DCCSI_PY_DEFAULT
from DccScriptingInterface.constants import ENVAR_DCCSI_PY_IDE

from DccScriptingInterface import PATH_PROGRAMFILES_X64
from DccScriptingInterface import PATH_DCCSI_PYTHON_LIB
from DccScriptingInterface import O3DE_DEV

ENVAR_DCCSI_CONFIG_DCC_MAYA = 'DCCSI_CONFIG_DCC_MAYA'
DCCSI_CONFIG_DCC_MAYA = True

# this is the shared default requirements.txt file to install for python 3.6.x+
DCCSI_PYTHON_REQUIREMENTS = f'{PATH_DCCSIG}\\requirements.txt'
# if using maya 2020 or less with py2.7 override with and use the one here:
# "DccScriptingInterface\Tools\DCC\Maya\requirements.txt"

# reference, here is a list of Maya envars
# https://github.com/mottosso/Maya-Environment-Variables/blob/master/README.md
# https://www.regnareb.com/pro/2021/05/new-environment-variables-in-maya-2022/

OBJ_DCCSI_MAINMENU = 'O3deDCCsiMainMenu'
TAG_DCCSI_MAINMENU = 'O3DE'

# mimicing all values from: "DccScriptingInterface\Tools\Dev\Windows\Env_DCC_Maya.bat"

# dcc: Maya ENVAR constants

# a dccsi envar to set/override the maya version
ENVAR_MAYA_VERSION = 'MAYA_VERSION'
MAYA_VERSION = 2023

# a maya managed envar to set/override the project root
# Specifies the location of your project folder on startup.
ENVAR_MAYA_PROJECT = 'MAYA_PROJECT'
MAYA_PROJECT = None  # placeholder, we will populate with game project

# imported above, dccsi managed envars for dccsi local folders
# ENVAR_PATH_DCCSI_TOOLS_DCC_MAYA
# PATH_DCCSI_TOOLS_DCC_MAYA

# a maya managed envar for where maya is located
# The path for the Maya installation directory (this is default win install path)
ENVAR_MAYA_LOCATION = 'MAYA_LOCATION'
MAYA_LOCATION = f'{PATH_PROGRAMFILES_X64}\\Autodesk\\Maya{MAYA_VERSION}'

# dccsi managed envar for finding maya's bin folder
ENVAR_MAYA_BIN_PATH = 'MAYA_BIN_PATH'
MAYA_BIN_PATH = f'{MAYA_LOCATION}\\bin'

# some maya manged envars we are setting
# disbales Autodesk Customer Involvement Program (CIP), improves shutdown time
ENVAR_MAYA_DISABLE_CIP = 'MAYA_DISABLE_CIP'
MAYA_DISABLE_CIP = True

# disables maya Customer Error Reporting (CER)
ENVAR_MAYA_DISABLE_CER = 'MAYA_DISABLE_CER'
MAYA_DISABLE_CER = True

# disbales additional maya analytics
ENVAR_MAYA_DISABLE_CLIC_IPM = 'MAYA_DISABLE_CLIC_IPM'
MAYA_DISABLE_CLIC_IPM = True

# disables additional maya analytics
ENVAR_MAYA_DISABLE_ADP = 'MAYA_DISABLE_ADP'
MAYA_DISABLE_ADP = True

# enables/disables the maya boot console window
ENVAR_MAYA_NO_CONSOLE_WINDOW = 'MAYA_NO_CONSOLE_WINDOW'
# as a dev, I need the console for debugging
MAYA_NO_CONSOLE_WINDOW = 0

# in 2022 they disabled the output window by default
ENVAR_MAYA_SHOW_OUTPUT_WINDOW = 'MAYA_SHOW_OUTPUT_WINDOW'
MAYA_SHOW_OUTPUT_WINDOW = 1

# a dccsi managed enavr to enable/disable our callbacks at boot
ENVAR_DCCSI_MAYA_SET_CALLBACKS = 'DCCSI_MAYA_SET_CALLBACKS'
DCCSI_MAYA_SET_CALLBACKS = True

# dccsi managed envar for our local maya plugins path
ENVAR_DCCSI_MAYA_PLUG_IN_PATH = 'DCCSI_MAYA_PLUG_IN_PATH'
DCCSI_MAYA_PLUG_IN_PATH = f'{PATH_DCCSI_TOOLS_DCC_MAYA}\\plugins'

# maya managed enavr for dding plugin search paths
ENVAR_MAYA_PLUG_IN_PATH = 'MAYA_PLUG_IN_PATH'

# dccsi managed envar for our local maya shelf data
ENVAR_DCCSI_MAYA_SHELF_PATH = 'DCCSI_MAYA_SHELF_PATH'
DCCSI_MAYA_SHELF_PATH = f'{PATH_DCCSI_TOOLS_DCC_MAYA}\\Prefs\\Shelves'

# maya managed enavr for adding shelf paths
ENVAR_MAYA_SHELF_PATH = 'MAYA_SHELF_PATH'

# dccsi managed envar for our local icon images
ENVAR_DCCSI_MAYA_XBMLANGPATH = 'DCCSI_MAYA_XBMLANGPATH'
DCCSI_MAYA_XBMLANGPATH = f'{PATH_DCCSI_TOOLS_DCC_MAYA}\\Prefs\\Icons'

# maya managed enavr for adding icon search paths
ENVAR_XBMLANGPATH = 'XBMLANGPATH'

# dccsi managed envar for ou local mel scripts (please, do not use mel if you can avoid it)
ENVAR_DCCSI_MAYA_SCRIPT_MEL_PATH = 'DCCSI_MAYA_SCRIPT_MEL_PATH'
DCCSI_MAYA_SCRIPT_MEL_PATH = f'{PATH_DCCSI_TOOLS_DCC_MAYA}\\Scripts\\Mel'

# dccsi managed envar for ou local python scripts
ENVAR_DCCSI_MAYA_SCRIPT_PY_PATH = 'DCCSI_MAYA_SCRIPT_PY_PATH'
DCCSI_MAYA_SCRIPT_PY_PATH = f'{PATH_DCCSI_TOOLS_DCC_MAYA}\\Scripts\\Python'

# dccsi managed enavr for our local scripts root (userSetup.py lives here)
ENVAR_DCCSI_MAYA_SCRIPT_PATH = "DCCSI_MAYA_SCRIPT_PATH"
DCCSI_MAYA_SCRIPT_PATH = f'{PATH_DCCSI_TOOLS_DCC_MAYA}\\Scripts'

# maya managed envar for adding script search paths
ENVAR_MAYA_SCRIPT_PATH = 'MAYA_SCRIPT_PATH'

# a slug for the DX11 tag
SLUG_MAYA_DX11 = 'VirtualDeviceDx11'

# maya managed envar
# "MAYA_VP2_DEVICE_OVERRIDE" can be set to "VirtualDeviceDx11",
# "VirtualDeviceGLCore", or "VirtualDeviceGL"
ENVAR_MAYA_VP2_DEVICE_OVERRIDE = "MAYA_VP2_DEVICE_OVERRIDE"
MAYA_VP2_DEVICE_OVERRIDE = SLUG_MAYA_DX11

# maya managed envar
# "MAYA_OGS_DEVICE_OVERRIDE" can be set to "VirtualDeviceDx11"
# to override the Viewport 2.0 rendering engine.
ENVAR_MAYA_OGS_DEVICE_OVERRIDE = "MAYA_OGS_DEVICE_OVERRIDE"
MAYA_OGS_DEVICE_OVERRIDE = SLUG_MAYA_DX11

SLUG_O3DE_DCC_MAYA_MEL = 'dccsi_setup.mel'
SLUG_MAYA_WORKSPACE = 'workspace.mel'

# dccsi envar to find maya executable
SLUG_MAYA_EXE = 'maya.exe'
ENVAR_DCCSI_MAYA_EXE = 'DCCSI_MAYA_EXE'
DCCSI_MAYA_EXE = f'{MAYA_BIN_PATH}\\{SLUG_MAYA_EXE}'

# dccsi envar to get maya batch executable (headless)
SLUG_MAYABATCH_EXE = 'mayabatch.exe'
ENVAR_DCCSI_MAYABATCH_EXE = 'DCCSI_MAYABATCH_EXE'
DCCSI_MAYABATCH_EXE = f'{MAYA_BIN_PATH}\\{SLUG_MAYABATCH_EXE}'

# slug for mayapy.exe
SLUG_MAYAPY_EXE = 'mayapy.exe'

# a dccsi envar to store the path to mayapy.exe
# we can pass this to IDE's like wing to access as python launch interpreter
ENVAR_DCCSI_PY_MAYA = 'DCCSI_PY_MAYA'
DCCSI_PY_MAYA = f'{MAYA_BIN_PATH}\\{SLUG_MAYAPY_EXE}'

DCCSI_MAYA_WIKI_URL = 'https://github.com/o3de/o3de/wiki/O3DE-DCCsi-Tools-DCC-Maya'

_LOGGER.debug('{0} took: {1} sec'.format(_MODULENAME, timeit.default_timer() - _START))
# -------------------------------------------------------------------------
