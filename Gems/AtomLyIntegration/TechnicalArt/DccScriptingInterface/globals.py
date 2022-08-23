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
_MODULENAME = 'DCCsi.globals'

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.info('Initializing: {0}.'.format({_MODULENAME}))
_MODULE_PATH = Path(__file__) # thos module should not be used as an entry
# -------------------------------------------------------------------------
# global state to be shared
from DccScriptingInterface.constants import ENVAR_PATH_DCCSIG
# the default folder path location for the dccsi gem
PATH_DCCSIG = _MODULE_PATH.parents[0].resolve()
# allows env to override the folder path location externally
PATH_DCCSIG = Path(os.getenv(ENVAR_PATH_DCCSIG, PATH_DCCSIG)).resolve()
sys.path.append(PATH_DCCSIG)
site.addsitedir(PATH_DCCSIG)
_LOGGER.debug(f'{ENVAR_PATH_DCCSIG}: {PATH_DCCSIG}') # debug tracking

# ensure package dependancies are accessible, by python sys version
# other pkgs and modules may depend on them
from DccScriptingInterface.constants import PATH_DCCSI_PYTHON_LIB
PATH_DCCSI_PYTHON_LIB = Path(PATH_DCCSI_PYTHON_LIB).resolve()
site.addsitedir(PATH_DCCSIG)

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
from azpy.env_bool import env_bool

# global state init and storage, can be overriden from external env
# retreive the dccsi global debug flag
# this adds additional debug behaviour and inspection, more verbose
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

# retreive the dccsi global developer mode flag
# enables early auto-debugger attachment
DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

# retreive the dccsi global ide debugger type
# Wing Pro 8 is currently the only one set up
DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')

# retreive the dccsi global log level
# you can for example enable debug logging without global debug or dev mode flags
DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))

# retreive the dccsi global test mode flag
# when enabled additional interal tests are run, like forcing cascaded imports
DCCSI_TESTS = env_bool(ENVAR_DCCSI_TESTS, False)

if DCCSI_GDEBUG and DCCSI_DEV_MODE:
    DCCSI_LOGLEVEL = _logging.DEBUG
    _LOGGER.setLevel(DCCSI_LOGLEVEL) # throttle up help

# configure basic logger
# note: not using a common logger to reduce cyclical imports
_logging.basicConfig(level=DCCSI_LOGLEVEL,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)

# resolve the O3DE root
# a suggestion would be for us to refactor from _O3DE_DEV to _O3DE_ROOT
# dev is a legacy Lumberyard concept, as the engine snadbox was /dev
O3DE_DEV = PATH_DCCSIG.parents[4].resolve()

# default settings file path
DCCSI_SETTIBGS_LOCAL_PATH = Path(DCCSI_SETTINGS_LOCAL_FILENAME).resolve()

# default / temp log path
DCCSI_O3DE_USER_HOM_LOG = Path(PATH_DCCSI_LOG_PATH).resolve()

# putting these here, allows us to pull them from globals
# and reduce boilerplate in other modules
_LOGGER.debug(f'{ENVAR_PATH_DCCSIG}: {PATH_DCCSIG}') # debug tracking
_LOGGER.debug(f'{ENVAR_DCCSI_GDEBUG}: {DCCSI_GDEBUG}')
_LOGGER.debug(f'{ENVAR_DCCSI_DEV_MODE}: {DCCSI_DEV_MODE}')
_LOGGER.debug(f'{ENVAR_DCCSI_GDEBUGGER}: {DCCSI_GDEBUGGER}')
_LOGGER.debug(f'{ENVAR_DCCSI_LOGLEVEL}: {DCCSI_LOGLEVEL}')
_LOGGER.debug(f'{ENVAR_DCCSI_TESTS}: {DCCSI_TESTS}')

###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run as main, perform additional debug and module tests"""

    pass
