#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! TThis init allows us to treat Blender setup as a DCCsi tools python package

:file: DccScriptingInterface\\Tools\\DCC\\Blender\\__init__.py
:Status: Prototype
:Version: 0.0.1
:Future: is unknown
:Notice:
"""

# -------------------------------------------------------------------------
# standard imports
import os
import site
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.Tools.DCC import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.Blender'

__all__ = ['config',
           'constants',
           'start',
           'scripts',
           'discovery']

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# set up access to this Blender folder as a pkg
_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')

# last two parents
from DccScriptingInterface import add_site_dir

from DccScriptingInterface import STR_CROSSBAR
from DccScriptingInterface import SETTINGS_FILE_SLUG
from DccScriptingInterface import LOCAL_SETTINGS_FILE_SLUG

from DccScriptingInterface.Tools import PATH_DCCSI_TOOLS
from DccScriptingInterface.Tools.DCC import PATH_DCCSI_TOOLS_DCC

from DccScriptingInterface.globals import *

# our dccsi location for <DCCsi>\Tools\DCC\Blender
ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER = "PATH_DCCSI_TOOLS_DCC_BLENDER"

# the path to this < dccsi >/Tools/DCC pkg
PATH_DCCSI_TOOLS_DCC_BLENDER = Path(_MODULE_PATH.parent)
PATH_DCCSI_TOOLS_DCC_BLENDER = Path(os.getenv(ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER,
                                              PATH_DCCSI_TOOLS_DCC_BLENDER.as_posix()))
add_site_dir(PATH_DCCSI_TOOLS_DCC_BLENDER.as_posix())
_LOGGER.debug(f'{ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER}: {PATH_DCCSI_TOOLS_DCC_BLENDER}')
_LOGGER.debug(STR_CROSSBAR)

PATH_DCCSI_TOOLS_DCC_BLENDER = _MODULE_PATH.parent
PATH_DCCSI_TOOLS_DCC = PATH_DCCSI_TOOLS_DCC_BLENDER.parent
PATH_DCCSI_TOOLS = PATH_DCCSI_TOOLS_DCC.parent

PATH_DCCSI_TOOLS_DCC_BLENDER_SETTINGS = PATH_DCCSI_TOOLS_DCC_BLENDER.joinpath(SETTINGS_FILE_SLUG).resolve()
PATH_DCCSI_TOOLS_DCC_BLENDER_LOCAL_SETTINGS = PATH_DCCSI_TOOLS_DCC_BLENDER.joinpath(LOCAL_SETTINGS_FILE_SLUG).resolve()

# default version
ENVAR_DCCSI_BLENDER_VERSION = "DCCSI_BLENDER_VERSION"
SLUG_DCCSI_BLENDER_VERSION = "3.1"

ENVAR_DCCSI_BLENDER_LOCATION = "PATH_DCCSI_BLENDER_LOCATION"
PATH_DCCSI_BLENDER_ROOT = f'C:\\Program Files\\Blender Foundation\\Blender' # default
PATH_DCCSI_BLENDER_LOCATION = f'{PATH_DCCSI_BLENDER_ROOT} {SLUG_DCCSI_BLENDER_VERSION}'

ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS = "PATH_DCCSI_BLENDER_SCRIPTS"
PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS = f'{PATH_DCCSI_TOOLS_DCC_BLENDER}\\scripts'

# I think this one will launch with a console
SLUG_BLENDER_EXE = "blender.exe"
ENVAR_PATH_DCCSI_BLENDER_EXE = "PATH_DCCSI_BLENDER_EXE"
PATH_DCCSI_BLENDER_EXE = f"{PATH_DCCSI_BLENDER_LOCATION}\\{SLUG_BLENDER_EXE}"

# our dccsi default start up script for blender aka bootstrap
SLUG_DCCSI_BLENDER_BOOTSTRAP = "bootstrap.py"
ENVAR_PATH_DCCSI_BLENDER_BOOTSTRAP = "PATH_DCCSI_BLENDER_BOOTSTRAP"
PATH_DCCSI_BLENDER_BOOTSTRAP = f'{PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS}\\{SLUG_DCCSI_BLENDER_BOOTSTRAP}'
