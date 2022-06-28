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
    < DCCsi >:: Tools//DCC//Blender//config.py

This module manages the dynamic config and settings for boostrapping Blender
"""
# -------------------------------------------------------------------------
# standard imports
import sys
import os
import site
import re
import timeit
import importlib.util
import pathlib
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
#os.environ['PYTHONINSPECT'] = 'True'
_MODULE_START = timeit.default_timer()  # start tracking

# global scope
_MODULENAME = 'Tools.DCC.Blender.config'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.info(f'Initializing: {_MODULENAME}')

_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

# we need to set up basic access to the DCCsi
_PATH_DCCSIG = Path(_MODULE_PATH, '../../../..').resolve()
site.addsitedir(_PATH_DCCSIG.as_posix())

# set envar so DCCsi synthetic env bootstraps with it (config.py)
from azpy.constants import ENVAR_PATH_DCCSIG
os.environ[ENVAR_PATH_DCCSIG] = str(_PATH_DCCSIG.as_posix())
_LOGGER.debug(f'PATH_DCCSIG: {_PATH_DCCSIG.as_posix()}')

# now we have dccsi azpy api access
from azpy.config_utils import *  # imports from __all__

# these must be imported explicitly, they are not defined in __all__
from azpy.config_utils import ENVAR_DCCSI_GDEBUG
from azpy.config_utils import ENVAR_DCCSI_DEV_MODE
from azpy.config_utils import ENVAR_DCCSI_LOGLEVEL
from azpy.config_utils import ENVAR_DCCSI_GDEBUGGER
from azpy.config_utils import FRMT_LOG_LONG
from azpy.config_utils import STR_CROSSBAR

# defaults, can be overriden/forced here for development
from azpy.env_bool import env_bool
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_LOGLEVEL = env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO)
_DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')

# this will boostrap access to the dccsi managed package dependancies
_PATH_DCCSI_PYTHON_LIB = bootstrap_dccsi_py_libs()
site.addsitedir(_PATH_DCCSI_PYTHON_LIB.as_posix())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# This will import and retreive the core <dccsi>/config.py and settings
_DCCSI_CORE_CONFIG = get_dccsi_config(_PATH_DCCSIG)

# now standalone we can validate the config. env, settings.
_SETTINGS = _DCCSI_CORE_CONFIG.get_config_settings(enable_o3de_python=False,
                                                  enable_o3de_pyside2=True,
                                                  set_env=True)
# we don't init the O3DE python env settings!
# that will cause conflicts with the DCC tools python!!!
# we are enabling the O3DE PySide2 (aka QtForPython) access
# to do: get o3de Qt running on a thread in blender:
# https://github.com/friedererdmann/blender_pyside2_example

# now we can extend the environment specific to Blender
# start by grabbing the constants we want to work with as envars
# import others
from Tools.DCC.Blender.constants import *
# import them all, but below are the ones we will use directly
from Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_EXE
from Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_LAUNCHER_EXE
from Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_PY_EXE

#_DCCSI_PATH_BLENDER = Path(sys.prefix)
#os.environ["DYNACONF_DCCSI_PATH_BLENDER"] = _DCCSI_PATH_BLENDER.resolve()
#_LOGGER.debug(f"Blender Install: {_DCCSI_PATH_BLENDER}")
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_config_settings(settings=_SETTINGS):
    return settings
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone cli script for testing/debugging"""

    main_start = timeit.default_timer()  # start tracking

    while 0:  # temp internal debug flag, toggle values for manual testing
        _DCCSI_GDEBUG = True
        _DCCSI_DEV_MODE = False
        _DCCSI_LOGLEVEL = _logging.DEBUG
        _DCCSI_GDEBUGGER = 'WING'
        break

    _MODULENAME = 'DCCsi.Tools.DCC.Blender.config.cli'

    # default loglevel to info unless set
    _DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))
    if _DCCSI_GDEBUG:
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

    _LOGGER = _logging.getLogger(_MODULENAME)
    _LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
    _LOGGER.debug('site.addsitedir({})'.format(_PATH_DCCSIG))
    _LOGGER.debug('_DCCSI_GDEBUG: {}'.format(_DCCSI_GDEBUG))
    _LOGGER.debug('_DCCSI_DEV_MODE: {}'.format(_DCCSI_DEV_MODE))
    _LOGGER.debug('_DCCSI_LOGLEVEL: {}'.format(_DCCSI_LOGLEVEL))

    # happy print
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('~ {} ... Running script as __main__'.format(_MODULENAME))
    _LOGGER.info(STR_CROSSBAR)

    # go ahead and run the rest of the configuration
    # parse the command line args
    import argparse
    parser = argparse.ArgumentParser(
        description='O3DE DCCsi Dynamic Config (dynaconf) for Blender',
        epilog="Attempts to determine O3DE project if -pp not set")

    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        default=False,
                        help='(NOT IMPLEMENTED) Enables global debug flag.')

    parser.add_argument('-dm', '--developer-mode',
                        type=bool,
                        required=False,
                        default=False,
                        help='(NOT IMPLEMENTED) Enables dev mode for early auto attaching debugger.')

    parser.add_argument('-sd', '--set-debugger',
                        type=str,
                        required=False,
                        default='WING',
                        help='(NOT IMPLEMENTED) Default debugger: WING, thers: PYCHARM and VSCODE.')

    parser.add_argument('-pp', '--project-path',
                        type=pathlib.Path,
                        required=False,
                        default=Path('{ to do: implement }'),
                        help='(NOT IMPLEMENTED) The path to the project.')

    parser.add_argument('-qt', '--enable-qt',
                        type=bool,
                        required=False,
                        default=False,
                        help='(NOT IMPLEMENTED) Enables O3DE Qt & PySide2 access.')

    parser.add_argument('-tp', '--test-pyside2',
                        type=bool,
                        required=False,
                        default=False,
                        help='(NOT IMPLEMENTED) Runs Qt/PySide2 tests and reports.')

    parser.add_argument('-ex', '--exit',
                        type=bool,
                        required=False,
                        default=False,
                        help='(NOT IMPLEMENTED) Exits python. Do not exit if you want to be in interactive interpretter after config')

    args = parser.parse_args()

    # easy overrides
    if args.global_debug:
        _DCCSI_GDEBUG = True
        os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)

    if args.developer_mode:
        _DCCSI_DEV_MODE = True
        attach_debugger()  # attempts to start debugger

    if args.set_debugger:
        _LOGGER.info('Setting and switching debugger type not implemented (default=WING)')
        # To Do: implement debugger plugin pattern

    # after CLI set the altered/extended env
    settings = get_config_settings()
    settings.setenv()  # doing this will add/set the additional DYNACONF_ envars

    # CORE
    _LOGGER.info(STR_CROSSBAR)
    # not using fstrings in this module because it might run in py2.7 (maya)
    _LOGGER.info('DCCSI_GDEBUG: {}'.format(settings.DCCSI_GDEBUG))
    _LOGGER.info('DCCSI_DEV_MODE: {}'.format(settings.DCCSI_DEV_MODE))
    _LOGGER.info('DCCSI_LOGLEVEL: {}'.format(settings.DCCSI_LOGLEVEL))
    _LOGGER.info('DCCSI_OS_FOLDER: {}'.format(settings.DCCSI_OS_FOLDER))

    _LOGGER.info('O3DE_DEV: {}'.format(settings.O3DE_DEV))
    _LOGGER.info('O3DE_O3DE_BUILD_FOLDER: {}'.format(settings.PATH_O3DE_BUILD))
    _LOGGER.info('PATH_O3DE_BUILD: {}'.format(settings.PATH_O3DE_BUILD))
    _LOGGER.info('PATH_O3DE_BIN: {}'.format(settings.PATH_O3DE_BIN))

    _LOGGER.info('PATH_DCCSIG: {}'.format(settings.PATH_DCCSIG))
    _LOGGER.info('DCCSI_LOG_PATH: {}'.format(settings.DCCSI_LOG_PATH))
    _LOGGER.info('PATH_DCCSI_CONFIG: {}'.format(settings.PATH_DCCSI_CONFIG))

    # end tracking here, the pyside test exits before hitting the end of script
    _LOGGER.info('{0} took: {1} sec'.format(_MODULENAME, timeit.default_timer() - _MODULE_START))

    if args.exit:
        # return
        sys.exit()
# --- END -----------------------------------------------------------------
