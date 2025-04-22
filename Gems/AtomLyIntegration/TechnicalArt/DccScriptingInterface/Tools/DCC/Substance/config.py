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
    < DCCsi >:: Tools//DCC//Substance//config.py

This module manages the dynamic config and settings for bootstrapping
Adobe Substance Designer with o3de inter-op, scripts, etc.

This like the core <DCCsi>/config.py uses Dynaconf:
1. Modular rather then monolithic (single DCC integrations can stand on their own)
2. Layered config and settings (the core can be extended or replaced)
3. Derive or alter env via logic
4. DCC integration code and comments are written for a novice TA audience
5. etc. (to do: expand on the benefits)

"""
# -------------------------------------------------------------------------
import timeit
_MODULE_START = timeit.default_timer()  # start tracking

# standard imports
import sys
import os
import site
import re
import inspect
import importlib.util
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------

# This module and others like Blender/config.py have a lot of duplicate
# boilerplate code (as we are early, these are the first versions to stand up)
# They could possibly be improved by unifying into a Class object designed
# with inheritance extensibility

# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'Tools.DCC.Substance.config'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')

_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

# This sets up basic code access to the DCCsi
# <o3de>/Gems/AtomLyIntegration/TechnicalArt/<DCCsi>
_PATH_DCCSIG = Path(_MODULE_PATH, '../../../..').resolve()
site.addsitedir(_PATH_DCCSIG.as_posix())

# set envar so DCCsi synthetic env bootstraps with it (config.py)
from azpy.constants import ENVAR_PATH_DCCSIG
os.environ[ENVAR_PATH_DCCSIG] = str(_PATH_DCCSIG.as_posix())
_LOGGER.debug(f'PATH_DCCSIG: {_PATH_DCCSIG.as_posix()}')

# now we have dccsi azpy api access
import azpy.config_utils

# these must be imported explicitly, they are not defined in __all__
from azpy.config_utils import ENVAR_DCCSI_GDEBUG
from azpy.config_utils import ENVAR_DCCSI_DEV_MODE
from azpy.config_utils import ENVAR_DCCSI_LOGLEVEL
from azpy.config_utils import ENVAR_DCCSI_GDEBUGGER
from azpy.config_utils import FRMT_LOG_LONG
from azpy.config_utils import STR_CROSSBAR

# defaults, can be overridden/forced here for development
# they should be committed in an off/False state
from azpy.env_bool import env_bool
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_LOGLEVEL = env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO)
_DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')
# you can also set in a persistent manner externally
# method 1: set envar within Env_Dev.bat file
#    this would set in env prior to running anything within that env context
# method 2: override in settings.local.json
#    this is the preferred manner, dynaconf will always load this by default
#    so these values will always be last and take precedence within code
#    that is executed after:
#        from dynaconf import settings
#        settings.setenv()

# this will bootstrap access to the dccsi managed package dependencies
# <DCCsi>\3rdParty\Python\Lib\3.x\3.x.x (based on python version)
_PATH_DCCSI_PYTHON_LIB = azpy.config_utils.bootstrap_dccsi_py_libs()
site.addsitedir(_PATH_DCCSI_PYTHON_LIB.as_posix())
# ^ we don't add this to dynaconf env, or it will end up in settings.local.json
# and we don't want that, since this LIB location is transient/procedural
# to ensure we are always bootstrapping the correct path based on python version running
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# early debugging
if _DCCSI_DEV_MODE:
    azpy.config_utils.attach_debugger(_DCCSI_GDEBUGGER)

# This will import and retrieve the core <dccsi>/config.py and settings
_DCCSI_CORE_CONFIG = azpy.config_utils.get_dccsi_config(_PATH_DCCSIG)

# now standalone we can validate the config, env, settings.
_SETTINGS = _DCCSI_CORE_CONFIG.get_config_settings(enable_o3de_python=False,
                                                  enable_o3de_pyside2=False,
                                                  set_env=True)
# we don't init the O3DE python env settings!
# that will cause conflicts with the DCC tools python!!!
# we are also not enabling the O3DE PySide2 (aka QtForPython) access
# this can cause GUI and boot failure with Qt based applications (Maya, Substance3D, etc.)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
# start locally prepping known default values for dynamic environment settings
# these global variables are passed as defaults into methods within the module

# special, a global home for stashing PATHs for managed settings
_DCCSI_SYS_PATH = _DCCSI_CORE_CONFIG._DCCSI_SYS_PATH

# special, a global home for stashing PYTHONPATHs for managed settings
_DCCSI_PYTHONPATH = _DCCSI_CORE_CONFIG._DCCSI_PYTHONPATH

# special, stash local PYTHONPATHs in a non-managed way (won't end up in settings.local.json)
_DCCSI_PYTHONPATH_EXCLUDE = _DCCSI_CORE_CONFIG._DCCSI_PYTHONPATH_EXCLUDE

# this is a dict bucket to store unmanaged settings (fully local to module)
_DCCSI_LOCAL_SETTINGS = _DCCSI_CORE_CONFIG._DCCSI_LOCAL_SETTINGS
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# grab imports from Substance\constants.py that we will use
from Tools.DCC.Substance.constants import ENVAR_DCCSI_TOOLS_SUBSTANCE
from Tools.DCC.Substance.constants import PATH_DCCSI_TOOLS_SUBSTANCE

from Tools.DCC.Substance.constants import ENVAR_PATH_DCCSI_TOOLS
from Tools.DCC.Substance.constants import PATH_PATH_DCCSI_TOOLS

from Tools.DCC.Substance.constants import ENVAR_DCCSI_SUBSTANCE_RESOURCES
from Tools.DCC.Substance.constants import PATH_DCCSI_SUBSTANCE_RESOURCES

from Tools.DCC.Substance.constants import ENVAR_DCCSI_SUBSTANCE_LOCATION
from Tools.DCC.Substance.constants import PATH_DCCSI_SUBSTANCE_LOCATION

from Tools.DCC.Substance.constants import ENVAR_DCCSI_SUBSTANCE_EXE
from Tools.DCC.Substance.constants import PATH_DCCSI_SUBSTANCE_EXE

from Tools.DCC.Substance.constants import ENVAR_DCCSI_SUBSTANCE_PYTHON
from Tools.DCC.Substance.constants import PATH_DCCSI_SUBSTANCE_PYTHON

from Tools.DCC.Substance.constants import ENVAR_DCCSI_SUBSTANCE_PY_EXE
from Tools.DCC.Substance.constants import PATH_DCCSI_SUBSTANCE_PY_EXE

from Tools.DCC.Substance.constants import ENVAR_DCCSI_IDE_PY_SUBSTANCE
from Tools.DCC.Substance.constants import PATH_DCCSI_IDE_PY_SUBSTANCE

from Tools.DCC.Substance.constants import ENVAR_DCCSI_SUBSTANCE_SET_CALLBACKS
from Tools.DCC.Substance.constants import DCCSI_SUBSTANCE_SET_CALLBACKS

from Tools.DCC.Substance.constants import ENVAR_DCCSI_SUBSTANCE_SCRIPTS
from Tools.DCC.Substance.constants import PATH_DCCSI_SUBSTANCE_SCRIPTS

from Tools.DCC.Substance.constants import ENVAR_DCCSI_SUBSTANCE_CFG
from Tools.DCC.Substance.constants import PATH_DCCSI_SUBSTANCE_CFG
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_config_settings(core_config=_DCCSI_CORE_CONFIG,
                        settings=_SETTINGS,
                        dccsi_sys_path=_DCCSI_SYS_PATH,
                        dccsi_pythonpath=_DCCSI_PYTHONPATH,
                        set_env=True):
    # now we can extend the environment specific to Substance
    # start by grabbing the constants we want to work with as envars
    # import others

    time_start = timeit.default_timer()  # start tracking for perf

    # we don't do this often but we want to stash to global dict
    # global _DCCSI_LOCAL_SETTINGS # non-dynaconf managed settings

    # This extends the settings and environment with Substance3d stuff
    os.environ[f"DYNACONF_{ENVAR_DCCSI_SUBSTANCE_SET_CALLBACKS}"] = str(DCCSI_SUBSTANCE_SET_CALLBACKS)

    os.environ[f"DYNACONF_{ENVAR_DCCSI_TOOLS_SUBSTANCE}"] = PATH_DCCSI_TOOLS_SUBSTANCE.as_posix()
    os.environ[f"DYNACONF_{ENVAR_PATH_DCCSI_TOOLS}"] = PATH_PATH_DCCSI_TOOLS.as_posix()
    os.environ[f"DYNACONF_{ENVAR_DCCSI_SUBSTANCE_RESOURCES}"] = PATH_DCCSI_SUBSTANCE_RESOURCES.as_posix()
    os.environ[f"DYNACONF_{ENVAR_DCCSI_SUBSTANCE_LOCATION}"] = PATH_DCCSI_SUBSTANCE_LOCATION.as_posix()
    os.environ[f"DYNACONF_{ENVAR_DCCSI_SUBSTANCE_EXE}"] = PATH_DCCSI_SUBSTANCE_EXE.as_posix()
    os.environ[f"DYNACONF_{ENVAR_DCCSI_SUBSTANCE_PYTHON}"] = PATH_DCCSI_SUBSTANCE_PYTHON.as_posix()
    os.environ[f"DYNACONF_{ENVAR_DCCSI_SUBSTANCE_PY_EXE}"] = PATH_DCCSI_SUBSTANCE_PY_EXE.as_posix()
    os.environ[f"DYNACONF_{ENVAR_DCCSI_IDE_PY_SUBSTANCE}"] = PATH_DCCSI_IDE_PY_SUBSTANCE.as_posix()
    os.environ[f"DYNACONF_{ENVAR_DCCSI_SUBSTANCE_SCRIPTS}"] = PATH_DCCSI_SUBSTANCE_SCRIPTS.as_posix()
    os.environ[f"DYNACONF_{ENVAR_DCCSI_SUBSTANCE_CFG}"] = PATH_DCCSI_SUBSTANCE_CFG.as_posix()

    # appends paths lists locally
    dccsi_sys_path.append(PATH_DCCSI_SUBSTANCE_LOCATION.as_posix())
    dccsi_sys_path.append(PATH_DCCSI_SUBSTANCE_PYTHON.as_posix())

    dccsi_pythonpath.append(PATH_DCCSI_SUBSTANCE_SCRIPTS.as_posix())

    # packs paths lists to DCCSI_ managed envars
    core_config.add_path_list_to_envar(dccsi_sys_path, 'DYNACONF_DCCSI_SYS_PATH')

    # to do: might be a bug here, not writing to setting.local.json
    core_config.add_path_list_to_envar(dccsi_pythonpath, 'DYNACONF_DCCSI_PYTHONPATH')

    # now standalone we can validate the config. env, settings.
    from dynaconf import settings

    if set_env:
        settings.setenv()

        # final stage, if we have managed path lists set them
        core_config.add_path_list_to_envar(dccsi_sys_path)
        core_config.add_path_list_to_addsitedir(dccsi_pythonpath)

    time_complete = timeit.default_timer() - time_start
    _LOGGER.info('~   config.init_o3de_core() DONE: {} sec'.format(time_complete))

    return settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_MODULE_END = timeit.default_timer() - _MODULE_START
_LOGGER.debug(f'{_MODULENAME} took: {_MODULE_END} sec')
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone cli script for testing/debugging"""

    main_start = timeit.default_timer()  # start tracking

    _MODULENAME = 'DCCsi.Tools.DCC.Substance.config.cli'

    import pathlib

    from azpy.env_bool import env_bool
    # temp internal debug flag, toggle values for manual testing
    _DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
    _DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
    _DCCSI_LOGLEVEL = env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO)
    _DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')

    # default loglevel to info unless set
    _DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))
    if _DCCSI_GDEBUG:
        # override loglevel if running debug
        _DCCSI_LOGLEVEL = _logging.DEBUG

    # configure basic logger
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                        format=FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')

    _LOGGER = _logging.getLogger(_MODULENAME)

    from azpy.constants import STR_CROSSBAR

    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('~ {}.py ... Running script as __main__'.format(_MODULENAME))
    _LOGGER.info(STR_CROSSBAR)

    # go ahead and run the rest of the configuration
    # parse the command line args
    import argparse
    parser = argparse.ArgumentParser(
        description='O3DE DCCsi Dynamic Config (dynaconf) for Substance',
        epilog="Attempts to determine O3DE project if -pp not set")

    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        help='Enables global debug flag.')

    parser.add_argument('-dm', '--developer-mode',
                        type=bool,
                        required=False,
                        help='Enables dev mode for early auto attaching debugger.')

    parser.add_argument('-sd', '--set-debugger',
                        type=str,
                        required=False,
                        default='WING',
                        help='(NOT IMPLEMENTED) Default debugger: WING, others: PYCHARM and VSCODE.')

    parser.add_argument('-pp', '--project-path',
                        type=pathlib.Path,
                        required=False,
                        default=Path('{ to do: implement }'),
                        help='(NOT IMPLEMENTED) The path to the project.')

    parser.add_argument('-qt', '--enable-qt',
                        type=bool,
                        required=False,
                        help='(NOT IMPLEMENTED) Enables O3DE Qt & PySide2 access.')

    parser.add_argument('-tp', '--test-pyside2',
                        type=bool,
                        required=False,
                        help='(NOT IMPLEMENTED) Runs Qt/PySide2 tests and reports.')

    parser.add_argument('-pc', '--project-config',
                        type=bool,
                        required=False,
                        help='(not implemented) Enables reading the < >project >/registry/dccsi_configuration.setreg.')

    parser.add_argument('-cls', '--cache-local-settings',
                        type=bool,
                        required=False,
                        default=True,
                        help='Ensures setting.local.json is written to cache settings')

    parser.add_argument('-es', '--export-settings',
                        type=pathlib.Path,
                        required=False,
                        help='(Not Tested) Writes managed settings to specified path.')

    parser.add_argument('-ls', '--load-settings',
                        type=pathlib.Path,
                        required=False,
                        help='(Not Implemented) Would load and read settings from a specified path.')

    parser.add_argument('-ec', '--export-configuration',
                        type=bool,
                        required=False,
                        help='(not implemented) writes settings as a O3DE < project >/registry/dccsi_configuration.setreg.')

    parser.add_argument('-lps', '--log-print-settings',
                        type=bool,
                        required=False,
                        default=True,
                        help='Will dump settings results into the log.')

    parser.add_argument('-ipd', '--install-package-dependencies',
                        type=bool,
                        required=False,
                        default=True,
                        help='Will install/update the DCCsi python package dependencies.')

    parser.add_argument('-ex', '--exit',
                        type=bool,
                        required=False,
                        help='(NOT IMPLEMENTED) Exits python. Do not exit if you want to be in interactive interpreter after config')

    args = parser.parse_args()

    from azpy.shared.utils.arg_bool import arg_bool

    # easy overrides
    if arg_bool(args.global_debug, desc="args.global_debug"):
        from azpy.constants import ENVAR_DCCSI_GDEBUG
        _DCCSI_GDEBUG = True
        _LOGGER.setLevel(_logging.DEBUG)
        _LOGGER.info(f'Global debug is set, {ENVAR_DCCSI_GDEBUG}={_DCCSI_GDEBUG}')

    if arg_bool(args.developer_mode, desc="args.developer_mode"):
        _DCCSI_DEV_MODE = True  # session based
        azpy.config_utils.attach_debugger()  # attempts to start debugger

    if args.set_debugger:
        _LOGGER.info('Setting and switching debugger type not implemented (default=WING)')
        # To Do: implement dev module with debugger plugin pattern

    # now standalone we can validate the config. env, settings.
    # settings = get_config_settings(stub) # To Do: pipe in CLI
    settings = get_config_settings(core_config=_DCCSI_CORE_CONFIG,
                                   settings=_SETTINGS,
                                   dccsi_sys_path=_DCCSI_SYS_PATH,
                                   dccsi_pythonpath=_DCCSI_PYTHONPATH,
                                   set_env=True)

    if _DCCSI_GDEBUG or args.cache_local_settings:
        if args.export_settings:
            export_settings_path = Path(args.export_settings).resolve()
            _DCCSI_CORE_CONFIG.export_settings(settings=settings,
                                               settings_filepath=export_settings_path.as_posix())
        else:
            export_settings_path = Path(PATH_DCCSI_TOOLS_SUBSTANCE,
                                        'settings.local.json').resolve()
            _DCCSI_CORE_CONFIG.export_settings(settings=settings,
                                               settings_filepath=export_settings_path.as_posix())

            # this should be set if there are local settings!? to do: circle back
            _LOGGER.debug('DCCSI_LOCAL_SETTINGS: {}'.format(settings.DCCSI_LOCAL_SETTINGS))

    # handle dumping logging of settings
    if arg_bool(args.log_print_settings, desc="args.log_print_settings"):
        # format the setting, Box is an orderedDict object with convenience methods
        from box import Box
        _settings_box = Box(settings.as_dict())

        _LOGGER.info('Logging the Substance Dynaconf settings object ...')
        _LOGGER.info(str(_settings_box.to_json(sort_keys=True,
                                               indent=4)))

    if arg_bool(args.install_package_dependencies, desc="args.install_package_dependencies"):
        import foundation
        _LOGGER.info('Installing python package dependencies from requirements.txt')
        foundation.install_requirements(python_exe=settings.DCCSI_SUBSTANCE_PY_EXE)
        _LOGGER.warning('If the Substance3D app is running we suggest restarting it')

    # custom prompt
    sys.ps1 = f"[{_MODULENAME}]>>"

    _MODULE_END = timeit.default_timer() - _MODULE_START
    _LOGGER.info(f'{_MODULENAME} took: {_MODULE_END} sec')

    if args.exit:
        # return
        sys.exit()
# --- END -----------------------------------------------------------------
