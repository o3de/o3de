# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# --------------------------------------------------------------------------
"""! @brief

<DCCsi>/__init__.py

Allow DccScriptingInterface Gem to be the parent python pkg
"""
dccsi_info = {
    'name': 'O3DE_DCCSI_GEM',
    "description": "DccScriptingInterface",
    'status': 'prototype',
    'version': (0, 0, 1),
    'platforms': ({
        'include': ('win'),   # windows
        'exclude': ('darwin', # mac
                    'linux')  # linux, linux_x64
    }),
    "doc_url": "https://github.com/o3de/o3de/blob/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/readme.md"
}
# -------------------------------------------------------------------------
# standard imports
import os
import sys
import site
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------
# global scope
_PACKAGENAME = 'DCCsi'

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

__all__ = ['globals', # global state module
           'config', # dccsi core config.py
           'constants', # global dccsi constants
           'foundation', # set up dependancy pkgs for DCC tools
           'return_sys_version', # util
           'azpy', # shared pure python api
           'Editor', # O3DE editor scripts
           'Tools' # DCC and IDE tool integrations
           ]

# we need to set up basic access to the DCCsi
_MODULE_PATH = Path(__file__)

# add gems parent, dccsi lives under:
#  < o3de >\Gems\AtomLyIntegration\TechnicalArt
_PATH_O3DE_TECHART_GEMS = _MODULE_PATH.parents[1].resolve()
sys.path.append(_PATH_O3DE_TECHART_GEMS)
site.addsitedir(_PATH_O3DE_TECHART_GEMS)

# there may be other TechArt gems in the future
# or the dccsi maybe split up

# pulling from this __init__.py module, may cause some cyclical import
# problems. So we may need to move the logical parts to a sub pkg/modula

# -------------------------------------------------------------------------
# global constants here temporarily, to be moved to constants.py
# None

# imported global constants
from DccScriptingInterface.constants import ENVAR_PATH_DCCSIG
# the default folder path location for the dccsi gem
_PATH_DCCSIG = _MODULE_PATH.parents[0].resolve()
# allows env to override the folder path location externally
_PATH_DCCSIG = Path(os.getenv(ENVAR_PATH_DCCSIG, _PATH_DCCSIG)).resolve()
sys.path.append(_PATH_DCCSIG)
site.addsitedir(_PATH_DCCSIG)
_LOGGER.debug(f'{ENVAR_PATH_DCCSIG}: {_PATH_DCCSIG}') # debug tracking

# ensure package dependancies are accessible, by python sys version
# other pkgs and modules may depend on them
from DccScriptingInterface.constants import PATH_DCCSI_PYTHON_LIB
PATH_DCCSI_PYTHON_LIB = Path(PATH_DCCSI_PYTHON_LIB).resolve()
site.addsitedir(_PATH_DCCSIG)

from DccScriptingInterface.constants import ENVAR_DCCSI_GDEBUG
from DccScriptingInterface.constants import ENVAR_DCCSI_DEV_MODE
from DccScriptingInterface.constants import ENVAR_DCCSI_GDEBUGGER
from DccScriptingInterface.constants import ENVAR_DCCSI_LOGLEVEL
from DccScriptingInterface.constants import ENVAR_DCCSI_TESTS
from DccScriptingInterface.constants import DCCSI_SETTINGS_LOCAL_FILENAME
from DccScriptingInterface.constants import PATH_DCCSI_LOG_PATH

# sub pkg dccsi imports
# in next iteration these should be refactored to:
# from DccScriptingInterface.azpy import foo
from azpy import test_imports
from azpy.env_bool import env_bool
from azpy.config_utils import attach_debugger

# global state init and storage, can be overriden from external env
# retreive the dccsi global debug flag
# this adds additional debug behaviour and inspection, more verbose
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)

# retreive the dccsi global developer mode flag
# enables early auto-debugger attachment
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

# retreive the dccsi global ide debugger type
# Wing Pro 8 is currently the only one set up
_DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')

# retreive the dccsi global log level
# you can for example enable debug logging without global debug or dev mode flags
_DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))

# retreive the dccsi global test mode flag
# when enabled additional interal tests are run, like forcing cascaded imports
_DCCSI_TESTS = env_bool(ENVAR_DCCSI_TESTS, False)

if _DCCSI_GDEBUG and _DCCSI_DEV_MODE:
    _DCCSI_LOGLEVEL = _logging.DEBUG
    _LOGGER.setLevel(_DCCSI_LOGLEVEL) # throttle up help

if _DCCSI_DEV_MODE:
    # if dev mode, this will attemp to auto-attach the debugger
    # at the earliest possible point in this module
    attach_debugger(debugger_type=_DCCSI_GDEBUGGER)

    _LOGGER.debug(f'Testing Imports from {_PACKAGENAME}')

    # If in dev mode and test this will force imports of __all__
    # although slower and verbose, this can help detect cyclical import
    # failure and other issues
    if _DCCSI_TESTS:
        test_imports(_all=__all__,
                     _pkg=_PACKAGENAME,
                     _logger=_LOGGER)

###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run as main, perform additional debug and module tests"""
    pass






