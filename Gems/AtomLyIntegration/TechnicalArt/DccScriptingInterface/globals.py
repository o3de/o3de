#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! dccsi global state module
:file: < DCCsi >/globals.py
:Status: Prototype
:Version: 0.0.1
"""
# -------------------------------------------------------------------------
# standard imports
import os
import sys
import site
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface import _PACKAGENAME, STR_CROSSBAR
_MODULENAME = f'{_PACKAGENAME}.globals'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
_MODULE_PATH = Path(__file__) # this module should not be used as an entry
# -------------------------------------------------------------------------
# global state to be shared
from DccScriptingInterface import ENVAR_PATH_DCCSIG

# the default folder path location for the dccsi gem
# pull the default path
from DccScriptingInterface import PATH_DCCSIG

# this allows env to override the folder path location externally
PATH_DCCSIG = Path(os.getenv(ENVAR_PATH_DCCSIG, PATH_DCCSIG)).resolve()
sys.path.append(PATH_DCCSIG.as_posix())
site.addsitedir(PATH_DCCSIG.as_posix())
_LOGGER.debug(f'{ENVAR_PATH_DCCSIG}: {PATH_DCCSIG.as_posix()}') # debug tracking

# propagate the lib bootstrap folder path
# ensure package dependencies are accessible, by python sys version
# other pkgs and modules may depend on them
# this is transient (per-session) so we don't override in env
from DccScriptingInterface import PATH_DCCSI_PYTHON_LIB
PATH_DCCSI_PYTHON_LIB = Path(PATH_DCCSI_PYTHON_LIB).resolve()
site.addsitedir(PATH_DCCSI_PYTHON_LIB.as_posix())

# propagate the o3de engine root
from DccScriptingInterface import ENVAR_O3DE_DEV
from DccScriptingInterface import O3DE_DEV
# this allows env to override the folder path location externally
O3DE_DEV = Path(os.getenv(ENVAR_O3DE_DEV, O3DE_DEV)).resolve()
sys.path.append(O3DE_DEV.as_posix())
site.addsitedir(O3DE_DEV.as_posix())
_LOGGER.debug(f'{ENVAR_O3DE_DEV}: {O3DE_DEV.as_posix()}') # debug tracking

from DccScriptingInterface import DCCSI_STRICT

from DccScriptingInterface.constants import ENVAR_DCCSI_GDEBUG
from DccScriptingInterface.constants import ENVAR_DCCSI_DEV_MODE
from DccScriptingInterface.constants import ENVAR_DCCSI_GDEBUGGER
from DccScriptingInterface.constants import ENVAR_DCCSI_LOGLEVEL
from DccScriptingInterface.constants import ENVAR_DCCSI_TESTS
from DccScriptingInterface.constants import DCCSI_SETTINGS_LOCAL_FILENAME
from DccScriptingInterface.constants import PATH_DCCSI_LOG_PATH

from DccScriptingInterface.constants import STR_CROSSBAR
from DccScriptingInterface.constants import FRMT_LOG_LONG

# sub pkg dccsi imports
# in next iteration these should be refactored to:
# from DccScriptingInterface.azpy import foo
from DccScriptingInterface.azpy.env_bool import env_bool

# global state init and storage, can be overridden from external env
# retrieve the dccsi global debug flag
# this adds additional debug behavior and inspection, more verbose
DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)

# suggestion, it might be an improvement to pack all of these into
# a single map/dict using a dot-notation Box so that you could do this:
#
# from DccScriptingInterface.globals import global_state
# if global_state.DCCSI_DEV_MODE: print('foo')
#
# instead of this:
#
# from DccScriptingInterface.globals import *
# if DCCSI_DEV_MODE: print('foo')

# retrieve the dccsi global developer mode flag
# enables early auto-debugger attachment
DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

# retrieve the dccsi global ide debugger type
# Wing Pro 8 is currently the only one set up
DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')

# retrieve the dccsi global log level
# you can for example enable debug logging without global debug or dev mode flags
DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))

# retrieve the dccsi global test mode flag
# when enabled additional internal tests are run, like forcing cascaded imports
DCCSI_TESTS = env_bool(ENVAR_DCCSI_TESTS, False)

if DCCSI_GDEBUG or DCCSI_DEV_MODE:
    DCCSI_LOGLEVEL = _logging.DEBUG
    _LOGGER.setLevel(DCCSI_LOGLEVEL) # throttle up help

# configure basic logger
# note: not using a common logger to reduce cyclical imports
_logging.basicConfig(level=DCCSI_LOGLEVEL,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)

# default settings file path
DCCSI_SETTINGS_LOCAL_PATH = Path(DCCSI_SETTINGS_LOCAL_FILENAME).resolve()

# default / temp log path
DCCSI_O3DE_USER_HOME_LOG = Path(PATH_DCCSI_LOG_PATH).resolve()

# putting these here, allows us to pull them from globals
# and reduce boilerplate in other modules
_LOGGER.debug(f'This MODULE_PATH: {_MODULE_PATH}')
_LOGGER.debug(f'Default {ENVAR_PATH_DCCSIG}: {PATH_DCCSIG}') # debug tracking
_LOGGER.debug(f'Default {ENVAR_O3DE_DEV}: {O3DE_DEV}')
_LOGGER.debug(f'{ENVAR_DCCSI_GDEBUG}: {DCCSI_GDEBUG}')
_LOGGER.debug(f'{ENVAR_DCCSI_DEV_MODE}: {DCCSI_DEV_MODE}')
_LOGGER.debug(f'{ENVAR_DCCSI_GDEBUGGER}: {DCCSI_GDEBUGGER}')
_LOGGER.debug(f'{ENVAR_DCCSI_LOGLEVEL}: {DCCSI_LOGLEVEL}')
_LOGGER.debug(f'{ENVAR_DCCSI_TESTS}: {DCCSI_TESTS}')
_LOGGER.debug(STR_CROSSBAR)

###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run as main, perform additional debug and module tests"""

    # some simple logger tests
    # evoke the filehandlers and test writing to the log file
    if DCCSI_GDEBUG:
        _LOGGER.debug('Forced Info! for {0}.'.format({_MODULENAME}))
        _LOGGER.error('Forced ERROR! for {0}.'.format({_MODULENAME}))

    pass
