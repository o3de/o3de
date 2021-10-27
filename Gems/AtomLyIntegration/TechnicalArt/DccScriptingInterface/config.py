"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

"""<DCCsi>.config
Generate dynamic and synethetic environment contest and settings
using dynaconf (dynamic configuration and settings)
This config.py synthetic env can be overriden or extended with a local .env
See: example.env.tmp (copy and rename to .env)
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
_O3DE_RUNNING=None    
try:
    import azlmbr
    _O3DE_RUNNING=True
except:
    _O3DE_RUNNING=False
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def attach_debugger():
    _DCCSI_GDEBUG = True
    os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)
    
    _DCCSI_DEV_MODE = True
    os.environ["DYNACONF_DCCSI_DEV_MODE"] = str(_DCCSI_DEV_MODE)
    
    from azpy.test.entry_test import connect_wing
    _debugger = connect_wing()
    
    return _debugger
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = __name__
if _MODULENAME is '__main__':
    _MODULENAME = 'DCCsi.config'

#os.environ['PYTHONINSPECT'] = 'True'
_MODULE_PATH = os.path.abspath(__file__)

# we don't have access yet to the DCCsi Lib\site-packages
# (1) this will give us import access to azpy (always?)
_DCCSI_PATH = os.getenv('DCCSI_PATH',
                         os.path.abspath(os.path.dirname(_MODULE_PATH)))
# ^ we assume this config is in the root of the DCCsi
# if it's not, be sure to set envar 'DCCSI_PATH' to ensure it
site.addsitedir(_DCCSI_PATH)  # must be done for azpy

# now we have azpy api access
import azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL

# set up global space, logging etc.
# set these true if you want them set globally for debugging
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, int(20)))
if _DCCSI_GDEBUG:
    _DCCSI_LOGLEVEL = int(10)

# early attach WingIDE debugger (can refactor to include other IDEs later)
# requires externally enabling via ENVAR
if _DCCSI_DEV_MODE:
    _debugger = attach_debugger()
# to do: ^ this should be replaced with full featured azpy.dev.util
# that supports additional debuggers (pycharm, vscode, etc.)

# set up module logging
for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)
    
_LOGGER = azpy.initialize_logger(_MODULENAME,
                                 log_to_file=_DCCSI_GDEBUG,
                                 default_log_level=_DCCSI_LOGLEVEL)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
_LOGGER.info('site.addsitedir({})'.format(_DCCSI_PATH))
_LOGGER.debug('_DCCSI_GDEBUG: {}'.format(_DCCSI_GDEBUG))
_LOGGER.debug('_DCCSI_DEV_MODE: {}'.format(_DCCSI_DEV_MODE))
_LOGGER.debug('_DCCSI_LOGLEVEL: {}'.format(_DCCSI_LOGLEVEL))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# this will give us import access to additional modules we provide with DCCsi
_DCCSI_PYTHON_LIB_PATH = azpy.config_utils.bootstrap_dccsi_py_libs(_DCCSI_PATH)

# Now we should be able to just carry on with pth lib and dynaconf
from dynaconf import Dynaconf
try:
    import pathlib
except:
    import pathlib2 as pathlib
from pathlib import Path

_DCCSI_PATH = Path(_DCCSI_PATH)  # pathify
_DCCSI_PYTHON_PATH = Path(_DCCSI_PATH,'3rdParty','Python')
_DCCSI_PYTHON_LIB_PATH = Path(_DCCSI_PYTHON_LIB_PATH)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# start locally prepping known default values for dyanmic environment settings
_O3DE_DCCSI_PATH = os.environ['PATH']
os.environ["DYNACONF_PATH"] = _O3DE_DCCSI_PATH

# this will retreive the O3DE engine root
_O3DE_DEV = azpy.config_utils.get_o3de_engine_root()
# set up dynamic config envars
os.environ["DYNACONF_O3DE_DEV"] = str(_O3DE_DEV.resolve())

from azpy.constants import TAG_DIR_O3DE_BUILD_FOLDER
_O3DE_BUILD_FOLDER = TAG_DIR_O3DE_BUILD_FOLDER
os.environ["DYNACONF_O3DE_BUILD_FOLDER"] = str(_O3DE_BUILD_FOLDER)
_O3DE_BUILD_PATH = Path(_O3DE_DEV, TAG_DIR_O3DE_BUILD_FOLDER)
os.environ["DYNACONF_O3DE_BUILD_PATH"] = str(_O3DE_BUILD_PATH.resolve())

from azpy.constants import STR_O3DE_BIN_PATH
_O3DE_BIN_PATH = Path(STR_O3DE_BIN_PATH.format(_O3DE_BUILD_PATH))
os.environ["DYNACONF_O3DE_BIN_PATH"] = str(_O3DE_BIN_PATH.resolve())

# this in most cases will return the project folder
# if it returns a matching engine folder then we don't know the project folder
_O3DE_PROJECT_PATH = azpy.config_utils.get_o3de_project_path()
os.environ["DYNACONF_O3DE_PROJECT_PATH"] = str(_O3DE_PROJECT_PATH.resolve())

# special, a home for stashing PYTHONPATHs into managed settings
_O3DE_PYTHONPATH = list()
_O3DE_PYTHONPATH.append(_DCCSI_PATH)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init_o3de_pyside2(dccsi_path=_DCCSI_PATH,
                      engine_bin=_O3DE_BIN_PATH):
    """Initialize the DCCsi Qt/PySide dynamic env and settings
    sets access to lumberyards Qt dlls and PySide"""
    
    _DCCSI_PATH = Path(dccsi_path)
    _O3DE_BIN_PATH = Path(engine_bin)

    if not _O3DE_BIN_PATH.exists():
        raise Exception('_O3DE_BIN_PATH does NOT exist: {0}'.format(_O3DE_BIN_PATH))
    else:
        pass
    
    # python config
    _DCCSI_PYTHON_PATH = Path(_DCCSI_PATH,'3rdParty','Python')
    os.environ["DYNACONF_DCCSI_PYTHON_PATH"] = str(_DCCSI_PYTHON_PATH.resolve())

    # # allows to retreive from settings.QTFORPYTHON_PATH
    # from azpy.constants import STR_QTFORPYTHON_PATH  # a path string constructor
    # QTFORPYTHON_PATH = Path(STR_QTFORPYTHON_PATH.format(O3DE_DEV)).resolve()
    # os.environ["DYNACONF_QTFORPYTHON_PATH"] = str(QTFORPYTHON_PATH)
    # site.addsitedir(str(QTFORPYTHON_PATH))  # PYTHONPATH

    QT_PLUGIN_PATH = Path.joinpath(_O3DE_BIN_PATH,'EditorPlugins')
    os.environ["DYNACONF_QT_PLUGIN_PATH"] = str(QT_PLUGIN_PATH.resolve())
    os.environ['PATH'] = QT_PLUGIN_PATH.as_posix() + os.pathsep + os.environ['PATH']

    QT_QPA_PLATFORM_PLUGIN_PATH = Path.joinpath(QT_PLUGIN_PATH, 'platforms')
    os.environ["DYNACONF_QT_QPA_PLATFORM_PLUGIN_PATH"] = str(QT_QPA_PLATFORM_PLUGIN_PATH.resolve())
    # if the line below is removed external standalone apps can't load PySide2
    os.environ["QT_QPA_PLATFORM_PLUGIN_PATH"] = str(QT_QPA_PLATFORM_PLUGIN_PATH.resolve())
    # ^^ bypass trying to set only with DYNACONF environment
    os.environ['PATH'] = QT_QPA_PLATFORM_PLUGIN_PATH.as_posix() + os.pathsep + os.environ['PATH']
    # ^^ this particular env only works correctly if put on the PATH in this manner

    # add Qt binaries to the Windows path to handle findings DLL file dependencies
    if sys.platform.startswith('win'):
        _LOGGER.info('~   Qt/PySide2 bootstrapped PATH for Windows.')
    else:
        _LOGGER.warning('~   Not tested on Non-Windows platforms.')
        # To Do: figure out how to test and/or modify to work

    try:
        import PySide2
        _LOGGER.info('~   SUCCESS: import PySide2')
        _LOGGER.debug(PySide2)
        status = True
    except ImportError as e:
        _LOGGER.error('~   FAILURE: import PySide2')
        status = False
        raise(e)

    try:
        import shiboken2
        _LOGGER.info('~   SUCCESS: import shiboken2')
        _LOGGER.debug(shiboken2)
        status = True
    except ImportError as e:
        _LOGGER.error('~   FAILURE: import shiboken2')
        status = False
        raise(e)

    # set up the pyside2-tools (pyside2uic)
    # to do: move path construction string to constants and build off of SDK
    # have not done that yet as I really want to get legal approval and
    # add this to the QtForPython Gem
    # please pass this in current code reviews
    _DCCSI_PYSIDE2_TOOLS = Path(_DCCSI_PYTHON_PATH,'pyside2-tools')
    if _DCCSI_PYSIDE2_TOOLS.exists():
        os.environ["DYNACONF_DCCSI_PYSIDE2_TOOLS"] = str(_DCCSI_PYSIDE2_TOOLS.resolve())
        os.environ['PATH'] = _DCCSI_PYSIDE2_TOOLS.as_posix() + os.pathsep + os.environ['PATH']
    
        site.addsitedir(_DCCSI_PYSIDE2_TOOLS)
        _O3DE_PYTHONPATH.append(_DCCSI_PYSIDE2_TOOLS.resolve())
        _LOGGER.info('~   PySide2-Tools bootstrapped PATH for Windows.')
        try:
            import pyside2uic
            _LOGGER.info('~   SUCCESS: import pyside2uic')
            _LOGGER.debug(shiboken2)
            status = True
        except ImportError as e:
            _LOGGER.error('~   FAILURE: import pyside2uic')
            status = False
            raise(e)
    else:
        _LOGGER.warning('~   No PySide2 Tools: {}'.format(_DCCSI_PYSIDE2_TOOLS.resolve))
    
    _O3DE_DCCSI_PATH = os.environ['PATH']
    os.environ["DYNACONF_PATH"] = _O3DE_DCCSI_PATH
    
    try: 
        _DCCSI_PYTHONPATH = os.environ['PYTHONPATH']
        os.environ["DYNACONF_PYTHONPATH"] = _DCCSI_PYTHONPATH
    except:
        pass

    from dynaconf import settings    
    
    _LOGGER.info('~   config.init_o3de_pyside() ... DONE')

    if status:
        return settings
    else:
        return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def test_pyside2():
    """Convenience method to test Qt / PySide2 access"""
    # now test
    _LOGGER.info('~   Testing Qt / PySide2')
    try:
        from PySide2.QtWidgets import QApplication, QPushButton
        app = QApplication(sys.argv)
        hello = QPushButton("~   O3DE DCCsi PySide2 Test!")
        hello.resize(200, 60)
        hello.show()
    except Exception as e:
        _LOGGER.error('~   FAILURE: Qt / PySide2')
        status = False
        raise(e)

    _LOGGER.info('~   SUCCESS: .test_pyside2()')
    sys.exit(app.exec_())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init_o3de_core(engine_path=_O3DE_DEV,
                   build_folder=_O3DE_BUILD_FOLDER,
                   project_name=None,
                   project_path=_O3DE_PROJECT_PATH):
    """Initialize the DCCsi Core dynamic env and settings"""
    # `envvar_prefix` = export envvars with `export DYNACONF_FOO=bar`.
    # `settings_files` = Load this files in the order.
    # here we are modifying or adding to the dynamic config settings on import
    settings = Dynaconf(envvar_prefix='DYNACONF',
                        settings_files=['settings.json',
                                        'dev.settings.json',
                                        'user.settings.json',
                                        '.secrets.json'])
    
    # global settings
    os.environ["DYNACONF_DCCSI_OS_FOLDER"] = azpy.config_utils.get_os()
    os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)
    os.environ["DYNACONF_DCCSI_DEV_MODE"] = str(_DCCSI_DEV_MODE)
    os.environ['DYNACONF_DCCSI_LOGLEVEL'] = str(_DCCSI_LOGLEVEL)

    os.environ["DYNACONF_DCCSI_PATH"] = str(_DCCSI_PATH.resolve())
    os.environ['PATH'] = _DCCSI_PATH.as_posix() + os.pathsep + os.environ['PATH']

    # we already defaulted to discovering these two early because of importance
    #os.environ["DYNACONF_O3DE_DEV"] = str(_O3DE_DEV.resolve())
    #os.environ["DYNACONF_O3DE_PROJECT_PATH"] = str(_O3DE_PROJECT_PATH)
    # we also already added them to DYNACONF_
    # this in an explicit pass in
    if project_path:
        _project_path = Path(project_path)
        try:
            _project_path.exists()
            _O3DE_PROJECT_PATH = _project_path
            os.environ["DYNACONF_O3DE_PROJECT_PATH"] = str(_O3DE_PROJECT_PATH.resolve())
        except FileExistsError as e:
            _LOGGER.error('~   The project path specified does not appear to exist!')
            _LOGGER.warning('~   project_path: {}'.format(project_path))
            _LOGGER.warning('~   fallback to engine root: {}'.format())
            project_path = _O3DE_DEV
            os.environ["DYNACONF_O3DE_PROJECT_PATH"] = str(_O3DE_DEV.resolve())

    # we can pull the O3DE_PROJECT (name) from the project path
    if not project_name:
        project_name = Path(_O3DE_PROJECT_PATH).name
    os.environ["DYNACONF_O3DE_PROJECT"] = str(project_name)
    # To Do: there might be a project namespace in the project.json?

    # -- O3DE build -- set up \bin\path (for Qt dll access)
    os.environ["DYNACONF_O3DE_BUILD_FOLDER"] = str(build_folder)
    _O3DE_BUILD_PATH = Path(_O3DE_DEV, build_folder)
    
    os.environ["DYNACONF_O3DE_BUILD_PATH"] = str(_O3DE_BUILD_PATH.resolve())
    
    _O3DE_BIN_PATH = Path(STR_O3DE_BIN_PATH.format(_O3DE_BUILD_PATH))
    os.environ["DYNACONF_O3DE_BIN_PATH"] = str(_O3DE_BIN_PATH.resolve())
    
    # hard check
    if not _O3DE_BIN_PATH.exists():
        raise Exception('O3DE_BIN_PATH does NOT exist: {0}'.format(_O3DE_BIN_PATH))
    else:
        # adding to sys.path apparently doesn't work for .dll locations like Qt
        os.environ['PATH'] = _O3DE_BIN_PATH.as_posix() + os.pathsep + os.environ['PATH']    
    # --
    
    from azpy.constants import TAG_DIR_DCCSI_TOOLS
    _DCCSI_TOOLS_PATH = Path(_DCCSI_PATH, TAG_DIR_DCCSI_TOOLS)
    os.environ["DYNACONF_DCCSI_TOOLS_PATH"] = str(_DCCSI_TOOLS_PATH.resolve())
    
    from azpy.constants import TAG_DCCSI_NICKNAME
    from azpy.constants import PATH_DCCSI_LOG_PATH
    _DCCSI_LOG_PATH = Path(PATH_DCCSI_LOG_PATH.format(O3DE_PROJECT_PATH=project_path,
                                                      TAG_DCCSI_NICKNAME=TAG_DCCSI_NICKNAME))
    os.environ["DYNACONF_DCCSI_LOG_PATH"] = str(_DCCSI_LOG_PATH)    
    
    from azpy.constants import TAG_DIR_REGISTRY, TAG_DCCSI_CONFIG
    _DCCSI_CONFIG_PATH = Path(project_path, TAG_DIR_REGISTRY, TAG_DCCSI_CONFIG)
    os.environ["DYNACONF_DCCSI_CONFIG_PATH"] = str(_DCCSI_CONFIG_PATH.resolve())
    
    from azpy.constants import TAG_DIR_DCCSI_TOOLS
    _DCCSIG_TOOLS_PATH = Path.joinpath(_DCCSI_PATH, TAG_DIR_DCCSI_TOOLS)
    os.environ["DYNACONF_DCCSIG_TOOLS_PATH"] = str(_DCCSIG_TOOLS_PATH.resolve())
    
    _O3DE_DCCSI_PATH = os.environ['PATH']
    os.environ["DYNACONF_PATH"] = _O3DE_DCCSI_PATH
    
    from dynaconf import settings    
    
    _LOGGER.info('~   config.init_o3de_core() ... DONE')
    
    return settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init_o3de_python(engine_path=_O3DE_DEV,
                     engine_bin=_O3DE_BIN_PATH,
                     dccsi_path=_DCCSI_PATH):
    
    # pathify
    _O3DE_DEV = Path(engine_path)
    _O3DE_BIN_PATH = Path(engine_bin)
    _DCCSI_PATH = Path(dccsi_path)
    
    # python config
    _DCCSI_PYTHON_PATH = Path(_DCCSI_PATH,'3rdParty','Python')
    os.environ["DYNACONF_DCCSI_PYTHON_PATH"] = str(_DCCSI_PYTHON_PATH.resolve())
    
    _DCCSI_PYTHON_LIB_PATH = azpy.config_utils.bootstrap_dccsi_py_libs(_DCCSI_PATH)
    os.environ["DYNACONF_DCCSI_PYTHON_LIB_PATH"] = str(_DCCSI_PYTHON_LIB_PATH.resolve())
    os.environ['PATH'] = _DCCSI_PYTHON_LIB_PATH.as_posix() + os.pathsep + os.environ['PATH']
    site.addsitedir(_DCCSI_PYTHON_LIB_PATH)
    _O3DE_PYTHONPATH.append(_DCCSI_PYTHON_LIB_PATH.resolve())    
    
    site.addsitedir(_O3DE_BIN_PATH)
    _O3DE_PYTHONPATH.append(_O3DE_BIN_PATH.resolve())   
        
    _O3DE_PY_EXE = Path(sys.executable)
    _DCCSI_PY_IDE = Path(_O3DE_PY_EXE)
    os.environ["DYNACONF_DCCSI_PY_IDE"] = str(_DCCSI_PY_IDE.resolve())
    
    _O3DE_PYTHONHOME = Path(_O3DE_PY_EXE.parents[0])
    os.environ["DYNACONF_O3DE_PYTHONHOME"] = str(_O3DE_PYTHONHOME.resolve())
    os.environ['PATH'] = _O3DE_PYTHONHOME.as_posix() + os.pathsep + os.environ['PATH']
    _LOGGER.info('~   O3DE_PYTHONHOME - is now the folder containing O3DE python executable')
    
    _O3DE_PYTHON_INSTALL = Path(_O3DE_DEV, 'python')
    os.environ["DYNACONF_O3DE_PYTHON_INSTALL"] = str(_O3DE_PYTHON_INSTALL.resolve())
    os.environ['PATH'] = _O3DE_PYTHON_INSTALL.as_posix() + os.pathsep + os.environ['PATH']    

    if sys.platform.startswith('win'):
        _DCCSI_PY_BASE = Path(_O3DE_PYTHON_INSTALL, 'python.cmd')
    elif sys.platform == "linux":
        _DCCSI_PY_BASE = Path(_O3DE_PYTHON_INSTALL, 'python.sh')
    elif sys.platform == "darwin":
        _DCCSI_PY_BASE = Path(_O3DE_PYTHON_INSTALL, 'python.sh')
    else:
        _DCCSI_PY_BASE = None
        
    if _DCCSI_PY_BASE:
        os.environ["DYNACONF_DCCSI_PY_BASE"] = str(_DCCSI_PY_BASE.resolve())        
    
    _O3DE_DCCSI_PATH = os.environ['PATH']
    os.environ["DYNACONF_PATH"] = _O3DE_DCCSI_PATH
    
    try: 
        _DCCSI_PYTHONPATH = os.environ['PYTHONPATH']
        os.environ["DYNACONF_PYTHONPATH"] = _DCCSI_PYTHONPATH
    except:
        pass

    from dynaconf import settings    
    
    _LOGGER.info('~   config.init_o3de_python() ... DONE')
    
    return settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# settings.setenv()  # doing this will add the additional DYNACONF_ envars
def get_config_settings(engine_path=_O3DE_DEV,
                        build_folder=_O3DE_BUILD_FOLDER,
                        project_name=None,
                        project_path=_O3DE_PROJECT_PATH,
                        enable_o3de_python=None,
                        enable_o3de_pyside2=None,
                        set_env=True):
    """Convenience method to initialize and retreive settings directly from module."""
    
    settings = init_o3de_core(engine_path,
                              build_folder,
                              project_name,
                              project_path)

    if enable_o3de_python:
        settings = init_o3de_python(settings.O3DE_DEV,
                                    settings.O3DE_BIN_PATH,
                                    settings.DCCSI_PATH)
    
    # These should ONLY be set for O3DE and non-DCC environments
    # They will most likely cause other Qt/PySide DCC apps to fail
    # or hopefully they can be overridden for DCC envionments
    # that provide their own Qt dlls and Pyside2
    # _LOGGER.info('QTFORPYTHON_PATH: {}'.format(settings.QTFORPYTHON_PATH))
    # _LOGGER.info('QT_PLUGIN_PATH: {}'.format(settings.QT_PLUGIN_PATH))
    # assume our standalone python tools wants this access?
    # it's safe to do this for dev and from ide
    if enable_o3de_pyside2:
        settings = init_o3de_pyside2(settings.DCCSI_PATH,
                                     settings.O3DE_BIN_PATH)

    # now standalone we can validate the config. env, settings.
    from dynaconf import settings
    if set_env:
        settings.setenv()
    return settings
# --- END -----------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone cli script"""
    
    _MODULENAME = __name__
    if _MODULENAME is '__main__':
        _MODULENAME = 'DCCsi.config'
    
    from azpy.constants import STR_CROSSBAR
    
    while 0: # temp internal debug flag
        _DCCSI_GDEBUG = True
        break
    
    # overide logger for standalone to be more verbose and log to file
    _LOGGER = azpy.initialize_logger(_MODULENAME,
                                     log_to_file=_DCCSI_GDEBUG,
                                     default_log_level=_DCCSI_LOGLEVEL)

    # happy print
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('~ constants.py ... Running script as __main__')
    _LOGGER.info(STR_CROSSBAR)

    # go ahead and run the rest of the configuration
    # parse the command line args
    import argparse
    parser = argparse.ArgumentParser(
        description='O3DE DCCsi Dynamic Config (dynaconf)',
        epilog="Attempts to determine O3DE project if -pp not set")
    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        help='Enables global debug flag.')
    parser.add_argument('-dm', '--developer-mode',
                        type=bool,
                        required=False,
                        help='Enables dev mode for early auto attaching debugger.')
    parser.add_argument('-ep', '--engine-path',
                        type=pathlib.Path,
                        required=False,
                        help='The path to the o3de engine.')
    parser.add_argument('-bf', '--build-folder',
                        type=str,
                        required=False,
                        help='The name (tag) of the o3de build folder, example build or windows_vs2019.')
    parser.add_argument('-pp', '--project-path',
                        type=pathlib.Path,
                        required=False,
                        help='The path to the project.')
    parser.add_argument('-pn', '--project-name',
                        type=str,
                        required=False,
                        help='The name of the project.')
    parser.add_argument('-py', '--enable-python',
                        type=bool,
                        required=False,
                        help='Enables O3DE python access.')
    parser.add_argument('-qt', '--enable-qt',
                        type=bool,
                        required=False,
                        help='Enables O3DE Qt\PySide2 access.')
    parser.add_argument('-sd', '--set-debugger',
                        type=str,
                        required=False,
                        help='Default debugger: WING, others: PYCHARM, VSCODE (not yet implemented).')
    parser.add_argument('-pc', '--project-config',
                        type=bool,
                        required=False,
                        help='Enables reading the projects registry\dccsiconfiguration.setreg.')
    parser.add_argument('-es', '--export-settings',
                        type=pathlib.Path,
                        required=False,
                        help='Writes managed settings to specified path.')
    parser.add_argument('-ec', '--export-configuration',
                        type=bool,
                        required=False,
                        help='writes settings as a O3DE registry\dccsiconfiguration.setreg.')
    parser.add_argument('-tp', '--test-pyside2',
                        type=bool,
                        required=False,
                        help='Runs Qt/PySide2 tests and reports.')
    args = parser.parse_args()

    # easy overrides
    if args.global_debug:
        _DCCSI_GDEBUG = True
        os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)
    if args.developer_mode:
        attach_debugger()  # attempts to start debugger

    if args.set_debugger:
        _LOGGER.info('Setting and switching debugger type from WingIDE not implemented.')
        # To Do: implement debugger plugin pattern
        
    # need to do a little plumbing
    if not args.engine_path:
        args.engine_path=_O3DE_DEV
    if not args.build_folder:
        from azpy.constants import TAG_DIR_O3DE_BUILD_FOLDER
        args.build_folder = TAG_DIR_O3DE_BUILD_FOLDER
    if not args.project_path:
        args.project_path=_O3DE_PROJECT_PATH
        
    if _DCCSI_GDEBUG:
        args.enable_python = True
        args.enable_qt = True

    # now standalone we can validate the config. env, settings.
    settings = get_config_settings(engine_path=args.engine_path,
                                   build_folder=args.build_folder,
                                   project_name=args.project_name,
                                   project_path=args.project_path,
                                   enable_o3de_python=args.enable_python,
                                   enable_o3de_pyside2=args.enable_qt)

    ## CORE
    _LOGGER.info(STR_CROSSBAR)
    # not using fstrings in this module because it might run in py2.7 (maya)
    _LOGGER.info('DCCSI_GDEBUG: {}'.format(settings.DCCSI_GDEBUG))
    _LOGGER.info('DCCSI_DEV_MODE: {}'.format(settings.DCCSI_DEV_MODE))
    _LOGGER.info('DCCSI_LOGLEVEL: {}'.format(settings.DCCSI_LOGLEVEL))
    _LOGGER.info('DCCSI_OS_FOLDER: {}'.format(settings.DCCSI_OS_FOLDER))
    
    _LOGGER.info('O3DE_DEV: {}'.format(settings.O3DE_DEV))
    _LOGGER.info('O3DE_O3DE_BUILD_FOLDER: {}'.format(settings.O3DE_BUILD_PATH))
    _LOGGER.info('O3DE_BUILD_PATH: {}'.format(settings.O3DE_BUILD_PATH))
    _LOGGER.info('O3DE_BIN_PATH: {}'.format(settings.O3DE_BIN_PATH))
    
    _LOGGER.info('O3DE_PROJECT: {}'.format(settings.O3DE_PROJECT))
    _LOGGER.info('O3DE_PROJECT_PATH: {}'.format(settings.O3DE_PROJECT_PATH))
    
    _LOGGER.info('DCCSI_PATH: {}'.format(settings.DCCSI_PATH))
    _LOGGER.info('DCCSI_LOG_PATH: {}'.format(settings.DCCSI_LOG_PATH))
    _LOGGER.info('DCCSI_CONFIG_PATH: {}'.format(settings.DCCSI_CONFIG_PATH))
    
    if settings.O3DE_DCCSI_ENV_TEST:
        _LOGGER.info('O3DE_DCCSI_ENV_TEST: {}'.format(settings.O3DE_DCCSI_ENV_TEST))
    
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('')
    
    if args.enable_python:
        _LOGGER.info(STR_CROSSBAR)
        _LOGGER.info('DCCSI_PYTHON_PATH'.format(settings.DCCSI_PYTHON_PATH))
        _LOGGER.info('DCCSI_PYTHON_LIB_PATH: {}'.format(settings.DCCSI_PYTHON_LIB_PATH))
        _LOGGER.info('DCCSI_PY_IDE'.format(settings.DCCSI_PY_IDE))
        _LOGGER.info('O3DE_PYTHONHOME'.format(settings.O3DE_PYTHONHOME))
        _LOGGER.info('O3DE_PYTHON_INSTALL'.format(settings.O3DE_PYTHON_INSTALL))
        _LOGGER.info('DCCSI_PY_BASE: {}'.format(settings.DCCSI_PY_BASE))
        _LOGGER.info(STR_CROSSBAR)
        _LOGGER.info('')
    else:
        _LOGGER.info('Tip: add arg --enable-python to extend the environment with O3DE python access')
        
    if args.enable_qt:
        _LOGGER.info(STR_CROSSBAR)
        # _LOGGER.info('QTFORPYTHON_PATH: {}'.format(settings.QTFORPYTHON_PATH))
        _LOGGER.info('QT_PLUGIN_PATH: {}'.format(settings.QT_PLUGIN_PATH))
        _LOGGER.info('QT_QPA_PLATFORM_PLUGIN_PATH: {}'.format(settings.QT_QPA_PLATFORM_PLUGIN_PATH))
        _LOGGER.info('DCCSI_PYSIDE2_TOOLS: {}'.format(settings.DCCSI_PYSIDE2_TOOLS))  
        _LOGGER.info(STR_CROSSBAR)
        _LOGGER.info('')
    else:

        _LOGGER.info('Tip: add arg --enable-qt to extend the environment with O3DE Qt/PySide2 support')        
    
    settings.setenv()  # doing this will add/set the additional DYNACONF_ envars
    
    if _DCCSI_GDEBUG or args.export_settings:
        # to do: need to add malformed json validation to <dccsi>\settings.json
        # this can cause a bad error that is deep and hard to debug
        
        # writting settings
        from dynaconf import loaders
        from dynaconf.utils.boxing import DynaBox
    
        _settings_dict = settings.as_dict()
        
        # default temp filename
        _settings_file = Path('settings_export.json.tmp')
        
        # writes to a file, the format is inferred by extension
        # can be .yaml, .toml, .ini, .json, .py
        #loaders.write(_settings_file, DynaBox(data).to_dict(), merge=False, env='development')
        #loaders.write(_settings_file, DynaBox(data).to_dict())
    
        # we want to possibly modify or stash our settings into a o3de .setreg
        from box import Box
        _settings_box = Box(_settings_dict)
    
        _LOGGER.info('Pretty print, _settings_box: {}'.format(_settings_file))
        _LOGGER.info(str(_settings_box.to_json(sort_keys=True,
                                               indent=4)))
        
        # writes settings box
        _settings_box.to_json(filename=_settings_file.as_posix(),
                               sort_keys=True,
                               indent=4)
    
    if _DCCSI_GDEBUG or args.test_pyside2:
        test_pyside2()  # test PySide2 access with a pop-up button
        try:
            import pyside2uic
        except ImportError as e:
            _LOGGER.warning("Could not import 'pyside2uic'")
            _LOGGER.warning("Refer to: '< local DCCsi >\3rdParty\Python\README.txt'")
            _LOGGER.error(e)

    # return
    sys.exit()
# --- END -----------------------------------------------------------------    







