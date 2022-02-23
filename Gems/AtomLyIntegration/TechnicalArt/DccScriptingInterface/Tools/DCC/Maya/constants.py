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

This module contains default values for commony used constants & strings.
We can make an update here easily that is propogated elsewhere.
"""
# -------------------------------------------------------------------------
# built-ins
from os.path import expanduser
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'Tools.DCC.Maya.constants'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Maya is frozen
#_MODULE_PATH = Path(__file__)
# https://tinyurl.com/y49t3zzn
# module path when frozen
_MODULE_PATH = Path(os.path.abspath(inspect.getfile(inspect.currentframe())))
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH))

# we need to set up basic access to the DCCsi
_PATH_DCCSI_TOOLS_MAYA = Path(_MODULE_PATH.parent)
_PATH_DCCSI_TOOLS_MAYA = Path(os.getenv('PATH_DCCSI_TOOLS_MAYA', _PATH_DCCSI_TOOLS_MAYA.as_posix()))
site.addsitedir(_PATH_DCCSI_TOOLS_MAYA.as_posix())

# we need to set up basic access to the DCCsi
_PATH_DCCSI_TOOLS_DCC = Path(_PATH_DCCSI_TOOLS_MAYA.parent)
_PATH_DCCSI_TOOLS_DCC = Path(os.getenv('PATH_DCCSI_TOOLS_DCC', _PATH_DCCSI_TOOLS_DCC.as_posix()))
site.addsitedir(_PATH_DCCSI_TOOLS_DCC.as_posix())

# we need to set up basic access to the DCCsi
_PATH_DCCSI_TOOLS = Path(_PATH_DCCSI_TOOLS_DCC.parent)
_PATH_DCCSI_TOOLS = Path(os.getenv('PATH_DCCSI_TOOLS', _PATH_DCCSI_TOOLS.as_posix()))

# we need to set up basic access to the DCCsi
_PATH_DCCSIG = Path(_PATH_DCCSI_TOOLS.parent)
_PATH_DCCSIG = Path(os.getenv('PATH_DCCSIG', _PATH_DCCSIG.as_posix()))
site.addsitedir(_PATH_DCCSIG.as_posix())

# now we have azpy api access
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.constants import * # but here are the specific ones we are gonna use
from azpy.constants import PATH_PROGRAMFILES_X64
from azpy.constants import PATH_USER_HOME
from azpy.constants import PATH_USER_O3DE
from azpy.constants import ENVAR_O3DE_DEV
from azpy.constants import PATH_O3DE_DEV
from azpy.constants import ENVAR_PATH_DCCSIG
from azpy.constants import PATH_DCCSIG
from azpy.constants import ENVAR_DCCSI_LOG_PATH
from azpy.constants import PATH_DCCSI_LOG_PATH
from azpy.constants import ENVAR_DCCSI_PY_VERSION_MAJOR
from azpy.constants import ENVAR_DCCSI_PY_VERSION_MINOR
from azpy.constants import ENVAR_PATH_DCCSI_PYTHON_LIB
from azpy.constants import PATH_DCCSI_PYTHON_LIB
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# dcc: Maya ENVAR constants
ENVAR_DCCSI_PY_VERSION_MAJOR=str("DCCSI_PY_VERSION_MAJOR")
ENVAR_DCCSI_PY_VERSION_MINOR=str("DCCSI_PY_VERSION_MINOR")
ENVAR_DCCSI_PY_VERSION_RELEASE=str("DCCSI_PY_VERSION_RELEASE")

ENVAR_MAYA_NO_CONSOLE_WINDOW = str("MAYA_NO_CONSOLE_WINDOW")
ENVAR_MAYA_SHOW_OUTPUT_WINDOW = str("MAYA_SHOW_OUTPUT_WINDOW")

TAG_O3DE_DCC_MAYA_MEL = 'dccsi_setup.mel'
TAG_MAYA_WORKSPACE = 'workspace.mel'

ENVAR_DCCSI_PY_MAYA = str('DCCSI_PY_MAYA')

ENVAR_MAYA_VERSION = str('MAYA_VERSION')
ENVAR_MAYA_LOCATION = str('MAYA_LOCATION')

ENVAR_PATH_DCCSI_TOOLS_MAYA = str('PATH_DCCSI_TOOLS_MAYA')
ENVAR_MAYA_MODULE_PATH = str('MAYA_MODULE_PATH')
ENVAR_MAYA_BIN_PATH = str('MAYA_BIN_PATH')

ENVAR_DCCSI_MAYA_PLUG_IN_PATH = str('DCCSI_MAYA_PLUG_IN_PATH')
ENVAR_MAYA_PLUG_IN_PATH = str('MAYA_PLUG_IN_PATH')

ENVAR_DCCSI_MAYA_SHELF_PATH = str('DCCSI_MAYA_SHELF_PATH')
ENVAR_MAYA_SHELF_PATH = str('MAYA_SHELF_PATH')

ENVAR_DCCSI_MAYA_XBMLANGPATH = str('DCCSI_MAYA_XBMLANGPATH')
ENVAR_XBMLANGPATH = str('XBMLANGPATH')

ENVAR_DCCSI_MAYA_SCRIPT_MEL_PATH = str('DCCSI_MAYA_SCRIPT_MEL_PATH')
ENVAR_DCCSI_MAYA_SCRIPT_PY_PATH = str('DCCSI_MAYA_SCRIPT_PY_PATH')
ENVAR_DCCSI_MAYA_SCRIPT_PATH = str("DCCSI_MAYA_SCRIPT_PATH")
ENVAR_MAYA_SCRIPT_PATH = str('MAYA_SCRIPT_PATH')

ENVAR_DCCSI_MAYA_SET_CALLBACKS = str('DCCSI_MAYA_SET_CALLBACKS')

ENVAR_MAYA_VP2_DEVICE_OVERRIDE=str("MAYA_VP2_DEVICE_OVERRIDE")
ENVAR_MAYA_OGS_DEVICE_OVERRIDE=str("MAYA_OGS_DEVICE_OVERRIDE")
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Maya consts
#USER_HOME = Path.home()
# mimicing all values from: "DccScriptingInterface\Tools\Dev\Windows\Env_DCC_Maya.bat"
# note: these are just default values, they are only initially CONST
# if/when imported from here (constants.py)
DCCSI_PY_VERSION_MAJOR   = 3
DCCSI_PY_VERSION_MINOR   = 7
DCCSI_PY_VERSION_RELEASE = 7

# not actually a maya envar, to do: could rename DCCSI_MAYA_VERSION
MAYA_VERSION=2022

# is a maya envar
MAYA_PROJECT = _PATH_DCCSIG.as_posix()

PATH_DCCSI_TOOLS_MAYA = _PATH_DCCSI_TOOLS_MAYA.as_posix()

# is a maya envar
MAYA_MODULE_PATH = _PATH_DCCSI_TOOLS_MAYA.as_posix()

# is a maya envar
MAYA_LOCATION = Path.joinpath(self) %ProgramFiles%\Autodesk\Maya%MAYA_VERSION%

# is a maya envar
MAYA_BIN_PATH=%MAYA_LOCATION%\bin

DCCSI_MAYA_SET_CALLBACKS = True

# is a maya envar
MAYA_NO_CONSOLE_WINDOW = False
MAYA_SHOW_OUTPUT_WINDOW = True

DCCSI_PY_MAYA=%MAYA_BIN_PATH%\mayapy.exe

DCCSI_MAYA_PLUG_IN_PATH=%PATH_DCCSI_TOOLS_MAYA%\plugins

# is a maya envar
MAYA_PLUG_IN_PATH=%DCCSI_MAYA_PLUG_IN_PATH%;%MAYA_PLUG_IN_PATH%

DCCSI_MAYA_SHELF_PATH=%PATH_DCCSI_TOOLS_MAYA%\Prefs\Shelves

DCCSI_MAYA_XBMLANGPATH=%PATH_DCCSI_TOOLS_MAYA%\Prefs\icons

# is a maya envar
# maya resources, very oddly named
XBMLANGPATH=%DCCSI_MAYA_XBMLANGPATH%;%XBMLANGPATH%

DCCSI_MAYA_SCRIPT_MEL_PATH=%PATH_DCCSI_TOOLS_MAYA%\Scripts\Mel

# is a maya envar
MAYA_SCRIPT_PATH=%DCCSI_MAYA_SCRIPT_MEL_PATH%;%MAYA_SCRIPT_PATH%

DCCSI_MAYA_SCRIPT_PY_PATH=%PATH_DCCSI_TOOLS_MAYA%\Scripts\Python

# is a maya envar
MAYA_SCRIPT_PATH=%DCCSI_MAYA_SCRIPT_PY_PATH%;%MAYA_SCRIPT_PATH%

DCCSI_MAYA_SCRIPT_PATH=%PATH_DCCSI_TOOLS_MAYA%\Scripts

# is a maya envar
MAYA_SCRIPT_PATH=%DCCSI_MAYA_SCRIPT_PATH%;%MAYA_SCRIPT_PATH%

# is a maya envar
MAYA_VP2_DEVICE_OVERRIDE="VirtualDeviceDx11"
MAYA_OGS_DEVICE_OVERRIDE="VirtualDeviceDx11"

# reference, here is a list of Maya envars
# https://github.com/mottosso/Maya-Environment-Variables/blob/master/README.md
# -------------------------------------------------------------------------        
        


# Note: we've developed and tested with Blender 3.0 (experimental)
# change at your own risk, we are just future proofing.
DCCSI_BLENDER_VERSION =   3.0
DCCSI_BLENDER_LOCATION =  f"C:/Program Files/Blender Foundation/Blender {DCCSI_BLENDER_VERSION}"

# I think this one will launch with a console
DCCSI_BLENDER_EXE =       f"{DCCSI_BLENDER_LOCATION}/blender.exe"

# this is the standard launcher that doesn't have
DCCSI_BLENDER_LAUNCHER =  f"{DCCSI_BLENDER_LOCATION}/blender-launcher.exe"

DCCSI_BLENDER_PYTHON =    f"{DCCSI_BLENDER_LOCATION}/{DCCSI_BLENDER_VERSION}/python"
DCCSI_BLENDER_PY_EXE =    f"{DCCSI_BLENDER_PYTHON}/bin/python.exe"

DCCSI_BLENDER_WIKI_URL =  'https://github.com/o3de/o3de/wiki/O3DE-DCCsi-Blender'

DCCSI_TOOLS_BLENDER_PATH = 'foo'

# -------------------------------------------------------------------------