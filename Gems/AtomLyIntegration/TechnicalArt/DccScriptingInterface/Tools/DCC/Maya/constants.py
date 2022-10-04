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
ENVAR_DCCSI_CONFIG_DCC_MAYA = 'DCCSI_CONFIG_DCC_MAYA'

# this is the shared default requirements.txt file to install for python 3.6.x+
DCCSI_PYTHON_REQUIREMENTS = Path(PATH_DCCSIG, 'requirements.txt')
# if using maya 2020 or less with py2.7 override with and use the one here:
# "DccScriptingInterface\Tools\DCC\Maya\requirements.txt"

from DccScriptingInterface import PATH_PROGRAMFILES_X64
from DccScriptingInterface import PATH_DCCSI_PYTHON_LIB
from DccScriptingInterface import O3DE_DEV

# reference, here is a list of Maya envars
# https://github.com/mottosso/Maya-Environment-Variables/blob/master/README.md

OBJ_DCCSI_MAINMENU = 'O3deDCCsiMainMenu'
TAG_DCCSI_MAINMENU = 'O3DE(DCCsi)'

# dcc: Maya ENVAR constants
ENVAR_DCCSI_PY_VERSION_MAJOR = "DCCSI_PY_VERSION_MAJOR"
ENVAR_DCCSI_PY_VERSION_MINOR = "DCCSI_PY_VERSION_MINOR"
ENVAR_DCCSI_PY_VERSION_RELEASE = "DCCSI_PY_VERSION_RELEASE"

ENVAR_MAYA_NO_CONSOLE_WINDOW = "MAYA_NO_CONSOLE_WINDOW"
ENVAR_MAYA_SHOW_OUTPUT_WINDOW = "MAYA_SHOW_OUTPUT_WINDOW"

TAG_O3DE_DCC_MAYA_MEL = 'dccsi_setup.mel'
TAG_MAYA_WORKSPACE = 'workspace.mel'

ENVAR_DCCSI_PY_MAYA = 'DCCSI_PY_MAYA'

ENVAR_MAYA_VERSION = 'MAYA_VERSION'
ENVAR_MAYA_LOCATION = 'MAYA_LOCATION'

ENVAR_MAYA_MODULE_PATH = 'MAYA_MODULE_PATH'
ENVAR_MAYA_BIN_PATH = 'MAYA_BIN_PATH'

ENVAR_DCCSI_MAYA_PLUG_IN_PATH = 'DCCSI_MAYA_PLUG_IN_PATH'
ENVAR_MAYA_PLUG_IN_PATH = 'MAYA_PLUG_IN_PATH'

ENVAR_DCCSI_MAYA_SHELF_PATH = 'DCCSI_MAYA_SHELF_PATH'
ENVAR_MAYA_SHELF_PATH = 'MAYA_SHELF_PATH'

ENVAR_DCCSI_MAYA_XBMLANGPATH = 'DCCSI_MAYA_XBMLANGPATH'
ENVAR_XBMLANGPATH = 'XBMLANGPATH'

ENVAR_DCCSI_MAYA_SCRIPT_MEL_PATH = 'DCCSI_MAYA_SCRIPT_MEL_PATH'
ENVAR_DCCSI_MAYA_SCRIPT_PY_PATH = 'DCCSI_MAYA_SCRIPT_PY_PATH'
ENVAR_DCCSI_MAYA_SCRIPT_PATH = "DCCSI_MAYA_SCRIPT_PATH"
ENVAR_MAYA_SCRIPT_PATH = 'MAYA_SCRIPT_PATH'

ENVAR_DCCSI_MAYA_SET_CALLBACKS = 'DCCSI_MAYA_SET_CALLBACKS'

ENVAR_MAYA_VP2_DEVICE_OVERRIDE = "MAYA_VP2_DEVICE_OVERRIDE"
ENVAR_MAYA_OGS_DEVICE_OVERRIDE = "MAYA_OGS_DEVICE_OVERRIDE"

# mimicing all values from: "DccScriptingInterface\Tools\Dev\Windows\Env_DCC_Maya.bat"
DCCSI_PY_VERSION_MAJOR = 3
DCCSI_PY_VERSION_MINOR = 7
DCCSI_PY_VERSION_RELEASE = 7

# not actually a maya envar, to do: could rename DCCSI_MAYA_VERSION
MAYA_VERSION=2022

# is a maya envar
MAYA_PROJECT = PATH_DCCSIG.as_posix()


# is a maya envar
MAYA_MODULE_PATH = PATH_DCCSI_TOOLS_DCC_MAYA.as_posix()

# is a maya envar
MAYA_LOCATION = Path(PATH_PROGRAMFILES_X64,
                     'Autodesk',
                     f'Maya{PATH_DCCSI_TOOLS_DCC_MAYA}').as_posix()

# is a maya envar
MAYA_BIN_PATH = Path(MAYA_LOCATION, 'bin').as_posix()

DCCSI_MAYA_SET_CALLBACKS = True

# is a maya envar
MAYA_NO_CONSOLE_WINDOW = False
MAYA_SHOW_OUTPUT_WINDOW = True

DCCSI_MAYA_EXE = Path(MAYA_BIN_PATH, 'maya.exe')

DCCSI_MAYABATCH_EXE = Path(MAYA_BIN_PATH, 'mayabatch.exe')

DCCSI_PY_MAYA = Path(MAYA_BIN_PATH, 'mayapy.exe')

