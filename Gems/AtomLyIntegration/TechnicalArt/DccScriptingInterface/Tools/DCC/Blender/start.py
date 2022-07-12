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
    < DCCsi >:: Tools//DCC//Blender//start.py

This module is used to start blender with O3DE bootstrapping
"""
# -------------------------------------------------------------------------
# standard imports
import sys
import os
import site
import timeit
import inspect
import subprocess
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
#os.environ['PYTHONINSPECT'] = 'True'
# global scope
_MODULENAME = 'Tools.DCC.Blender.start'

_START = timeit.default_timer() # start tracking

# we need to set up basic access to the DCCsi
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_PATH_DCCSIG = Path(_MODULE_PATH, '../../../..').resolve()
site.addsitedir(_PATH_DCCSIG.as_posix())

# set envar so DCCsi synthetic env bootstraps with it (config.py)
from azpy.constants import ENVAR_PATH_DCCSIG
_PATH_DCCSIG = Path(os.getenv(ENVAR_PATH_DCCSIG,
                              _PATH_DCCSIG.as_posix()))

from azpy.constants import FRMT_LOG_LONG
_logging.basicConfig(level=_logging.DEBUG,
                     format=FRMT_LOG_LONG,
                    datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')
_LOGGER.debug(f'PATH_DCCSIG: {_PATH_DCCSIG.as_posix()}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# now we have dccsi azpy api access
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG

# these allow these ENVARs to be set externally
# defaults can be overridden/forced here for development
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_LOGLEVEL = env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO)
_DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# default local dccsi related paths for Blender
# the constants module doesn't set these as ENVARs, so we can do that here.
# these defaults could then be picked up in bootstrap.py and/or config.py
from Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_TOOLS
from Tools.DCC.Blender.constants import PATH_DCCSI_TOOLS
os.environ[ENVAR_PATH_DCCSI_TOOLS] = PATH_DCCSI_TOOLS.as_posix()

from Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_TOOLS_DCC
from Tools.DCC.Blender.constants import PATH_DCCSI_TOOLS_DCC
os.environ[ENVAR_PATH_DCCSI_TOOLS_DCC] = PATH_DCCSI_TOOLS_DCC.as_posix()

from Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_TOOLS_BLENDER
from Tools.DCC.Blender.constants import PATH_DCCSI_TOOLS_BLENDER
os.environ[ENVAR_PATH_DCCSI_TOOLS_BLENDER] = PATH_DCCSI_TOOLS_BLENDER.as_posix()

from Tools.DCC.Blender.constants import ENVAR_DCCSI_BLENDER_VERSION
from Tools.DCC.Blender.constants import TAG_DCCSI_BLENDER_VERSION
os.environ[ENVAR_DCCSI_BLENDER_VERSION] = TAG_DCCSI_BLENDER_VERSION

from Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_BLENDER_LOC
from Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_LOC
os.environ[ENVAR_PATH_DCCSI_BLENDER_LOC] = PATH_DCCSI_BLENDER_LOC.as_posix()

from Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_BLENDER_EXE
from Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_EXE
os.environ[ENVAR_PATH_DCCSI_BLENDER_EXE] = PATH_DCCSI_BLENDER_EXE.as_posix()

from Tools.DCC.Blender.constants import ENVAR_DCCSI_BLENDER_PY_EXE
from Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_PY_EXE
os.environ[ENVAR_DCCSI_BLENDER_PY_EXE] = PATH_DCCSI_BLENDER_PY_EXE.as_posix()
# --- END -----------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main (external commandline)"""
    STR_CROSSBAR = f"{'-' * 74}"

    if _DCCSI_GDEBUG:
        # override loglevel if running debug
        _DCCSI_LOGLEVEL = _logging.DEBUG

    FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"

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
        description='O3DE DCCsi.Tools.DCC.Blender.start',
        epilog="Attempts to start Blender with the DCCsi and O3DE bootstrapping")

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

    parser.add_argument('-be', '--blender-executable',
                        type=str,
                        required=False,
                        default='Blender',
                        help="Name of exe to start, options are: 'Blender', 'Launcher' (aka Blender-Launcher.exe),'Python' (blenders python) ")

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

    if args.blender_executable:
        if args.blender_executable == 'Blender':
            _blender_exe = str(PATH_DCCSI_BLENDER_EXE.as_posix())
            subprocess.Popen(f'{_blender_exe}',
                             env=os.environ.copy(), shell=True)

        elif args.blender_executable == 'Launcher':
            _LOGGER.warn(f'Not Implemented Yet')

        elif args.blender_executable == 'Python':
            _LOGGER.warn(f'Not Implemented Yet')

        else:
            _LOGGER.error(f'Specified option {args.blender_executable}, is not supported!')

    # -- DONE ----
    _LOGGER.info(STR_CROSSBAR)

    _LOGGER.debug('{0} took: {1} sec'.format(_MODULENAME, timeit.default_timer() - _START))

    if args.exit:
        import sys
        # return
        sys.exit()
    else:
        # custom prompt
        sys.ps1 = "[{}]>>".format(_MODULENAME)
# --- END -----------------------------------------------------------------
