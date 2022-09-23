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

from DccScriptingInterface import SETTINGS_FILE_SLUG
from DccScriptingInterface import LOCAL_SETTINGS_FILE_SLUG

from DccScriptingInterface.Tools import PATH_DCCSI_TOOLS
from DccScriptingInterface.Tools.DCC import PATH_DCCSI_TOOLS_DCC

from DccScriptingInterface.globals import *

_DCCSI_TOOLS_BLENDER_PATH = Path(_MODULE_PATH.parent)
add_site_dir(_DCCSI_TOOLS_BLENDER_PATH.as_posix())

# our dccsi location for substance designer <DCCsi>\Tools\DCC\Blender
ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER = "PATH_DCCSI_TOOLS_DCC_BLENDER"

# the path to this < dccsi >/Tools/IDE pkg
PATH_DCCSI_TOOLS_DCC_BLENDER = Path(_MODULE_PATH.parent)
PATH_DCCSI_TOOLS_DCC_BLENDER = Path(os.getenv(ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER,
                                              PATH_DCCSI_TOOLS_DCC_BLENDER.as_posix()))
add_site_dir(PATH_DCCSI_TOOLS_DCC_BLENDER.as_posix())
_LOGGER.debug(f'{ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER}: {PATH_DCCSI_TOOLS_DCC_BLENDER}')
_LOGGER.debug(STR_CROSSBAR)

# pulled from constants
PATH_DCCSI_TOOLS_DCC_BLENDER = _MODULE_PATH.parent
PATH_DCCSI_TOOLS_DCC = PATH_DCCSI_TOOLS_DCC_BLENDER.parent
PATH_DCCSI_TOOLS = PATH_DCCSI_TOOLS_DCC.parent

# from dynaconf import LazySettings

PATH_DCCSI_TOOLS_DCC_BLENDER_SETTINGS = PATH_DCCSI_TOOLS_DCC_BLENDER.joinpath(SETTINGS_FILE_SLUG).resolve()
PATH_DCCSI_TOOLS_DCC_BLENDER_LOCAL_SETTINGS = PATH_DCCSI_TOOLS_DCC_BLENDER.joinpath(LOCAL_SETTINGS_FILE_SLUG).resolve()

# put these here so we don't have to init config to get them
# default version
ENVAR_DCCSI_BLENDER_VERSION = "DCCSI_BLENDER_VERSION"
SLUG_DCCSI_BLENDER_VERSION = "3.1"

ENVAR_DCCSI_BLENDER_LOCATION = "PATH_DCCSI_BLENDER_LOCATION"
PATH_DCCSI_BLENDER_ROOT = f'C:\\Program Files\\Blender Foundation\\Blender'
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


# settings = LazySettings(
#     SETTINGS_FILE_FOR_DYNACONF=PATH_DCCSI_TOOLS_DCC_BLENDER_SETTINGS.as_posix(),
#     INCLUDES_FOR_DYNACONF=[PATH_DCCSI_TOOLS_DCC_BLENDER_LOCAL_SETTINGS.as_posix()]
# )
#
# # need to create a json validator for settings.local.json
# settings.setenv()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# suggestion would be to turn this into a method to reduce boilerplate
# but where to put it that makes sense?
if DCCSI_DEV_MODE:
    _LOGGER.debug(f'Testing Imports from {_PACKAGENAME}')

    # If in dev mode and test is flagged this will force imports of __all__
    # although slower and verbose, this can help detect cyclical import
    # failure and other issues

    # the DCCSI_TESTS flag needs to be properly added in .bat env
    if DCCSI_TESTS:
        from DccScriptingInterface.azpy import test_imports
        test_imports(_all=__all__,
                     _pkg=_PACKAGENAME,
                     _logger=_LOGGER)
# -------------------------------------------------------------------------