# this is transient and will always track the exe this script is executing on
O3DE_PY_EXE = Path(sys.executable).as_posix()
DCCSI_PY_IDE = Path(DCCSI_PY_MAYA).as_posix()

DCCSI_MAYA_PLUG_IN_PATH = Path(PATH_DCCSI_TOOLS_DCC_MAYA, 'plugins').as_posix()

# is a maya envar
MAYA_PLUG_IN_PATH = Path(DCCSI_MAYA_PLUG_IN_PATH).as_posix() # extend %MAYA_PLUG_IN_PATH%
# to do: remove or extend next PR, technically there can be more then one plugin path
#while MAYA_PLUG_IN_PATH:
    #if ENVAR_MAYA_PLUG_IN_PATH in os.environ:
        #maya_plug_pathlist = os.getenv(ENVAR_MAYA_PLUG_IN_PATH).split(os.pathsep)
        #maya_plug_new_pathlist = maya_plug_pathlist.copy()
        #maya_plug_new_pathlist.insert(0, Path(DCCSI_MAYA_PLUG_IN_PATH).as_posix())
        #os.environ[ENVAR_MAYA_PLUG_IN_PATH] = os.pathsep.join(maya_plug_new_pathlist)
    #else:
        #os.environ[ENVAR_MAYA_PLUG_IN_PATH] = DCCSI_MAYA_PLUG_IN_PATH

    #MAYA_PLUG_IN_PATH = os.getenv(ENVAR_MAYA_PLUG_IN_PATH, "< NOT SET >")
    #break

DCCSI_MAYA_SHELF_PATH = Path(PATH_DCCSI_TOOLS_DCC_MAYA, 'Prefs', 'Shelves').as_posix()

DCCSI_MAYA_XBMLANGPATH = Path(PATH_DCCSI_TOOLS_DCC_MAYA, 'Prefs', 'icons').as_posix()

# is a maya envar
# maya resources, very oddly named
XBMLANGPATH = Path(DCCSI_MAYA_XBMLANGPATH).as_posix() # extend %XBMLANGPATH%
# to do: remove or extend next PR, technically there can be more then one resource path specified
#while XBMLANGPATH:
    #if ENVAR_XBMLANGPATH in os.environ:
        #maya_xbm_pathlist = os.getenv(ENVAR_XBMLANGPATH).split(os.pathsep)
        #maya_xbm_new_pathlist = maya_xbm_pathlist.copy()
        #maya_xbm_new_pathlist.insert(0, Path(DCCSI_MAYA_XBMLANGPATH).as_posix())
        #os.environ[ENVAR_XBMLANGPATH] = os.pathsep.join(maya_xbm_new_pathlist)
    #else:
        #os.environ[ENVAR_XBMLANGPATH] = DCCSI_MAYA_XBMLANGPATH

    #XBMLANGPATH = os.getenv(ENVAR_XBMLANGPATH, "< NOT SET >")
    #break

DCCSI_MAYA_SCRIPT_PATH = Path(PATH_DCCSI_TOOLS_DCC_MAYA, 'Scripts').as_posix()
DCCSI_MAYA_SCRIPT_MEL_PATH = Path(PATH_DCCSI_TOOLS_DCC_MAYA, 'Scripts', 'Mel').as_posix()
DCCSI_MAYA_SCRIPT_PY_PATH = Path(PATH_DCCSI_TOOLS_DCC_MAYA, 'Scripts', 'Python').as_posix()

MAYA_SCRIPT_PATH = Path(DCCSI_MAYA_SCRIPT_PATH).as_posix() # extend %MAYA_SCRIPT_PATH%
# to do: remove or extend next PR, technically there can be more then one script path specified
#while MAYA_SCRIPT_PATH:
    #if ENVAR_MAYA_SCRIPT_PATH in os.environ:
        #maya_script_pathlist = os.getenv(ENVAR_MAYA_SCRIPT_PATH).split(os.pathsep)
        #maya_script_new_pathlist = maya_script_pathlist.copy()
        #maya_script_new_pathlist.insert(0, DCCSI_MAYA_SCRIPT_MEL_PATH)
        #maya_script_new_pathlist.insert(0, DCCSI_MAYA_SCRIPT_PY_PATH)
        #maya_script_new_pathlist.insert(0, DCCSI_MAYA_SCRIPT_PATH)
        #os.environ[ENVAR_MAYA_SCRIPT_PATH] = os.pathsep.join(maya_script_new_pathlist)
    #else:
        #os.environ[ENVAR_MAYA_SCRIPT_PATH] = os.pathsep.join( (DCCSI_MAYA_SCRIPT_PATH,
                                                               #DCCSI_MAYA_SCRIPT_PY_PATH,
                                                               #DCCSI_MAYA_SCRIPT_MEL_PATH) )

    #MAYA_SCRIPT_PATH = os.getenv(ENVAR_MAYA_SCRIPT_PATH, "< NOT SET >")
    #break

# is a maya envar
MAYA_VP2_DEVICE_OVERRIDE="VirtualDeviceDx11"
MAYA_OGS_DEVICE_OVERRIDE="VirtualDeviceDx11"

DCCSI_MAYA_WIKI_URL =  'https://github.com/o3de/o3de/wiki/O3DE-DCCsi-Tools-DCC-Maya'

_LOGGER.debug('{0} took: {1} sec'.format(_MODULENAME, timeit.default_timer() - _START))
# -------------------------------------------------------------------------
