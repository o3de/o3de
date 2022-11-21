#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! This module is an early temporary way to attach wing ide as debugger"""
# TO DO: this whole file needs a refactor!!!
# -------------------------------------------------------------------------
# standard imports
from __future__ import unicode_literals
import os
import sys
import site
import importlib
import logging as _logging
from pathlib import Path
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_BOOT_CHECK = False  # set true to test breakpoint in this module directly

# global scope
from DccScriptingInterface import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.entry_test'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

# local dccsi imports
# this accesses common global state, e.g. DCCSI_GDEBUG (is True or False)
from DccScriptingInterface.globals import *

# this module needs to be lightweight and avoid cyclical imports
# so refactoring this out, a more fully featured dev module with
# support for multiple IDEs can come later
# from DccScriptingInterface.Tools.IDE.Wing.config import wing_config
# settings = wing_config.get_settings()

from DccScriptingInterface import PATH_WINGHOME
WINGHOME = Path(PATH_WINGHOME).resolve()

from DccScriptingInterface import PATH_WING_APPDATA
WING_APPDATA = Path(PATH_WINGHOME).resolve()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def import_module_path(module_name, module_path):
    """! Imports a module from abs path."""
    from importlib import util

    spec = util.spec_from_file_location(module_name, module_path)
    module = util.module_from_spec(spec)

    spec.loader.exec_module(module)
    return module
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
def main(verbose=DCCSI_GDEBUG, connect_debugger=True):
    if verbose:
        _LOGGER.info(f"{'-' * 74}")
        _LOGGER.info('entry_test.main()')
        _LOGGER.info('Root test import successful:')
        _LOGGER.info('~   {}'.format(__file__))

    if connect_debugger:
        status = connect_wing()
        _LOGGER.info(status)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def connect_wing():
    _LOGGER.info(f"{'-' * 74}")
    _LOGGER.info('entry_test.connect_wing()')

    try:
        Path(PATH_WINGHOME).exists()
        _LOGGER.info(f'~   WINGHOME: {PATH_WINGHOME}')
    except Exception as e:
        _LOGGER.error(e)
        _LOGGER.error(f'WINGHOME does not exist')

        # this line temp until future refactor
        _LOGGER.warning(f'Check to ensure Wing Pro 8, is installed')
        _LOGGER.warning(f'C:\Program Files (x86)\Wing Pro 8')
        return None

    try:
        _wing_appdata = Path(WING_APPDATA)
        _wing_appdata.exists()
        _LOGGER.info(f'~   WING_APPDATA: {WING_APPDATA}')
        wDBstub = Path(_wing_appdata, 'wingdbstub.py').resolve(strict=True)
        _LOGGER.info(f'~   Wing debugger: {wDBstub}')
    except Exception as e:
        _LOGGER.error(e)
        _LOGGER.warning(f'{WING_APPDATA}\wingdbstub.py does not exist')
        _LOGGER.warning(f'This is required to attach wing as debugger')
        _LOGGER.warning(f'Copy the file: {PATH_WINGHOME}\\wingdbstub.py')
        _LOGGER.warning(f'To the dir: {WING_APPDATA}\\wingdbstub.py')
        _LOGGER.warning(f'Then open the file: {WING_APPDATA}\\wingdbstub.py')
        _LOGGER.warning(f"Modify line 96 to 'kEmbedded = 1'")
        return None

    try:
        wingdbstub = import_module_path("wingdbstub", wDBstub.as_posix())
        _LOGGER.info('~   Success: imported wingdbstub')
    except Exception as e:
        _LOGGER.warning(e)
        _LOGGER.warning('warning: import wingdbstub.py, FAILED')
        pass

#     try: # hail mary, random import attempt
#         import wingdbstub
#     except Exception as e:
#         _LOGGER.warning('warning: import wingdbstub.py, FAILED')
#         return None

    try:
        wingdbstub.Ensure(require_connection=1, require_debugger=1)
        _LOGGER.info('~   Success: wingdbstub.Ensure()')
    except Exception as e:
        _LOGGER.error(e)
        _LOGGER.warning('Error: wingdbstub.Ensure()')
        _LOGGER.warning('Check that wing ide is running')

    try:
        wingdbstub.debugger.StartDebug()
        # leave a tag, so another boostrap can check if already set
        os.environ["DCCSI_DEBUGGER_ATTACHED"] = 'True'
        _LOGGER.info('~   Success: wingdbstub.debugger.StartDebug()')
    except Exception as e:
        _LOGGER.warning(e)
        _LOGGER.warning('Error: wingdbstub.debugger.StartDebug()')
        _LOGGER.warning('Check that wing ide is running, and not attached to another process')

    _LOGGER.info('{0}'.format('-' * 74))

    # a hack to allow a place to drop a breakpoint on boot
    while _BOOT_CHECK:
        _LOGGER.debug(str('SPAM'))

    return
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run in debug perform local tests from IDE or CLI"""

    # default loglevel to info unless set
    DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))
    if DCCSI_GDEBUG:
        # override loglevel if runnign debug
        DCCSI_LOGLEVEL = _logging.DEBUG

    # configure basic logger
    # note: not using a common logger to reduce cyclical imports
    _logging.basicConfig(level=DCCSI_LOGLEVEL,
                        format=FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')

    _LOGGER = _logging.getLogger(_MODULENAME)

    _LOGGER.setLevel(_logging.DEBUG)

    _debugger = connect_wing()

    _LOGGER.debug('_DCCSI_GDEBUG: {}'.format(DCCSI_GDEBUG))
    _LOGGER.debug('_DCCSI_DEV_MODE: {}'.format(DCCSI_DEV_MODE))
    _LOGGER.debug('_DCCSI_LOGLEVEL: {}'.format(DCCSI_LOGLEVEL))
# -------------------------------------------------------------------------
