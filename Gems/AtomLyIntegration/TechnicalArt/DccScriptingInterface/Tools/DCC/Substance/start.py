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
    < DCCsi >:: Tools//DCC//Substance//start.py

This module is used to start substance with O3DE bootstrapping
"""
# -------------------------------------------------------------------------
import timeit
_MODULE_START = timeit.default_timer()  # start tracking

# standard imports
import sys
import os
import site
import subprocess
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'Tools.DCC.Substance.start'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')

_MODULE_PATH = Path(__file__)  # To Do: what if frozen?

# This ensures basic code access to the DCCsi
# <o3de>/Gems/AtomLyIntegration/TechnicalArt/<DCCsi>
_PATH_DCCSIG = Path(_MODULE_PATH, '../../../..').resolve()
site.addsitedir(_PATH_DCCSIG.as_posix())

import Tools.DCC.Substance.config as substance_config

_SETTINGS = substance_config.get_config_settings()
# --- END -----------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main (external commandline)"""

    _MODULENAME = 'DCCsi.Tools.DCC.Substance.config.cli'

    from azpy.config_utils import ENVAR_DCCSI_GDEBUG
    from azpy.config_utils import ENVAR_DCCSI_DEV_MODE
    from azpy.config_utils import ENVAR_DCCSI_LOGLEVEL
    from azpy.config_utils import ENVAR_DCCSI_GDEBUGGER

    # defaults, can be overridden/forced here for development
    # they should be committed in an off/False state
    from azpy.env_bool import env_bool
    _DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
    _DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
    _DCCSI_LOGLEVEL = env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO)
    _DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')

    from azpy.constants import FRMT_LOG_LONG
    from azpy.constants import STR_CROSSBAR

    if _DCCSI_GDEBUG:
        # override loglevel if running debug
        _DCCSI_LOGLEVEL = _logging.DEBUG

    # configure basic logger
    # note: not using a common logger to reduce cyclical imports
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                         format=FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')

    _LOGGER = _logging.getLogger(_MODULENAME)

    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
    _LOGGER.debug('_DCCSI_GDEBUG: {}'.format(_DCCSI_GDEBUG))
    _LOGGER.debug('_DCCSI_DEV_MODE: {}'.format(_DCCSI_DEV_MODE))
    _LOGGER.debug('_DCCSI_LOGLEVEL: {}'.format(_DCCSI_LOGLEVEL))

    # commandline interface
    import argparse
    parser = argparse.ArgumentParser(
        description='O3DE DCCsi.Tools.DCC.Substance.start',
        epilog="Attempts to start Substance3D with the DCCsi and O3DE bootstrapping")

    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables global debug flag.')

    parser.add_argument('-dm', '--developer-mode',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables dev mode for early auto attaching debugger.')

    parser.add_argument('-sd', '--set-debugger',
                        type=str,
                        required=False,
                        default='WING',
                        help='Default debugger: WING, (not implemented) others: PYCHARM and VSCODE.')

    parser.add_argument('-ex', '--exit',
                        type=bool,
                        required=False,
                        default=False,
                        help='Exits python. Do not exit if you want to be in interactive interpreter after config')

    args = parser.parse_args()

    # easy overrides
    if args.global_debug:
        _DCCSI_GDEBUG = True
        os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)

    if args.developer_mode:
        from azpy.config_utils import attach_debugger
        _DCCSI_DEV_MODE = True
        attach_debugger()  # attempts to start debugger

    if args.set_debugger:
        _LOGGER.info('Setting and switching debugger type not implemented (default=WING)')
        # To Do: implement debugger plugin pattern

    try:
        subprocess.Popen(f'{_SETTINGS.DCCSI_SUBSTANCE_EXE}',
                         env=os.environ.copy(),
                         shell=True)
    except Exception as e:
        _LOGGER.error(f'Could not start Substance: {e}')

    # -- DONE ----
    _LOGGER.info(STR_CROSSBAR)

    _LOGGER.debug('{0} took: {1} sec'.format(_MODULENAME, timeit.default_timer() - _MODULE_START))

    if args.exit:
        import sys
        # return
        sys.exit()
    else:
        # custom prompt
        sys.ps1 = "[{}]>>".format(_MODULENAME)
# --- END -----------------------------------------------------------------
