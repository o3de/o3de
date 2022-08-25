#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! This module is for use in bootstrapping the DccScriptingInterface Gem
< dccsi > with O3DE.  The dccsi package if a framework, which includes DCC
tool integrations for O3DE workflows; and also provides technical artists
and out-of-box python development environment.

:file: DccScriptingInterface\\editor\\scripts\\boostrap.py
:Status: Prototype
:Version: 0.0.1
:Future: is unknown
:Entrypoint: is an entrypoint and thus configures logging
:Notice:
    Currently windows only (not tested on other platforms)
    No support for legacy DCC tools stuck on py2 (py3 only)
"""
# standard imports
import sys
import os
import site
import timeit
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'DCCsi.editor.scripts.bootstrap'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

# this file
_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

# dccsi root, ensure pkg access
PATH_DCCSIG = _MODULE_PATH.parents[2].resolve()
site.addsitedir(PATH_DCCSIG.as_posix())

# set envar so DCCsi synthetic env bootstraps with it (config.py files)
from DccScriptingInterface.azpy.constants import ENVAR_PATH_DCCSIG
os.environ[ENVAR_PATH_DCCSIG] = str(PATH_DCCSIG.resolve())

import DccScriptingInterface.config as _core_config
# -------------------------------------------------------------------------


# ---- debug stuff --------------------------------------------------------
# local dccsi imports
# this accesses common global state, e.g. DCCSI_GDEBUG (is True or False)
from DccScriptingInterface.globals import *

# auto-attach ide debugging at the earliest possible point in module
from DccScriptingInterface.azpy.config_utils import attach_debugger
if DCCSI_DEV_MODE: # from DccScriptingInterface.globals
    attach_debugger(debugger_type=DCCSI_GDEBUGGER)

if DCCSI_GDEBUG: # provides some basic profiling to ensure dccsi speediness
    _START = timeit.default_timer() # start tracking

# configure basic logger
# it is suggested that this be replaced with a common logging module later
if DCCSI_GDEBUG or DCCSI_DEV_MODE:
    DCCSI_LOGLEVEL = _logging.DEBUG
    _LOGGER.setLevel(DCCSI_LOGLEVEL) # throttle up help


_logging.basicConfig(level=DCCSI_LOGLEVEL,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_Editor(test_config=DCCSI_GDEBUG,
                     test_pyside2=DCCSI_GDEBUG):
    """Put bootstrapping code here to execute in O3DE Editor.exe"""

    _settings = None

    if test_config:
        # set and retreive the base env context/_settings on import
        _settings = _core_config.get_config_settings(enable_o3de_python=True,
                                                     enable_o3de_pyside2=True)
        # note: this can impact start up times so currently we are only
        # running it to test.
        # To Do: slim down start up times so we can init _settings

    if test_pyside2:
        _core_config.test_pyside2()

    return _settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_MaterialEditor():
    """Put bootstrapping code here to execute in O3DE MaterialEdito.exe"""
    pass
    return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_AssetProcessor():
    """Put boostrapping code here to execute in O3DE AssetProcessor.exe"""
    pass
    return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_AssetBuilder():
    """Put boostrapping code here to execute in O3DE AssetBuilder.exe"""
    pass
    return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# set and retreive the *basic* env context/_settings on import
# What application is executing the bootstrap?
# Python is being run from:
#    editor.exe
#    materialeditor.exe
#    assetprocessor.exe
#    assetbuilder.exe, or the Python executable.
# Exclude the .exe so it works on other platforms

_O3DE_EDITOR = Path(sys.executable)
_LOGGER.debug(f'The sys.executable is: {_O3DE_EDITOR}')

if _O3DE_EDITOR.stem.lower() == "editor":
    # if _DCCSI_GDEBUG then run the pyside2 test
    _settings = bootstrap_Editor(DCCSI_GDEBUG)

elif _O3DE_EDITOR.stem.lower() == "materialeditor":
    _settings = bootstrap_MaterialEditor()

elif _O3DE_EDITOR.stem.lower() == "assetprocessor":
    _settings = bootstrap_AssetProcessor()

elif _O3DE_EDITOR.stem.lower() == "assetbuilder":
    _settings= bootstrap_AssetBuilder()

elif _O3DE_EDITOR.stem.lower() == "python":
    # in this case, we can re-use the editor settings
    # which will init python and pyside2 access externally
    _settings= bootstrap_Editor(DCCSI_GDEBUG)

else:
    _LOGGER.warning(f'No bootstrapping code for: {_O3DE_EDITOR}')

if DCCSI_GDEBUG:
    _LOGGER.debug('{0} took: {1} sec'.format(_MODULENAME, timeit.default_timer() - _START))
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main (external commandline for testing)"""
    # ---------------------------------------------------------------------
    # force enable debug settings manually
    DCCSI_GDEBUG = False # enable here to force temporarily
    _DCCSI_DEV_MODE = False

    # default loglevel to info unless set
    _DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))
    if DCCSI_GDEBUG:
        # override loglevel if runnign debug
        _DCCSI_LOGLEVEL = _logging.DEBUG

    # set up module logging
    #for handler in _logging.root.handlers[:]:
        #_logging.root.removeHandler(handler)

    # configure basic logger
    # note: not using a common logger to reduce cyclical imports
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                        format=FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')

    # happy print
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('DCCsi: bootstrap.py ... Running script as __main__')
    # ---------------------------------------------------------------------


    # ---------------------------------------------------------------------
    _O3DE_RUNNING=None
    try:
        import azlmbr
        _O3DE_RUNNING=True
    except:
        _O3DE_RUNNING=False
    # ---------------------------------------------------------------------


    # ---------------------------------------------------------------------
    # this is a simple commandline interface for running and testing externally
    if not _O3DE_RUNNING: # external tool or commandline

        # parse the command line args
        import argparse
        parser = argparse.ArgumentParser(
            description='O3DE DCCsi Boostrap (Test)',
            epilog="Will externally test the DCCsi boostrap")

        parser.add_argument('-gd', '--global-debug',
                            type=bool,
                            required=False,
                            help='Enables global debug flag.')

        parser.add_argument('-sd', '--set-debugger',
                            type=str,
                            required=False,
                            help='Default debugger: WING, others: PYCHARM, VSCODE (not yet implemented).')

        parser.add_argument('-dm', '--developer-mode',
                            type=bool,
                            required=False,
                            help='Enables dev mode for early auto attaching debugger.')

        parser.add_argument('-tp', '--test-pyside2',
                            type=bool,
                            required=False,
                            help='Runs Qt/PySide2 tests and reports.')

        args = parser.parse_args()

        # easy overrides
        if args.global_debug:
            DCCSI_GDEBUG = True
            _DCCSI_LOGLEVEL = _logging.DEBUG
            _LOGGER.setLevel(_DCCSI_LOGLEVEL)

        if args.set_debugger:
            _LOGGER.info('Setting and switching debugger type not implemented (default=WING)')
            # To Do: implement debugger plugin pattern

        if args.developer_mode or _DCCSI_DEV_MODE:
            _DCCSI_DEV_MODE = True
            foo = attach_debugger()  # attempts to start debugger

        _TEST_PYSIDE2 = False
        if args.test_pyside2:
            _TEST_PYSIDE2 = True

        _settings= bootstrap_Editor(DCCSI_GDEBUG, _TEST_PYSIDE2)

        if DCCSI_GDEBUG:
            _LOGGER.info(f'PATH_DCCSIG: {_settings.PATH_DCCSIG}')
            _LOGGER.info(f'DCCSI_G_DEBUG: {_settings.DCCSI_GDEBUG}')
            _LOGGER.info(f'DCCSI_DEV_MODE: {_settings.DCCSI_DEV_MODE}')

            _LOGGER.info(f'DCCSI_OS_FOLDER: {_settings.DCCSI_OS_FOLDER}')
            _LOGGER.info(f'O3DE_PROJECT: {_settings.O3DE_PROJECT}')
            _LOGGER.info(f'PATH_O3DE_PROJECT: {_settings.PATH_O3DE_PROJECT}')
            _LOGGER.info(f'O3DE_DEV: {_settings.O3DE_DEV}')
            _LOGGER.info(f'PATH_O3DE_BUILD: {_settings.PATH_O3DE_BUILD}')
            _LOGGER.info(f'PATH_O3DE_BIN: {_settings.PATH_O3DE_BIN}')

            _LOGGER.info(f'PATH_DCCSIG: {_settings.PATH_DCCSIG}')
            _LOGGER.info(f'PATH_DCCSI_PYTHON_LIB: {_settings.PATH_DCCSI_PYTHON_LIB}')
            _LOGGER.info(f'DCCSI_PY_BASE: {_settings.DCCSI_PY_BASE}')

        if args.test_pyside2:
            _LOGGER.info(f'PySide2: {PySide2}')
            _LOGGER.info(f'PATH_O3DE_BIN: {_settings.PATH_O3DE_BIN}')
            _LOGGER.info(f'QT_PLUGIN_PATH: {_settings.QT_PLUGIN_PATH}')
            _LOGGER.info(f'QT_QPA_PLATFORM_PLUGIN_PATH: {_settings.QT_QPA_PLATFORM_PLUGIN_PATH}')
    # ---------------------------------------------------------------------

    # -- DONE ----
    _LOGGER.info('{0} took: {1} sec'.format(_MODULENAME, timeit.default_timer() - _START))
    _LOGGER.info(STR_CROSSBAR)

    # custom prompt
    sys.ps1 = "[{}]>>".format(_MODULENAME)

# --- END -----------------------------------------------------------------
