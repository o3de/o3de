"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

"""Extend the .env using dynaconf (dynamic configuration and settings)
This config.py module assumes a minimal enviornment is defined in the .env
To do: ensure that we can stack/layer the dynamic env to work with O3DE projects
"""

# built in's
import os
import sys
import site
import re
import logging as _logging

# 3rdParty (possibly) py3 ships with pathlib, 2.7 does not
# import pathlib
# our framework for dcc tools need to run in apps like Maya that may still be
# on py27 so we need to import and use after some boostrapping
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def attach_debugger():
    _DCCSI_GDEBUG = True
    os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)
    
    _DCCSI_DEV_MODE = True
    os.environ["DYNACONF_DCCSI_DEV_MODE"] = str(_DCCSI_DEV_MODE)
    
    from azpy.test.entry_test import connect_wing
    return connect_wing()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = __name__
if _MODULENAME is '__main__':
    _MODULENAME = 'DCCsi.config'

os.environ['PYTHONINSPECT'] = 'True'
_MODULE_PATH = os.path.abspath(__file__)

# we don't have access yet to the DCCsi Lib\site-packages
# (1) this will give us import access to azpy (always?)
_DCCSI_PATH = os.getenv('DCCSIG_PATH',
                         os.path.abspath(os.path.dirname(_MODULE_PATH)))
# ^ we assume this config is in the root of the DCCsi
# if it's not, be sure to set envar 'DCCSIG_PATH' to ensure it
site.addsitedir(_DCCSI_PATH)  # PYTHONPATH, add code access

# now we have azpy api access
import azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import FRMT_LOG_LONG

# set up global space, logging etc.
# set these true if you want them set globally for debugging
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, int(20)))
if _DCCSI_GDEBUG:
    _DCCSI_LOGLEVEL = int(10)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# early attach WingIDE debugger (can refactor to include other IDEs later)
if _DCCSI_DEV_MODE:
    _debugger = attach_debugger()
# to do: ^ this should be replaced with full featured azpy.dev.util
# that supports additional debuggers (pycharm, vscode, etc.)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# set up module logging
for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)
_LOGGER = _logging.getLogger(_MODULENAME)
_logging.basicConfig(format=FRMT_LOG_LONG, level=_DCCSI_LOGLEVEL)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_LOGGER.info('site.addsitedir({})'.format(_DCCSI_PATH))
_LOGGER.debug('_DCCSI_GDEBUG: {}'.format(_DCCSI_GDEBUG))
_LOGGER.debug('_DCCSI_DEV_MODE: {}'.format(_DCCSI_DEV_MODE))
_LOGGER.debug('_DCCSI_LOGLEVEL: {}'.format(_DCCSI_LOGLEVEL))

# this will give us import access to modules we provide
_DCCSI_PYTHON_LIB_PATH = azpy.config_utils.bootstrap_dccsi_py_libs(_DCCSI_PATH)

# Now we should be able to just carry on with pth lib and dynaconf
from dynaconf import Dynaconf
try:
    import pathlib
except:
    import pathlib2 as pathlib
from pathlib import Path

_DCCSI_PATH = Path(_DCCSI_PATH)
_DCCSI_PYTHON_LIB_PATH = Path(_DCCSI_PYTHON_LIB_PATH)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# this will retreive the O3DE engine root
_O3DE_DEV = azpy.config_utils.get_o3de_engine_root()

# check in running O3DE editor (makes some things procedurally easy)
try:
    import azlmbr
    _LOGGER.info('azlmbr.paths.projectroot: {}'.format(azlmbr.paths.projectroot))
    _O3DE_PROJECT_PATH = Path(azlmbr.paths.projectroot)
except ImportError as e:  # not running O3DE

    _O3DE_PROJECT_PATH = azpy.config_utils.get_check_global_project()

# set up dynamic config envars
os.environ["DYNACONF_O3DE_DEV"] = str(_O3DE_DEV.resolve())


os.environ["DYNACONF_O3DE_PROJECT_PATH"] = str(_O3DE_PROJECT_PATH)

