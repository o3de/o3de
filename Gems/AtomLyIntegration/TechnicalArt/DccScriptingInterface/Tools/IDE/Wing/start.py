#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# -------------------------------------------------------------------------
#
"""! O3DE DCCsi Wing Pro 8+ IDE start module

There are severl ways this can be used:
1. From cli
    - open a cmd
    - change directory > cd c:\\path\\to\\DccScriptingInterface
    - run command > python Tools\\IDE\\Wing\\start.py

2. The O3DE editor uses this module to launch from menu and editor systems

:file: DccScriptingInterface\\Tools\\IDE\\Wing\\start.py
:Status: Prototype
:Version: 0.0.1
:Entrypoint: entrypoint, configures logging, includes cli
:Notice:
    Currently windows only (not tested on other platforms)
    Currently only tested with Wing Pro 8 in Windows
"""
# -------------------------------------------------------------------------
import timeit
_MODULE_START = timeit.default_timer()  # start tracking

# standard imports
import sys
import os
import subprocess
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# this is an entry point, we must self bootstrap
_MODULE_PATH = Path(__file__)
PATH_O3DE_TECHART_GEMS = _MODULE_PATH.parents[4].resolve()
os.chdir(PATH_O3DE_TECHART_GEMS.as_posix())
sys.path.insert(0, PATH_O3DE_TECHART_GEMS.as_posix())

from DccScriptingInterface import add_site_dir
add_site_dir(PATH_O3DE_TECHART_GEMS) # cleaner add
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.Tools.IDE.Wing import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.start'

# get the global dccsi state
from DccScriptingInterface.globals import *

from azpy.constants import FRMT_LOG_LONG
_logging.basicConfig(level=_logging.DEBUG,
                     format=FRMT_LOG_LONG,
                    datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)

# auto-attach ide debugging at the earliest possible point in module
if DCCSI_DEV_MODE:
    if DCCSI_GDEBUGGER == 'WING':
        import DccScriptingInterface.azpy.test.entry_test
        DccScriptingInterface.azpy.test.entry_test.connect_wing()
    elif DCCSI_GDEBUGGER == 'PYCHARM':
        _LOGGER.warning(f'{DCCSI_GDEBUGGER} debugger auto-attach not yet implemented')
    else:
        _LOGGER.warning(f'{DCCSI_GDEBUGGER} not a supported debugger')

_LOGGER.debug(f'Initializing: {_MODULENAME}')
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

# retreive the wing_config class object and it's settings
from DccScriptingInterface.Tools.IDE.Wing.config import wing_config
wing_config.settings.setenv() # ensure env is set

# alternatively, you could get the settings via this wrapped method
# settings = wing_config.get_settings(set_env=True)

# or the standard dynaconf way, this will walk to find and exec config.py
#from dynaconf import settings

# if you want to directly work with settings
# foo = wing_config.settings.WING_PROJ

from DccScriptingInterface.azpy.config_utils import check_is_ascii

# the subprocess call needs an environment
# if we try to pass this, we get a failure to launch
# env = wing_config.settings.as_dict()
# error: "Fatal Python error: _Py_HashRandomization_Init: failed to get random numbers to initialize Python\nPython runtime state: preinitialized"
#https://stackoverflow.com/questions/58997105/fatal-python-error-failed-to-get-random-numbers-to-initialize-python

# another encountered problem is that the dynaconf settings dict may have
# non-string Key:value pairs, which the subprocess env chokes on so we prune below

# store a copy, so we can inspect/compare later
orig_env = os.environ.copy()

# we are going to pass the system environ
wing_env = os.environ.copy()

# generate a pruned env from dict that subprocess will not complain about
# hopefully we don't loose anything vital (that isn't procedurally derived later)
wing_env = {key: value for key, value in wing_env.items() if check_is_ascii(key) and check_is_ascii(value)}

# if we are trying to start an app via script or from an app like o3de,
# the environ will propogate Qt related envars like 'QT_PLUGIN_PATH'
# Wing is a Qt5 application (as are some DCC tools) and this can cause a boot failure
# in this case, the most straightforward approach is to just prune them
wing_env = {key: value for key, value in wing_env.items() if not key.startswith("QT_")}

