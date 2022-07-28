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
<DCCsi>/azpy/__init__.py

This is the shared pure-python api.
"""

# -------------------------------------------------------------------------
# standard imports
import sys
import os
import site
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_PACKAGENAME = 'azpy'

__all__ = ['constants',
           'config_utils',
           'env_bool',
           'return_stub',
           'logger',
           'core',
           'dcc',
           'dev',
           'o3de',
           'shared',
           'test']

# This module should be improved in a future refactor
# 1 - reduce boiler plate and slim
# 2 - no need to support py2.7 anymore, can use pathlib

# we need to set up basic access to the DCCsi
_MODULE_PATH = Path(__file__)
_PATH_DCCSIG = _MODULE_PATH.parents[1].resolve()
# allows env to override the path externally
_PATH_DCCSIG = os.getenv('PATH_DCCSIG', _PATH_DCCSIG)
site.addsitedir(_PATH_DCCSIG)

_PATH_DCCSI_AZPY = _MODULE_PATH.parent.name

# azpy
import azpy.return_stub as return_stub
import azpy.env_bool as env_bool
import azpy.constants as constants
import azpy.config_utils as config_utils

_DCCSI_GDEBUG = env_bool.env_bool(constants.ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool.env_bool(constants.ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_TESTS = env_bool.env_bool(constants.ENVAR_DCCSI_TESTS, False)
_DCCSI_GDEBUGGER = env_bool.env_bool(constants.ENVAR_DCCSI_GDEBUGGER, 'WING')

# default loglevel to info unless set
_DCCSI_LOGLEVEL = int(env_bool.env_bool(constants.ENVAR_DCCSI_LOGLEVEL,
                                        _logging.INFO))
if _DCCSI_GDEBUG:
    # override loglevel if runnign debug
    _DCCSI_LOGLEVEL = _logging.DEBUG

_logging.basicConfig(level=_DCCSI_LOGLEVEL,
                     format=constants.FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# for py2.7 (Maya) we provide this, so we must assume some bootstrapping
# has occured, see DccScriptingInterface\\config.py (_PATH_DCCSI_PYTHON_LIB)
#
# We are no longer needing to maintain py2.7 support and this can be improved.
try:
    import pathlib
except:
    import pathlib2 as pathlib
from pathlib import Path
if _DCCSI_GDEBUG:
    _LOGGER.debug('[DCCsi][AZPY] DEBUG BREADCRUMB, pathlib is: {}'.format(pathlib))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# default o3de engin location
_O3DE_DEV = os.path.normpath(os.path.join(_PATH_DCCSIG, '../../../..'))
_O3DE_DEV = os.getenv(constants.ENVAR_O3DE_DEV, _O3DE_DEV)
_O3DE_DEV = Path(_O3DE_DEV)
_LOGGER.debug('_O3DE_DEV" {}'.format(_O3DE_DEV.resolve()))

# use dccsi as the default project location
_PATH_O3DE_PROJECT = Path(os.getenv(constants.ENVAR_PATH_O3DE_PROJECT, _PATH_DCCSIG))
_LOGGER.debug('Default PATH_O3DE_PROJECT" {}'.format(_PATH_O3DE_PROJECT.resolve()))

# use dccsi as the default project name
_O3DE_PROJECT = str(os.getenv(constants.ENVAR_O3DE_PROJECT, _PATH_O3DE_PROJECT.name))

# project cache log dir path
from azpy.constants import TAG_DCCSI_NICKNAME
from azpy.constants import PATH_DCCSI_LOG_PATH
_DCCSI_LOG_PATH = Path(PATH_DCCSI_LOG_PATH.format(PATH_O3DE_PROJECT=_PATH_O3DE_PROJECT.resolve(),
                                                  TAG_DCCSI_NICKNAME=TAG_DCCSI_NICKNAME))
# -------------------------------------------------------------------------

# some methods refactored from here into azpy.shared.utils.init
from azpy.shared.utils.init import makedirs
from azpy.shared.utils.init import FileExistsError
from azpy.shared.utils.init import initialize_logger
from azpy.shared.utils.init import test_imports

# -------------------------------------------------------------------------
# some simple logger tests
# evoke the filehandlers and test writting to the log file
if _DCCSI_GDEBUG:
    _LOGGER.info('Forced Info! for {0}.'.format({_PACKAGENAME}))
    _LOGGER.error('Forced ERROR! for {0}.'.format({_PACKAGENAME}))

# debug breadcrumbs to check this module and used paths
_LOGGER.debug('MODULE_PATH: {}'.format(_MODULE_PATH))
_LOGGER.debug('O3DE_DEV_PATH: {}'.format(_O3DE_DEV))
_LOGGER.debug('PATH_DCCSIG: {}'.format(_PATH_DCCSIG))
_LOGGER.debug('O3DE_PROJECT_TAG: {}'.format(_O3DE_PROJECT))
_LOGGER.debug('DCCSI_LOG_PATH: {}'.format(_DCCSI_LOG_PATH))
# -------------------------------------------------------------------------


##########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run in debug perform local tests from IDE or CLI"""

    # this is a legacy cli that existed prior to config.py patterns
    # which now handle configuration and dynamic settings
    # this cli should be removed in a future iteration (tech debt)
    # and this __ini__ streamlined and slimmed down

    _LOGGER.info(constants.STR_CROSSBAR)
    _LOGGER.info('~ {}.py ... Running script as __main__'.format(_PACKAGENAME))
    _LOGGER.info(constants.STR_CROSSBAR)

    # default loglevel to info unless set
    _DCCSI_LOGLEVEL = int(env_bool.env_bool(constants.ENVAR_DCCSI_LOGLEVEL,
                                            _logging.INFO))
    if _DCCSI_GDEBUG:
        # override loglevel if running debug
        _DCCSI_LOGLEVEL = _logging.DEBUG


    # configure basic logger
    # note: not using a common logger to reduce cyclical imports
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                        format=constants.FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')

    # re-configure basic logger for debug
    _LOGGER = _logging.getLogger(_PACKAGENAME)

    import argparse
    parser = argparse.ArgumentParser(
        description='O3DE DCCsi azpy API CLI',
        epilog="Allows for some light testing of the API structure from CLI")

    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables global debug flag.')

    parser.add_argument('-rt', '--run-tests',
                        type=bool,
                        required=False,
                        default=True,
                        help='Runs local import and other tests.')

    parser.add_argument('-sd', '--set-debugger',
                        type=str,
                        required=False,
                        default='WING',
                        help='Default debugger: WING, (not implemented) others: PYCHARM and VSCODE.')

    parser.add_argument('-dm', '--developer-mode',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables dev mode for early auto attaching debugger.')

    parser.add_argument('-ex', '--exit',
                        type=bool,
                        required=False,
                        default=True,
                        help='Exits python. Do not exit if you want to be in interactive interpreter after config')

    args = parser.parse_args()

    # easy overrides
    if args.global_debug:
        _DCCSI_GDEBUG = True
        os.environ["DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)

    if args.set_debugger:
        _LOGGER.info('Setting and switching debugger type not implemented yet (default=WING)')
        # To Do: implement debugger plugin pattern

    if args.developer_mode:
        from azpy.config_utils import attach_debugger
        _DCCSI_DEV_MODE = True
        os.environ["DCCSI_DEV_MODE"] = str(_DCCSI_DEV_MODE)
        attach_debugger()  # attempts to start debugger

    _LOGGER.info(constants.STR_CROSSBAR)
    _LOGGER.info('_MODULE_PATH: {}'.format(_MODULE_PATH))
    _LOGGER.info('_PATH_DCCSIG: {}'.format(_PATH_DCCSIG))
    _LOGGER.info('_DCCSI_GDEBUG: {}'.format(_DCCSI_GDEBUG))
    _LOGGER.info('_DCCSI_TESTS: {}'.format(_DCCSI_TESTS))
    _LOGGER.info('_DCCSI_DEV_MODE: {}'.format(_DCCSI_DEV_MODE))
    _LOGGER.info('_DCCSI_LOGLEVEL: {}'.format(_DCCSI_LOGLEVEL))

    if args.run_tests:
        _DCCSI_TESTS = True
        os.environ["DCCSI_TESTS"] = str(_DCCSI_TESTS)

        _LOGGER.info('TESTS ENABLED, _DCCSI_TESTS: True')
        _LOGGER.info('Tests are recursive package/module imports and will be verbose!')

        # If in dev mode this will test imports of __all__
        from azpy import test_imports

        _LOGGER.info(constants.STR_CROSSBAR)

        _LOGGER.info('Testing Imports from {0}'.format(_PACKAGENAME))
        test_imports(__all__,
                     _pkg=_PACKAGENAME,
                     _logger=_LOGGER)

    # -- DONE ----
    _LOGGER.info(constants.STR_CROSSBAR)

    if args.exit:
        import sys
        # return
        sys.exit()
# -------------------------------------------------------------------------