# there are multiple ways to determine the project
# the easiest is if we are running in O3DE
if azlmbr:
    _LOGGER.info('azlmbr.paths.projectroot: {}'.format(azlmbr.paths.projectroot))
    _O3DE_PROJECT = Path(azlmbr.paths.projectroot)
else:
    

_O3DE_PROJECT
os.environ["DYNACONF_O3DE_PROJECT"] = str(_O3DE_PROJECT.resolve())


_O3DE_PROJECT_PATH = Path(_O3DE_DEV, _O3DE_PROJECT)


# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init_o3de_pyside(O3DE_DEV=_O3DE_DEV):
    """sets access to lumberyards Qt dlls and PySide"""

    O3DE_DEV = Path(O3DE_DEV).resolve()
    if not O3DE_DEV.exists():
        raise Exception('O3DE_DEV does NOT exist: {0}'.format(O3DE_DEV))
    else:
        # to do: 'windows_vs2019' might change or be different locally
        # 'windows_vs2019' is defined as a str tag in constants
        # we may not yet have access to azpy.constants :(
        from azpy.constants import TAG_DIR_O3DE_BUILD
        from azpy.constants import PATH_O3DE_BUILD_PATH
        from azpy.constants import PATH_O3DE_BIN_PATH
        # to do: pull some of these str and tags from constants
        O3DE_BUILD_PATH = Path.joinpath(O3DE_DEV,
                                      TAG_DIR_O3DE_BUILD).resolve()
        O3DE_BIN_PATH = Path.joinpath(O3DE_BUILD_PATH,
                                    'bin',
                                    'profile').resolve()

    # # allows to retreive from settings.QTFORPYTHON_PATH
    # from azpy.constants import STR_QTFORPYTHON_PATH  # a path string constructor
    # QTFORPYTHON_PATH = Path(STR_QTFORPYTHON_PATH.format(O3DE_DEV)).resolve()
    # os.environ["DYNACONF_QTFORPYTHON_PATH"] = str(QTFORPYTHON_PATH)
    # site.addsitedir(str(QTFORPYTHON_PATH))  # PYTHONPATH

    QT_PLUGIN_PATH = Path.joinpath(O3DE_BIN_PATH,
                                   'EditorPlugins').resolve()
    os.environ["DYNACONF_QT_PLUGIN_PATH"] = str(QT_PLUGIN_PATH)
    os.environ['PATH'] = QT_PLUGIN_PATH.as_posix() + os.pathsep + os.environ['PATH']

    QT_QPA_PLATFORM_PLUGIN_PATH = Path.joinpath(QT_PLUGIN_PATH,
                                                'platforms').resolve()
    os.environ["DYNACONF_QT_QPA_PLATFORM_PLUGIN_PATH"] = str(QT_QPA_PLATFORM_PLUGIN_PATH)
    # if the line below is removed external standalone apps can't load PySide2
    os.environ["QT_QPA_PLATFORM_PLUGIN_PATH"] = str(QT_QPA_PLATFORM_PLUGIN_PATH)
    # ^^ bypass trying to set only with DYNACONF environment
    os.environ['PATH'] = QT_QPA_PLATFORM_PLUGIN_PATH.as_posix() + os.pathsep + os.environ['PATH']
    # ^^ this particular env only works correctly if put on the PATH in this manner

    # add Qt binaries to the Windows path to handle findings DLL file dependencies
    if sys.platform.startswith('win'):
        # path = os.environ['PATH']
        # newPath = ''
        # newPath += str(O3DE_BIN_PATH) + os.pathsep
        # newPath += str(Path.joinpath(QTFORPYTHON_PATH,
        #                              'shiboken2').resolve()) + os.pathsep
        # newPath += str(Path.joinpath(QTFORPYTHON_PATH,
        #                              'PySide2').resolve()) + os.pathsep
        # newPath += path
        # os.environ['PATH']=newPath
        _LOGGER.debug('PySide2 bootstrapped PATH for Windows.')

    try:
        import PySide2
        _LOGGER.debug('DCCsi, config.py: SUCCESS: import PySide2')
        _LOGGER.debug(PySide2)
        status = True
    except ImportError as e:
        _LOGGER.debug('DCCsi, config.py: FAILURE: import PySide2')
        status = False
        raise(e)

    try:
        import shiboken2
        _LOGGER.debug('DCCsi, config.py: SUCCESS: import shiboken2')
        _LOGGER.debug(shiboken2)
        status = True
    except ImportError as e:
        _LOGGER.debug('DCCsi, config.py: FAILURE: import shiboken2')
        status = False
        raise(e)

    # set up the pyside2-tools (pyside2uic)
    # to do: move path construction string to constants and build off of SDK
    # have not done that yet as I really want to get legal approval and
    # add this to the QtForPython Gem
    # please pass this on the current code review
    DCCSI_PYSIDE2_TOOLS = Path.joinpath(O3DE_DEV,
                                        'Gems',
                                        'AtomLyIntegration',
                                        'TechnicalArt',
                                        'DccScriptingInterface',
                                        '.dev',
                                        'QtForPython',
                                        'pyside2-tools-dev')
    os.environ["DYNACONF_DCCSI_PYSIDE2_TOOLS"] = str(DCCSI_PYSIDE2_TOOLS.resolve())
    os.environ['PATH'] = DCCSI_PYSIDE2_TOOLS.as_posix() + os.pathsep + os.environ['PATH']

    return status
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def test_pyside2():
    """Convenience method to test Qt / PySide2 access"""
    # now test
    _LOGGER.info('Testing Qt / PySide2')
    try:
        from PySide2.QtWidgets import QApplication, QPushButton
        app = QApplication(sys.argv)
        hello = QPushButton("Hello world!")
        hello.resize(200, 60)
        hello.show()
    except Exception as e:
        _LOGGER.error('FAILURE: Qt / PySide2')
        status = False
        raise(e)

    _LOGGER.info('SUCCESS: .test_pyside2()')
    sys.exit(app.exec_())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init_o3de(args):
    # `envvar_prefix` = export envvars with `export DYNACONF_FOO=bar`.
    # `settings_files` = Load this files in the order.
    # here we are modifying or adding to the dynamic config settings on import
    settings = Dynaconf(envvar_prefix="DYNACONF",
                        settings_files=['settings.json',
                                        'dev.settings.json',
                                        'user.settings.json',
                                        '.secrets.json']
                        )
    
    from azpy.constants import PATH_O3DE_BUILD_PATH
    from azpy.constants import PATH_O3DE_BIN_PATH

    # global settings
    os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)
    os.environ["DYNACONF_DCCSI_DEV_MODE"] = str(_DCCSI_DEV_MODE)
    os.environ['DYNACONF_DCCSI_LOGLEVEL'] = str(_LOG_LEVEL)

    # get the O3DE engine root folder
    # first just check if we are running in O3DE
    try:
        import azlmbr
        _LOGGER.info('azlmbr.paths.devroot: {}'.format(azlmbr.paths.devroot))
        _O3DE_DEV = Path(azlmbr.paths.devroot)
    except ImportError as e: # not running O3DE
        _O3DE_DEV = azpy.config_utils.get_o3de_engine_root()
    os.environ["DYNACONF_O3DE_DEV"] = str(_O3DE_DEV.resolve())
    
    # there are multiple ways to determine the project
    # the easiest is if we are running in O3DE
    if azlmbr:
        _LOGGER.info('azlmbr.paths.projectroot: {}'.format(azlmbr.paths.projectroot))
        _O3DE_PROJECT = Path(azlmbr.paths.projectroot)
    else:
        

    _O3DE_PROJECT = azpy.config_utils.get_check_global_project()
    os.environ["DYNACONF_O3DE_PROJECT"] = str(_O3DE_PROJECT.resolve())
    
    
    _O3DE_PROJECT_PATH = Path(_O3DE_DEV, _O3DE_PROJECT)
    os.environ["DYNACONF_O3DE_PROJECT_PATH"] = str(_O3DE_PROJECT_PATH)
    
    os.environ["DYNACONF_DCCSIG_PATH"] = str(_DCCSI_PATH.resolve())
    _DCCSI_CONFIG_PATH = Path(_MODULE_PATH).resolve()
    os.environ["DYNACONF_DCCSI_CONFIG_PATH"] = str(_DCCSI_CONFIG_PATH)
    _DCCSIG_TOOLS_PATH = Path.joinpath(_DCCSI_PATH, 'Tools').resolve()
    os.environ["DYNACONF_DCCSIG_SDK_PATH"] = str(_DCCSIG_TOOLS_PATH)
    os.environ["DYNACONF_DCCSI_PYTHON_LIB_PATH"] = str(_DCCSI_PYTHON_LIB_PATH.resolve())
    os.environ["DYNACONF_OS_FOLDER"] = azpy.config_utils.get_os()
    
    # we need to set up the Ly dev build \bin\path (for Qt dll access)
    _O3DE_BUILD_PATH = Path(PATH_O3DE_BUILD_PATH).resolve()
    os.environ["DYNACONF_O3DE_BUILD_PATH"] = str(_O3DE_BUILD_PATH)
    _O3DE_BIN_PATH = Path(PATH_O3DE_BIN_PATH).resolve()
    os.environ["DYNACONF_O3DE_BIN_PATH"] = str(_O3DE_BIN_PATH)
    
    # project cache log dir path
    from azpy.constants import ENVAR_DCCSI_LOG_PATH
    from azpy.constants import PATH_DCCSI_LOG_PATH
    _DCCSI_LOG_PATH = Path(os.getenv(ENVAR_DCCSI_LOG_PATH,
                                     Path(PATH_DCCSI_LOG_PATH.format(O3DE_DEV=_O3DE_DEV,
                                                                     O3DE_PROJECT=_O3DE_PROJECT))))
    os.environ["DYNACONF_DCCSI_LOG_PATH"] = str(_DCCSI_LOG_PATH)
    
    # hard checks
    if not _O3DE_BIN_PATH.exists():
        raise Exception('O3DE_BIN_PATH does NOT exist: {0}'.format(_O3DE_BIN_PATH))
    else:
        # adding to sys.path apparently doesn't work for .dll locations like Qt
        os.environ['PATH'] = _O3DE_BIN_PATH.as_posix() + os.pathsep + os.environ['PATH']
    
    _LOGGER.info('Dynaconf config.py ... DONE')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# settings.setenv()  # doing this will add the additional DYNACONF_ envars
