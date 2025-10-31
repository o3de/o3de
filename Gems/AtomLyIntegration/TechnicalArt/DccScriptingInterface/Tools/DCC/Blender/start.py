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
"""! O3DE DCCsi Blender DCC App start module

There are severl ways this can be used:
1. From cli
    - open a cmd
    - change directory > cd c:\\path\\to\\DccScriptingInterface
    - run command > python Tools\\DCC\\Blender\\start.py

2. The O3DE editor uses this module to launch from menu and editor systems

:file: DccScriptingInterface\\Tools\\DCC\\Blender\\start.py
:Status: Prototype
:Version: 0.0.1
:Entrypoint: entrypoint, configures logging, includes cli
:Notice:
    Currently windows only (not tested on other platforms)
    Currently only tested with Blender 3.0
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
# global scope
_MODULE_PATH = Path(__file__)
PATH_O3DE_TECHART_GEMS = _MODULE_PATH.parents[4].resolve()
os.chdir(PATH_O3DE_TECHART_GEMS.as_posix())
sys.path.insert(0, PATH_O3DE_TECHART_GEMS.as_posix())

from DccScriptingInterface import add_site_dir
add_site_dir(PATH_O3DE_TECHART_GEMS) # cleaner add

from DccScriptingInterface.Tools.DCC.Blender import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.start'

# get the global dccsi state
from DccScriptingInterface.globals import *
from DccScriptingInterface import add_site_dir

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

# this should execute the core config.py first and grab settings
from DccScriptingInterface.Tools.DCC.Blender.config import blender_config
_settings = blender_config.get_config_settings()
_settings.setenv() # ensure env is set

_BLENDER_EXE = Path(_settings.PATH_DCCSI_BLENDER_EXE).resolve(strict=True)

_BLENDER_SCRIPTS = Path(_settings.PATH_DCCSI_BLENDER_SCRIPTS).resolve(strict=True)
add_site_dir(_BLENDER_SCRIPTS)
from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_BLENDER_USER_SCRIPTS
os.environ[ENVAR_BLENDER_USER_SCRIPTS] = _BLENDER_SCRIPTS.as_posix()

_DEFAULT_BOOTSTRAP = Path(_settings.PATH_DCCSI_BLENDER_BOOTSTRAP).resolve(strict=True)

from DccScriptingInterface.azpy.config_utils import check_is_ascii

# store a copy, so we can inspect/compare later
orig_env = os.environ.copy()

# we are going to pass the system environ
blender_env = os.environ.copy()

# prunes non-string key:value envars
blender_env = {key: value for key, value in blender_env.items() if check_is_ascii(key) and check_is_ascii(value)}

if DCCSI_GDEBUG:
    # we can see what was pruned
    pruned = {k: blender_env[k] for k in set(blender_env) - set(orig_env)}

    if len(pruned.items()):
        _LOGGER.debug(f'prune diff is ...')
        for p in pruned.items():
            _LOGGER.debug(f'{p}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
#https://tinyurl.com/o3de-dccsi-blender-cli-help
# command args but be seperated properly
#https://blender.stackexchange.com/questions/169259/issue-running-blender-command-line-arguments-using-python-subprocess
# https://docs.blender.org/manual/en/latest/advanced/command_line/arguments.html

# some notes
# from cmd, use a startup script (we should be able to use to bootstrap)
#    ./blender --python [myscript.py]

# from cmd, enable addons, then load file:
#    ./blender -b --addons animation_nodes,meshlint [file]
# we don't put our AddOns directly into Blender
# we boostrap access to them.

# automation
# from cmd, enable addons, load file, start script
#    ./blender -b --addons animation_nodes,meshlint [file] --python [myscript.py]

# default launch command
_LAUNCH_COMMAND = [f'{str(_BLENDER_EXE)}',
                   f'--python', # this must be seperate from the .py file
                   f'{str(_DEFAULT_BOOTSTRAP)}']

# suggestion for future PR is to refactor this method into something like
# DccScriptingInterface.azpy.utils.start.popen()
def popen(command: list = _LAUNCH_COMMAND,
          env: dict = blender_env) -> subprocess:

    f"""Method call to start the DCC app {_PACKAGENAME}"""

    _LOGGER.info(f'Attempting to start {_PACKAGENAME} ...')
    _LOGGER.info(f'Command args: {command}')

    process = subprocess.Popen(args = command,
                               env = env,
                               shell=True,
                               stdout = subprocess.PIPE,
                               stderr = subprocess.PIPE,
                               close_fds=True)

    out, err = process.communicate()

    if process.returncode != 0:
        _LOGGER.error(f'{_PACKAGENAME} did not start ...')
        _LOGGER.error(f'{out}')
        _LOGGER.error(f'{err}')
        return None
    else:
        _LOGGER.info(f'Success: {_PACKAGENAME} started correctly!')

    return process
# -------------------------------------------------------------------------


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
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.debug(f'_MODULENAME: {_MODULENAME}')
    _LOGGER.debug(f'{ENVAR_DCCSI_GDEBUG}: {DCCSI_GDEBUG}')
    _LOGGER.debug(f'{ENVAR_DCCSI_DEV_MODE}: {DCCSI_DEV_MODE}')
    _LOGGER.debug(f'{ENVAR_DCCSI_LOGLEVEL}: {DCCSI_LOGLEVEL}')

    # commandline interface
    import argparse
    parser = argparse.ArgumentParser(
        description=f'O3DE {_MODULENAME}',
        epilog=(f"Attempts to start Blender with the DCCsi and O3DE bootstrapping"))

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

    try:
        process = popen(command = _LAUNCH_COMMAND,
                        env = blender_env)
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
