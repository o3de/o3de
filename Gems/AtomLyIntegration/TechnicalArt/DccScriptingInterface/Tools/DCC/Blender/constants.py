#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""Common constants for the dccsi blender dcc app integration

:file: DccScriptingInterface\\Tools\\DCC\\Blender\\constants.py
:Status: Prototype
:Version: 0.0.1

# :Notice: we've developed and tested with Blender 3.0 (experimental)
# change at your own risk, we are just future proofing.
"""
# -------------------------------------------------------------------------
import timeit
_MODULE_START = timeit.default_timer()  # start tracking

# standard imports
import sys
import os
import site
import timeit
import inspect
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.Tools.DCC.Blender import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.constants'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')
_MODULE_PATH = Path(__file__)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# dccsi imports here
from DccScriptingInterface.constants import USER_HOME
from DccScriptingInterface.Tools.DCC.Blender import PATH_DCCSI_TOOLS_DCC_BLENDER
from DccScriptingInterface.Tools.DCC.Blender import ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER

from DccScriptingInterface.constants import ENVAR_PATH_DCCSI_TOOLS_DCC
from DccScriptingInterface.constants import ENVAR_PATH_DCCSI_TOOLS
ENVAR_DCCSI_CONFIG_DCC_BLENDER = 'DCCSI_CONFIG_DCC_BLENDER'

from DccScriptingInterface.Tools.DCC.Blender import ENVAR_DCCSI_BLENDER_VERSION
from DccScriptingInterface.Tools.DCC.Blender import SLUG_DCCSI_BLENDER_VERSION

ENVAR_DCCSI_BLENDER_PROJECT = "DCCSI_BLENDER_PROJECT"  # project name

from DccScriptingInterface.Tools.DCC.Blender import ENVAR_DCCSI_BLENDER_LOCATION
from DccScriptingInterface.Tools.DCC.Blender import PATH_DCCSI_BLENDER_ROOT
from DccScriptingInterface.Tools.DCC.Blender import PATH_DCCSI_BLENDER_LOCATION

from DccScriptingInterface.Tools.DCC.Blender import ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS
from DccScriptingInterface.Tools.DCC.Blender import PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS

from DccScriptingInterface.Tools.DCC.Blender import PATH_DCCSI_TOOLS_DCC_BLENDER_SETTINGS
from DccScriptingInterface.Tools.DCC.Blender import PATH_DCCSI_TOOLS_DCC_BLENDER_LOCAL_SETTINGS

# I think this one will launch with a console
SLUG_BLENDER_EXE = "blender.exe"
ENVAR_PATH_DCCSI_BLENDER_EXE = "PATH_DCCSI_BLENDER_EXE"
PATH_DCCSI_BLENDER_EXE = f"{PATH_DCCSI_BLENDER_LOCATION}\\{SLUG_BLENDER_EXE}"

# this is the standard launcher that prevents the command window from popping up on start
# https://developer.blender.org/rBf3944cf503966a93a124e389d9232d7f833c0077
SLUG_BLENDER_LAUNCHER_EXE = "blender-launcher.exe"
ENVAR_DCCSI_BLENDER_LAUNCHER_EXE = "DCCSI_BLENDER_LAUNCHER_EXE"
PATH_DCCSI_BLENDER_LAUNCHER_EXE = f'{PATH_DCCSI_BLENDER_LOCATION}\\{SLUG_BLENDER_LAUNCHER_EXE}'

# know where to find Blenders python if we ever need it
ENVAR_DCCSI_BLENDER_PYTHON_LOC = "PATH_DCCSI_BLENDER_PYTHON_LOC"
PATH_DCCSI_BLENDER_PYTHON_LOC = f"{PATH_DCCSI_BLENDER_LOCATION}\\{SLUG_DCCSI_BLENDER_VERSION}/python"

ENVAR_DCCSI_BLENDER_PY_EXE = "DCCSI_BLENDER_PY_EXE"
PATH_DCCSI_BLENDER_PY_EXE = f'{PATH_DCCSI_BLENDER_PYTHON_LOC}\\bin\\python.exe'

# our dccsi default start up script for blender aka bootstrap
from DccScriptingInterface.Tools.DCC.Blender import SLUG_DCCSI_BLENDER_BOOTSTRAP
from DccScriptingInterface.Tools.DCC.Blender import ENVAR_PATH_DCCSI_BLENDER_BOOTSTRAP
from DccScriptingInterface.Tools.DCC.Blender import PATH_DCCSI_BLENDER_BOOTSTRAP

# set up user scripts envar for Blender: https://blenderartists.org/t/addons-environment-variable-linux/603145/2
ENVAR_BLENDER_USER_SCRIPTS = 'BLENDER_USER_SCRIPTS'

ENVAR_URL_DCCSI_BLENDER_WIKI = 'URL_DCCSI_BLENDER_WIKI'
URL_DCCSI_BLENDER_WIKI = 'https://github.com/o3de/o3de/wiki/O3DE-DCCsi-Tools-DCC-Blender'
# -------------------------------------------------------------------------
