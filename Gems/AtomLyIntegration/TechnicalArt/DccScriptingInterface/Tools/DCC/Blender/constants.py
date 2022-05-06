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
    < DCCsi >:: Tools//DCC//Blender//constants.py

This module contains default values for commony used constants & strings.
We can make an update here easily that is propogated elsewhere.
"""
# -------------------------------------------------------------------------
# built-ins
import sys
import os
import site
import timeit
import inspect
from os.path import expanduser
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_START = timeit.default_timer() # start tracking

# global scope
_MODULENAME = 'Tools.DCC.Blender.constants'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Blender consts
USER_HOME = Path.home()

_MODULE_PATH = Path(os.path.abspath(inspect.getfile(inspect.currentframe())))
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH))

_PATH_DCCSI_TOOLS_BLENDER = Path(_MODULE_PATH.parent)
_PATH_DCCSI_TOOLS_BLENDER = Path(os.getenv('PATH_DCCSI_TOOLS_BLENDER',
                                           _PATH_DCCSI_TOOLS_BLENDER.as_posix()))

_PATH_DCCSI_TOOLS_DCC = Path(_PATH_DCCSI_TOOLS_BLENDER.parent)
_PATH_DCCSI_TOOLS_DCC = Path(os.getenv('PATH_DCCSI_TOOLS_DCC',
                                       _PATH_DCCSI_TOOLS_DCC.as_posix()))

_PATH_DCCSI_TOOLS = Path(_PATH_DCCSI_TOOLS_DCC.parent)
_PATH_DCCSI_TOOLS = Path(os.getenv('PATH_DCCSI_TOOLS',
                                   _PATH_DCCSI_TOOLS.as_posix()))

# we need to set up basic access to the DCCsi
_PATH_DCCSIG = Path(_PATH_DCCSI_TOOLS.parent)
_PATH_DCCSIG = Path(os.getenv('PATH_DCCSIG', _PATH_DCCSIG.as_posix()))
site.addsitedir(_PATH_DCCSIG.as_posix())

_LOGGER.debug('_PATH_DCCSIG: {}'.format(_PATH_DCCSIG.as_posix()))

# now we have azpy api access
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.constants import * # but here are the specific ones we may use
from azpy.constants import PATH_PROGRAMFILES_X64
from azpy.constants import TAG_PY_MAJOR
from azpy.constants import TAG_PY_MINOR
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
from azpy.constants import STR_PATH_DCCSI_PYTHON_LIB
from azpy.constants import PATH_DCCSI_PYTHON_LIB
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Note: we've developed and tested with Blender 3.0 (experimental)
# change at your own risk, we are just future proofing.
DCCSI_BLENDER_VERSION = 3.0
DCCSI_BLENDER_LOCATION = f"C:/Program Files/Blender Foundation/Blender {DCCSI_BLENDER_VERSION}"

# I think this one will launch with a console
DCCSI_BLENDER_EXE = f"{DCCSI_BLENDER_LOCATION}/blender.exe"

# this is the standard launcher that doesn't have
DCCSI_BLENDER_LAUNCHER = f"{DCCSI_BLENDER_LOCATION}/blender-launcher.exe"

DCCSI_BLENDER_PYTHON = f"{DCCSI_BLENDER_LOCATION}/{DCCSI_BLENDER_VERSION}/python"
DCCSI_BLENDER_PY_EXE = f"{DCCSI_BLENDER_PYTHON}/bin/python.exe"

DCCSI_BLENDER_WIKI_URL = 'https://github.com/o3de/o3de/wiki/O3DE-DCCsi-Tools-DCC-Blender'
# -------------------------------------------------------------------------