def get_config_settings(setup_ly_pyside=False):
    """Convenience method to set and retreive settings directly from module."""
    from dynaconf import settings

    if setup_ly_pyside:
        init_o3de_pyside(settings.O3DE_DEV)

    settings.setenv()
    return settings
# --- END -----------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone script"""
    
    # overide logger for standalone to be more verbose and lof to file
    _LOGGER = azpy.initialize_logger(_MODULENAME,
                                     log_to_file=_DCCSI_GDEBUG,
                                     default_log_level=_DCCSI_LOGLEVEL)

    # go ahead and run the rest of the configuration
    # parse the command line args
    import argparse
    parser = argparse.ArgumentParser(
        description='O3DE DCCsi Dynamic Config',
        epilog="Attempts to determine O3DE project if -pp not set")
    parser.add_argument('-pp', '--project-path',
                        type=pathlib.Path,
                        required=False,
                        help='The path to the project.')
    parser.add_argument('-pn', '--project-name',
                        type=str,
                        required=False,
                        help='The name of the project.')
    parser.add_argument('-gd', '--global-debug',
                        type=str,
                        required=False,
                        help='Enables global debug flag.')
    parser.add_argument('-dm', '--developer-mode',
                        type=str,
                        required=False,
                        help='Enables dev mode for auto attaching debugger.')
    parser.add_argument('-es', '--export-settings',
                        type=str,
                        required=False,
                        help='Enables dev mode for auto attaching debugger.')
    args = parser.parse_args()
    
    init_o3de(args)  # initialize the core config and settings

    # now standalone we can validate the config. env, settings.
    from dynaconf import settings

    # not using fstrings in this module because it might run in py2.7 (maya)
    _LOGGER.info('DCCSI_GDEBUG: {}'.format(settings.DCCSI_GDEBUG))
    _LOGGER.info('DCCSI_DEV_MODE: {}'.format(settings.DCCSI_DEV_MODE))
    _LOGGER.info('DCCSI_LOGLEVEL: {}'.format(settings.DCCSI_LOGLEVEL))

    _LOGGER.info('OS_FOLDER: {}'.format(settings.OS_FOLDER))
    _LOGGER.info('O3DE_PROJECT: {}'.format(settings.O3DE_PROJECT))
    _LOGGER.info('O3DE_PROJECT_PATH: {}'.format(settings.O3DE_PROJECT_PATH))
    _LOGGER.info('O3DE_DEV: {}'.format(settings.O3DE_DEV))
    _LOGGER.info('O3DE_BUILD_PATH: {}'.format(settings.O3DE_BUILD_PATH))
    _LOGGER.info('O3DE_BIN_PATH: {}'.format(settings.O3DE_BIN_PATH))

    _LOGGER.info('DCCSI_LOG_PATH: {}'.format(settings.DCCSI_LOG_PATH))

    _LOGGER.info('DCCSI_CONFIG_PATH: {}'.format(settings.DCCSI_CONFIG_PATH))
    _LOGGER.info('DCCSIG_PATH: {}'.format(settings.DCCSIG_PATH))
    _LOGGER.info('DCCSI_PYTHON_LIB_PATH: {}'.format(settings.DCCSI_PYTHON_LIB_PATH))
    _LOGGER.info('DCCSI_PY_BASE: {}'.format(settings.DCCSI_PY_BASE))

    # To Do: These should ONLY be set for Lumberyard and non-DCC environments
    # They will most likely cause Qt/PySide DCC apps to fail
    # or hopefully they can be overridden for DCC envionments
    # that provide their own Qt dlls and Pyside2
    # _LOGGER.info('QTFORPYTHON_PATH: {}'.format(settings.QTFORPYTHON_PATH))
    # _LOGGER.info('QT_PLUGIN_PATH: {}'.format(settings.QT_PLUGIN_PATH))

    init_o3de_pyside(settings.O3DE_DEV)  # init lumberyard Qt/PySide2
    # from dynaconf import settings # <-- no need to reimport

    settings.setenv()  # doing this will add/set the additional DYNACONF_ envars
    
    # writting settings
    from dynaconf import loaders
    from dynaconf.utils.boxing import DynaBox

    _settings_dict = settings.as_dict()
    _settings_file = Path('tmp_settings.json')
    # writes to a file, the format is inferred by extension
    # can be .yaml, .toml, .ini, .json, .py
    #loaders.write(_settings_file, DynaBox(data).to_dict(), merge=False, env='development')
    #loaders.write(_settings_file, DynaBox(data).to_dict())

    from box import Box
    _settings_box = Box(_settings_dict)
    
    if _DCCSI_GDEBUG:
        _LOGGER.info('Pretty print, _settings_box: {}'.format(_settings_file))
        _LOGGER.info(str(_settings_box.to_json(sort_keys=True,
                                               indent=4)))
        
    _settings_box.to_json(filename=_settings_file.as_posix(),
                           sort_keys=True,
                           indent=4)
    
    # _LOGGER.info('QTFORPYTHON_PATH: {}'.format(settings.QTFORPYTHON_PATH))
    _LOGGER.info('O3DE_BIN_PATH: {}'.format(settings.O3DE_BIN_PATH))
    _LOGGER.info('QT_PLUGIN_PATH: {}'.format(settings.QT_PLUGIN_PATH))
    _LOGGER.info('QT_QPA_PLATFORM_PLUGIN_PATH: {}'.format(settings.QT_QPA_PLATFORM_PLUGIN_PATH))
    _LOGGER.info('DCCSI_PYSIDE2_TOOLS: {}'.format(settings.DCCSI_PYSIDE2_TOOLS))

    test_pyside2()  # test PySide2 access with a pop-up button







