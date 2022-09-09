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
import site
import inspect
import subprocess
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
#os.environ['PYTHONINSPECT'] = 'True'
# global scope
_MODULENAME = 'DCCsi.Tools.DCC.Blender.start'
_MODULE_PATH = Path(__file__)

# this is an entry point, we must self bootstrap
PATH_O3DE_TECHART_GEMS = _MODULE_PATH.parents[4].resolve()
os.chdir(PATH_O3DE_TECHART_GEMS.as_posix())

#sys.path.append(PATH_O3DE_TECHART_GEMS.as_posix())
from DccScriptingInterface import add_site_dir
add_site_dir(PATH_O3DE_TECHART_GEMS)

# get the global dccsi state
from DccScriptingInterface.globals import *

from azpy.constants import FRMT_LOG_LONG
_logging.basicConfig(level=_logging.DEBUG,
                     format=FRMT_LOG_LONG,
                    datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

# retreive the blender_config class object and it's settings
from DccScriptingInterface.Tools.DCC.Blender.config import blender_config

# Blender config needs to be refactored to use ConfigClass in future PR
# See DccScriptingInterface.Tools.IDE.Wing.config
# blender_config.settings.setenv() # ensure env is set
# use legacy code for now
settings = blender_config.get_config_settings()

from DccScriptingInterface.azpy.config_utils import check_is_ascii

# store a copy, so we can inspect/compare later
orig_env = os.environ.copy()

# we are going to pass the system environ
blender_env = os.environ.copy()

# prunes non-string key:value envars
blender_env = {key: value for key, value in blender_env.items() if check_is_ascii(key) and check_is_ascii(value)}

# will prune QT_ envars, to be used with QT bases apps like Maya or Wing
# Blender is not QT based, so we should be able to disable this
#blender_env = {key: value for key, value in blender_env.items() if not key.startswith("QT_")}

if DCCSI_GDEBUG:
    # we can see what was pruned
    pruned = {k: blender_env[k] for k in set(blender_env) - set(orig_env)}

    if len(pruned.items()):
        _LOGGER.debug(f'prune diff is ...')
        for p in pruned.items():
            _LOGGER.debug(f'{p}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# https://docs.blender.org/manual/en/ latest / advanced / command_line / arguments.html

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
_LAUNCH_COMMAND = [blender_exe, f"--python {bootstrap}"]

# suggestion for future PR is to refactor this method into something like
# DccScriptingInterface.azpy.utils.start.popen()

def popen(env: dict = blender_env,
          launch_command: list = _LAUNCH_COMMAND,
          app_name: str = 'Blender') -> subprocess:

    f"""Method call to start the DCC app {app_name}"""

    _LOGGER.debug(f'Attempting to start {app_name} ...')

    process = subprocess.Popen(launch_command,
                               env = env,
                               shell=True,
                               stdout = subprocess.PIPE,
                               stderr = subprocess.PIPE,
                               close_fds=True)

    out, err = process.communicate()

    if wing_proc.returncode != 0:
        _LOGGER.error(f'{app_name} did not start ...')
        _LOGGER.error(f'{err}')
        return None
    else:
        _LOGGER.info(f'Success: {app_name} started correctly!')

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
    _LOGGER.debug(STR_CROSSBAR)
    _LOGGER.debug(f'_MODULENAME: {_MODULENAME}')
    _LOGGER.debug(f'{ENVAR_DCCSI_GDEBUG}: {wing_config.settings.DCCSI_GDEBUG}')
    _LOGGER.debug(f'{ENVAR_DCCSI_DEV_MODE}: {wing_config.settings.DCCSI_DEV_MODE}')
    _LOGGER.debug(f'{ENVAR_DCCSI_LOGLEVEL}: {wing_config.settings.DCCSI_LOGLEVEL}')

    # commandline interface
    import argparse
    parser = argparse.ArgumentParser(
        description=f'O3DE {_MODULENAME}',
        epilog=(f"Attempts to start {}"
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