if DCCSI_GDEBUG:
    # we can see what was pruned
    pruned = {k: wing_env[k] for k in set(wing_env) - set(orig_env)}

    if len(pruned.items()):
        _LOGGER.debug(f'prune diff is ...')
        for p in pruned.items():
            _LOGGER.debug(f'{p}')

def popen(exe = wing_config.settings.WING_EXE,
         project_file = wing_config.settings.WING_PROJ):
    """Method to call, to start Wing IDE"""

    _LOGGER.debug(f'Attempting to start wing ...')

    wing_proc = subprocess.Popen([exe, project_file],
                                 env = wing_env,
                                 shell=True,
                                 stdout = subprocess.PIPE,
                                 stderr = subprocess.PIPE,
                                 close_fds=True)

    out, err = wing_proc.communicate()

    if wing_proc.returncode != 0:
        _LOGGER.error(f'Wing did not start ...')
        _LOGGER.error(f'{out}')
        _LOGGER.error(f'{err}')
        return None
    else:
        _LOGGER.info(f'Success: Wing started correctly!')

    return wing_proc

# the above method works, so will deprecate this call version
# should find out if a run variant is the prefered way to go py3?

# def call(exe = wing_config.settings.WING_EXE,
#          project_file = wing_config.settings.WING_PROJ):
#     """Method to call, to start Wing IDE
#     """
#
#     _LOGGER.debug(f'Attempting to start wing ...')
#
#     try:
#         wing_proc = subprocess.call([exe, project_file],
#                                      env = wing_env,
#                                      shell=True,
#                                      close_fds=True)
#     except Exception as e:
#         _LOGGER.error(f'Wing did not start ...')
#         _LOGGER.error(f'{err}')
#         return None
#
#     else:
#         _LOGGER.info(f'Success: Wing started correctly!')
#
#     return wing_proc
# --- END -----------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main (external commandline)"""

    _MODULENAME = f'{_MODULENAME}.cli'

    from DccScriptingInterface.globals import *

    from DccScriptingInterface.constants import STR_CROSSBAR
    from DccScriptingInterface.constants import FRMT_LOG_LONG

    # configure basic logger
    # note: not using a common logger to reduce cyclical imports
    _logging.basicConfig(level=DCCSI_LOGLEVEL,
                         format=FRMT_LOG_LONG,
                         datefmt='%m-%d %H:%M')

    _LOGGER = _logging.getLogger(_MODULENAME)

    # log global state to cli
    _LOGGER.debug(STR_CROSSBAR)
    _LOGGER.debug(f'_MODULENAME: {_MODULENAME}')
    _LOGGER.debug(f'{ENVAR_DCCSI_GDEBUG}: {wing_config.settings.DCCSI_GDEBUG}')
    _LOGGER.debug(f'{ENVAR_DCCSI_DEV_MODE}: {wing_config.settings.DCCSI_DEV_MODE}')
    _LOGGER.debug(f'{ENVAR_DCCSI_LOGLEVEL}: {wing_config.settings.DCCSI_LOGLEVEL}')

    # commandline interface
    import argparse
    parser = argparse.ArgumentParser(
        description=f'O3DE {_MODULENAME}',
        epilog=(f"Attempts to start Wing Pro {wing_config.settings.DCCSI_WING_VERSION_MAJOR}"
                "with the DCCsi and O3DE bootstrapping"))

    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables global debug flag.')

    parser.add_argument('-ex', '--exit',
                        type=bool,
                        required=False,
                        default=False,
                        help='Exits python. Do not exit if you want to be in interactive interpreter after config')

    args = parser.parse_args()

    # easy overrides
    if args.global_debug:
        # you can modify/oerrive any setting simply by re-adding it with new values
        wing_config.add_setting(ENVAR_DCCSI_GDEBUG, True)

    # fetch modified settings and set the env
    settings = wing_config.get_settings(set_env=True)

    try:
        wing_proc = popen(wing_config.settings.WING_EXE, wing_config.settings.WING_PROJ)
    except Exception as e:
        _LOGGER.warning(f'Could not start Wing')
        _LOGGER.error(f'{e} , traceback =', exc_info=True)
        if DCCSI_STRICT:
            _LOGGER.exception(f'{e} , traceback =', exc_info=True)
            raise e

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
