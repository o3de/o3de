#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! This module manages the dynamic config and settings for bootstrapping
Blender DCC app integration with o3de inter-op, scripts, extensions, etc.

:file: DccScriptingInterface\\Tools\\DCC\\Blender\\config.py
:Status: Prototype
:Version: 0.0.1
:Future: is unknown
:Notice:
"""
# -------------------------------------------------------------------------
import timeit
_MODULE_START = timeit.default_timer()  # start tracking

# standard imports
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------

# This module and others like Substance/config.py have a lot of duplicate
# boilerplate code (as we are early, these are the first versions to stand up)
# They could possibly be improved by unifying into a Class object designed
# with extensibility

# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.Tools.DCC.Blender import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.config'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

# ensure dccsi and o3de core access
# in a future iteration it is suggested that the core config
# be rewritten from ConfigClass, then BlenderConfig inherits core
import DccScriptingInterface.config as dccsi_core_config

_settings_core = dccsi_core_config.get_config_settings(enable_o3de_python=True,
                                                       enable_o3de_pyside2=False,
                                                       set_env=True)

# local dccsi imports
# this accesses common global state, e.g. DCCSI_GDEBUG (is True or False)
from DccScriptingInterface.globals import *

# this will auto-attach ide debugging at the earliest possible point in module
from azpy.config_utils import attach_debugger
if DCCSI_DEV_MODE: # from DccScriptingInterface.globals
    attach_debugger(debugger_type=DCCSI_GDEBUGGER)

# if the dccsi core config and it's settings are loaded this should pass
try:
    _settings_core.DCCSI_CONFIG_CORE
except EnvironmentError as e:
    _LOGGER.error('Setting does not exist: DCCSI_CONFIG_CORE')
    _LOGGER.warning(f'EnvironmentError: {e}')

# this is the root path for the wing pkg
from DccScriptingInterface.Tools.DCC.Blender import ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER
from DccScriptingInterface.Tools.DCC.Blender import PATH_DCCSI_TOOLS_DCC_BLENDER
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# now we build the wing config class
from DccScriptingInterface.azpy.config_class import ConfigClass

# blender_config is a class object of BlenderConfig
# BlenderConfig is a child class of ConfigClass
class BlenderConfig(ConfigClass):
    """Extend ConfigClass with new blender functionality"""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        _LOGGER.info(f'Initializing: {self.get_classname()}')

# build config object
blender_config = BlenderConfig(config_name='dccsi_dcc_blender', auto_set=True)

# in another module someone could work this way
# from DccScriptingInterface.Tools.IDE.Wing.config import wing_config
# settings = wing_config.get_settings(set_env=True)
# or
# if wing_config.settings.THIS_SETTING: do this

# now we can extend the environment specific to Blender
# start by grabbing the constants we want to work with as envars
# a managed setting to track the wing config is enabled
from Tools.DCC.Blender.constants import ENVAR_DCCSI_CONFIG_DCC_BLENDER
blender_config.add_setting(ENVAR_DCCSI_CONFIG_DCC_BLENDER, True)

from Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_TOOLS
from Tools.DCC.Blender.constants import PATH_PATH_DCCSI_TOOLS
PATH_PATH_DCCSI_TOOLS = Path(PATH_PATH_DCCSI_TOOLS).resolve()
blender_config.add_setting(ENVAR_PATH_DCCSI_TOOLS,
                           PATH_PATH_DCCSI_TOOLS.as_posix())

from Tools.DCC.Blender.constants import ENVAR_DCCSI_TOOLS_BLENDER
from Tools.DCC.Blender.constants import PATH_DCCSI_TOOLS_BLENDER
PATH_DCCSI_TOOLS_BLENDER = Path(PATH_DCCSI_TOOLS_BLENDER).resolve()
blender_config.add_setting(ENVAR_DCCSI_TOOLS_BLENDER,
                           PATH_DCCSI_TOOLS_BLENDER.as_posix())

from Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_BLENDER_SCRIPTS
from Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_SCRIPTS
PATH_DCCSI_BLENDER_SCRIPTS = Path(PATH_DCCSI_BLENDER_SCRIPTS).resolve()
blender_config.add_setting(ENVAR_PATH_DCCSI_BLENDER_SCRIPTS,
                           PATH_DCCSI_BLENDER_SCRIPTS.as_posix(),
                           set_sys_path=True,
                           set_pythonpath=True)

from Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_BLENDER_LOC
from Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_LOC
PATH_DCCSI_BLENDER_LOC = Path(PATH_DCCSI_BLENDER_LOC).resolve()
blender_config.add_setting(ENVAR_PATH_DCCSI_BLENDER_LOC,
                           PATH_DCCSI_BLENDER_LOC.as_posix(),
                           set_sys_path=True)

from Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_BLENDER_EXE
from Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_EXE
PATH_DCCSI_BLENDER_EXE = Path(PATH_DCCSI_BLENDER_EXE).resolve()
blender_config.add_setting(ENVAR_PATH_DCCSI_BLENDER_EXE,
                           PATH_DCCSI_BLENDER_EXE.as_posix())

from Tools.DCC.Blender.constants import ENVAR_DCCSI_BLENDER_LAUNCHER_EXE
from Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_LAUNCHER_EXE
PATH_DCCSI_BLENDER_LAUNCHER_EXE = Path(PATH_DCCSI_BLENDER_LAUNCHER_EXE).resolve()
blender_config.add_setting(ENVAR_DCCSI_BLENDER_LAUNCHER_EXE,
                           PATH_DCCSI_BLENDER_LAUNCHER_EXE.as_posix())

from Tools.DCC.Blender.constants import ENVAR_DCCSI_BLENDER_PYTHON_LOC
from Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_PYTHON_LOC
PATH_DCCSI_BLENDER_PYTHON_LOC = Path(PATH_DCCSI_BLENDER_PYTHON_LOC).resolve()
blender_config.add_setting(ENVAR_DCCSI_BLENDER_PYTHON_LOC,
                           PATH_DCCSI_BLENDER_PYTHON_LOC.as_posix(),
                           set_sys_path=True)

from Tools.DCC.Blender.constants import ENVAR_DCCSI_BLENDER_PY_EXE
from Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_PY_EXE

from Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_BLENDER_BOOTSTRAP
from Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_BOOTSTRAP

from Tools.DCC.Blender.constants import URL_DCCSI_BLENDER_WIKI
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_config_settings(core_config=_DCCSI_CORE_CONFIG,
                        settings=_SETTINGS,
                        dccsi_sys_path=_DCCSI_SYS_PATH,
                        dccsi_pythonpath=_DCCSI_PYTHONPATH,
                        set_env=True):
    # now we can extend the environment specific to Blender
    # start by grabbing the constants we want to work with as envars
    # import others

    time_start = timeit.default_timer()  # start tracking for perf

    # we don't do this often but we want to stash to global dict
    # global _DCCSI_LOCAL_SETTINGS # non-dynaconf managed settings

    # This extends the settings and environment with Subtance3d stuff
    os.environ[f"DYNACONF_{URL_DCCSI_BLENDER_WIKI}"] = URL_DCCSI_BLENDER_WIKI

    os.environ[f"DYNACONF_{ENVAR_PATH_DCCSI_TOOLS}"] = PATH_PATH_DCCSI_TOOLS.as_posix()
    os.environ[f"DYNACONF_{ENVAR_DCCSI_TOOLS_BLENDER}"] = PATH_DCCSI_TOOLS_BLENDER.as_posix()
    os.environ[f"DYNACONF_{ENVAR_PATH_DCCSI_BLENDER_SCRIPTS}"] = PATH_DCCSI_BLENDER_SCRIPTS.as_posix()
    os.environ[f"DYNACONF_{ENVAR_PATH_DCCSI_BLENDER_LOC}"] = PATH_DCCSI_BLENDER_LOC.as_posix()
    os.environ[f"DYNACONF_{ENVAR_PATH_DCCSI_BLENDER_EXE}"] = PATH_DCCSI_BLENDER_EXE.as_posix()
    os.environ[f"DYNACONF_{ENVAR_DCCSI_BLENDER_LAUNCHER_EXE}"] = PATH_DCCSI_BLENDER_LAUNCHER_EXE.as_posix()
    os.environ[f"DYNACONF_{ENVAR_DCCSI_BLENDER_PYTHON_LOC}"] = PATH_DCCSI_BLENDER_PYTHON_LOC.as_posix()
    os.environ[f"DYNACONF_{ENVAR_DCCSI_BLENDER_PY_EXE}"] = PATH_DCCSI_BLENDER_PY_EXE.as_posix()
    os.environ[f"DYNACONF_{ENVAR_PATH_DCCSI_BLENDER_BOOTSTRAP}"] = PATH_DCCSI_BLENDER_BOOTSTRAP.as_posix()

    # appends paths lists locally
    dccsi_sys_path.append(PATH_DCCSI_BLENDER_LOC.as_posix())
    dccsi_sys_path.append(PATH_DCCSI_BLENDER_PYTHON_LOC.as_posix())

    dccsi_pythonpath.append(PATH_DCCSI_BLENDER_SCRIPTS.as_posix())

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

    _MODULENAME = 'DCCsi.Tools.DCC.Blender.config.cli'

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
    # note: not using a common logger to reduce cyclical imports
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
        description='O3DE DCCsi Dynamic Config (dynaconf) for Blender',
        epilog="Attempts to determine O3DE project if -pp not set")

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
                        help='(NOT IMPLEMENTED) Default debugger: WING, others: PYCHARM and VSCODE.')

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
                        default=False,
                        help='(Not Implemented) Would load and read settings from a specified path.')

    parser.add_argument('-ec', '--export-configuration',
                        type=bool,
                        required=False,
                        default=False,
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
                        default=False,
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
            export_settings_path = Path(PATH_DCCSI_TOOLS_BLENDER,
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
        _LOGGER.info('Installing python package dependancies from requirements.txt')
        foundation.install_requirements(python_exe=settings.DCCSI_BLENDER_PY_EXE)
        _LOGGER.warning('If the Blender app is running we suggest restarting it')

    # custom prompt
    sys.ps1 = f"[{_MODULENAME}]>>"

    _MODULE_END = timeit.default_timer() - _MODULE_START
    _LOGGER.info(f'{_MODULENAME} took: {_MODULE_END} sec')

    if args.exit:
        # return
        sys.exit()
# --- END -----------------------------------------------------------------
