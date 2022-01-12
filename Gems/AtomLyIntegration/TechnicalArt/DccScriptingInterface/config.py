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
import time
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
# global scope
_MODULENAME = 'DCCsi.config'

#os.environ['PYTHONINSPECT'] = 'True'
_MODULE_PATH = os.path.abspath(__file__)

# we don't have access yet to the DCCsi Lib\site-packages
# (1) this will give us import access to azpy (always?)
_PATH_DCCSIG = os.getenv('PATH_DCCSIG',
                         os.path.abspath(os.path.dirname(_MODULE_PATH)))
os.environ['PATH_DCCSIG'] = _PATH_DCCSIG
# ^ we assume this config is in the root of the DCCsi
# if it's not, be sure to set envar 'PATH_DCCSIG' to ensure it
site.addsitedir(_PATH_DCCSIG)  # must be done for azpy
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# now we have azpy api access
import azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG

_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')

# default loglevel to info unless set
_DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))
if _DCCSI_GDEBUG:
    # override loglevel if runnign debug
    _DCCSI_LOGLEVEL = _logging.DEBUG
    
# set up module logging
#for handler in _logging.root.handlers[:]:
    #_logging.root.removeHandler(handler)
    
# configure basic logger
# note: not using a common logger to reduce cyclical imports
_logging.basicConfig(level=_DCCSI_LOGLEVEL,
                    format=FRMT_LOG_LONG,
                    datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
_LOGGER.debug('site.addsitedir({})'.format(_PATH_DCCSIG))
_LOGGER.debug('_DCCSI_GDEBUG: {}'.format(_DCCSI_GDEBUG))
_LOGGER.debug('_DCCSI_DEV_MODE: {}'.format(_DCCSI_DEV_MODE))
_LOGGER.debug('_DCCSI_LOGLEVEL: {}'.format(_DCCSI_LOGLEVEL))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def attach_debugger():
    """This will attemp to attch the WING debugger
    Note: other IDEs for debugging not yet implemented."""
    _DCCSI_GDEBUG = True
    os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)
    
    _DCCSI_DEV_MODE = True
    os.environ["DYNACONF_DCCSI_DEV_MODE"] = str(_DCCSI_DEV_MODE)
    
    from azpy.test.entry_test import connect_wing
    _debugger = connect_wing()
    
    return _debugger
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# this will give us import access to additional modules we provide with DCCsi
_PATH_DCCSI_PYTHON_LIB = azpy.config_utils.bootstrap_dccsi_py_libs(_PATH_DCCSIG)
# this will strap access and put this pattern on the PYTHONPATH
# '{_PATH_DCCSIG}\\3rdParty\\Python\\Lib\\{sys.version_info[0]}.x\\{sys.version_info[0]}.{sys.version_info[1]}.x\\site-packages'
# we want this is always procedurally boostrap
# we won't append it to _DCCSI_PYTHONPATH = list() to be managed
# because we do not want to cache this value so it remains transient
# this will allow any python interpreter to strap it correctly based on version

# Now we should be able to just carry on with pathlib and dynaconf
from dynaconf import Dynaconf
try:
    import pathlib
except:
    import pathlib2 as pathlib  # Maya py2.7 patch
from pathlib import Path

_PATH_DCCSIG = Path(_PATH_DCCSIG)  # pathify
_PATH_DCCSI_PYTHON = Path(_PATH_DCCSIG,'3rdParty','Python')
_PATH_DCCSI_PYTHON_LIB = Path(_PATH_DCCSI_PYTHON_LIB)
# since we always strap access to our package install sandbox
# this should be part of initializing the core
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# start locally prepping known default values for dyanmic environment settings
# these global variables are passed as defaults into methods within module

# special, a global home for stashing PATHs into managed settings
_DCCSI_SYS_PATH = list()

# special, a global home for stashing PYTHONPATHs into managed settings
_DCCSI_PYTHONPATH = list()

# this is a dict bucket to store none-managed settings
_DCCSI_LOCAL_SETTINGS = {}

# this will retreive the O3DE engine root
# Note: this may result in a search IF:
#     O3DE_DEV=< path to engine root > is not set IN:
#         the env or settings.local.json
_O3DE_DEV = azpy.config_utils.get_o3de_engine_root()
# set up dynamic config envars

# To Do: this doesn't perfom properly on installer pre-built engine
# To Do: this is a search and having it here will force it to always run
_PATH_O3DE_BUILD = azpy.config_utils.get_o3de_build_path(_O3DE_DEV,
                                                         'CMakeCache.txt')

from azpy.constants import STR_PATH_O3DE_BIN
_PATH_O3DE_BIN = Path(STR_PATH_O3DE_BIN.format(_PATH_O3DE_BUILD))

# this in most cases will return the project folder
# if it returns a matching engine folder then we don't know the project folder
# To Do: this is a search and having it here will force it to always run
_PATH_O3DE_PROJECT = azpy.config_utils.get_o3de_project_path()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# To Do: move to config utils?
def add_path_list_to_envar(path_list=_DCCSI_SYS_PATH,
                           envar='PATH'):
    """Take in a list of Path objects to add to system ENVAR (like PATH).
    This method explicitly adds the paths to the ENVAR."""
    
    _LOGGER.info('checking envar: {}'.format(envar))
    
    # get or default to empty
    if envar in os.environ:
        _ENVAR = os.getenv(envar, "< NOT SET >")
    else:
        os.environ[envar]='' # create empty
        _ENVAR = os.getenv(envar)
    
    # this is seperated (split) into a list
    known_pathlist = _ENVAR.split(os.pathsep)    
        
    if len(path_list) == 0:
        _LOGGER.warning('No {} paths added, path_list is empty'.format(envar))
        return None
    else:
        _LOGGER.info('Adding paths to envar: {}'.format(envar)) 
        # To Do: more validation and error checking?
        for p in path_list:
            p = Path(p)
            if (p.exists() and p.as_posix() not in known_pathlist):
                os.environ[envar] = p.as_posix() + os.pathsep + os.environ[envar]
                # adding directly to sys.path apparently doesn't work for .dll locations like Qt
                # this pattern by extending the ENVAR seems to work correctly

    return os.environ[envar]
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# To Do: move to config utils?
def list_to_addsitedir(path_list=_DCCSI_PYTHONPATH,
                       envar='PYTHONPATH'):
    """"Take in a list of Path objects to add to system ENVAR (like PYTHONPATH).
    This makes sure each path is fully added as searchable code access.
    Mainly to access site.addsitedir so from imports work in our namespace. """

    _LOGGER.info('checking envar: {}'.format(envar))    
    
    # get or default to empty
    if envar in os.environ:
        _ENVAR = os.getenv(envar, "< NOT SET >")
    else:
        os.environ[envar]='' # create empty
        _ENVAR = os.getenv(envar)
    
    # this is seperated (split) into a list
    known_pathlist = _ENVAR.split(os.pathsep)        
    
    new_pathlist = known_pathlist.copy()
        
    if len(path_list) == 0:
        _LOGGER.warning('No {} paths added, path_list is empty'.format(envar))
        return None
    else:
        _LOGGER.info('Adding paths to envar: {}'.format(envar)) 
        for p in path_list:
            p = Path(p)
            if (p.exists() and p.as_posix() not in known_pathlist):
                sys.path.insert(0, p.as_posix())
                new_pathlist.insert(0, p.as_posix())
                # Add site directory to make from imports work in namespace
                site.addsitedir(p.as_posix())
    
        os.environ[envar] = os.pathsep.join(new_pathlist)        
    
    return os.environ[envar]
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init_o3de_pyside2(dccsi_path=_PATH_DCCSIG,
                      engine_bin_path=_PATH_O3DE_BIN,
                      dccsi_sys_path=_DCCSI_SYS_PATH):
    """Initialize the DCCsi Qt/PySide dynamic env and settings
    sets access to lumberyards Qt dlls and PySide access"""
    time_start = time.process_time() # start tracking
    
    _PATH_DCCSIG = Path(dccsi_path)
    _PATH_O3DE_BIN = Path(engine_bin_path)

    if not _PATH_O3DE_BIN.exists():
        raise Exception('_PATH_O3DE_BIN does NOT exist: {0}'.format(_PATH_O3DE_BIN))
    else:
        pass

    # # allows to retreive from settings.QTFORPYTHON_PATH
    # from azpy.constants import STR_QTFORPYTHON_PATH  # a path string constructor
    # QTFORPYTHON_PATH = Path(STR_QTFORPYTHON_PATH.format(O3DE_DEV)).as_posix()
    # os.environ["DYNACONF_QTFORPYTHON_PATH"] = str(QTFORPYTHON_PATH)
    # site.addsitedir(str(QTFORPYTHON_PATH))  # PYTHONPATH

    QT_PLUGIN_PATH = Path.joinpath(_PATH_O3DE_BIN,'EditorPlugins')
    os.environ["DYNACONF_QT_PLUGIN_PATH"] = str(QT_PLUGIN_PATH.as_posix())
    dccsi_sys_path.append(QT_PLUGIN_PATH)

    QT_QPA_PLATFORM_PLUGIN_PATH = Path.joinpath(QT_PLUGIN_PATH, 'platforms')
    os.environ["DYNACONF_QT_QPA_PLATFORM_PLUGIN_PATH"] = str(QT_QPA_PLATFORM_PLUGIN_PATH.as_posix())
    # if the line below is removed external standalone apps can't load PySide2
    os.environ["QT_QPA_PLATFORM_PLUGIN_PATH"] = str(QT_QPA_PLATFORM_PLUGIN_PATH.as_posix())
    # ^^ bypass trying to set only with DYNACONF environment
    dccsi_sys_path.append(QT_QPA_PLATFORM_PLUGIN_PATH)
    
    # Access to QtForPython:
    # PySide2 is added to the O3DE Python install during CMake built aginst O3DE Qt dlls
    # the only way import becomes an actual issue is if running this config script
    # within a completely foreign python interpretter,
    # and/or a non-Qt app (or a DCC tool that is a Qt app but doesn't provide PySide2 bidnings)
    # To Do: tackle this when it becomes and issue (like Blender?)

    from dynaconf import settings    
    
    time_complete = time.process_time() - time_start 
    _LOGGER.info('~   config.init_o3de_pyside() DONE: {} sec'.format(time_complete))

    return settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init_o3de_pyside2_tools():
    # set up the pyside2-tools (pyside2uic)
    _DCCSI_PYSIDE2_TOOLS = Path(_PATH_DCCSI_PYTHON,'pyside2-tools')
    if _DCCSI_PYSIDE2_TOOLS.exists():
        os.environ["DYNACONF_DCCSI_PYSIDE2_TOOLS"] = str(_DCCSI_PYSIDE2_TOOLS.as_posix())
        os.environ['PATH'] = _DCCSI_PYSIDE2_TOOLS.as_posix() + os.pathsep + os.environ['PATH']
    
        site.addsitedir(_DCCSI_PYSIDE2_TOOLS)
        _DCCSI_PYTHONPATH.append(_DCCSI_PYSIDE2_TOOLS.as_posix())
        _LOGGER.info('~   PySide2-Tools bootstrapped PATH for Windows.')
        try:
            import pyside2uic
            _LOGGER.info('~   SUCCESS: import pyside2uic')
            _LOGGER.debug(pyside2uic)
            status = True
        except ImportError as e:
            _LOGGER.error('~   FAILURE: import pyside2uic')
            status = False
            raise(e)
    else:
        _LOGGER.warning('~   No PySide2 Tools: {}'.format(_DCCSI_PYSIDE2_TOOLS.as_posix()))    
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def validate_o3de_pyside2():
    # add Qt binaries to the Windows path to handle finding DLL file dependencies
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
                   engine_bin_path=_PATH_O3DE_BIN,
                   project_name=None,
                   project_path=_PATH_O3DE_PROJECT,
                   dccsi_path=_PATH_DCCSIG,
                   dccsi_sys_path=_DCCSI_SYS_PATH):
    """Initialize the DCCsi Core dynamic env and settings."""
    
    time_start = time.process_time() # start tracking for perf
    
    # we don't do this often but we want to stash to global dict
    global _DCCSI_LOCAL_SETTINGS # non-dynaconf managed settings
    
    # ENVARS with DYNACONF_ are managed and exported to settings
    # These envars are set like the following
    #     from dynaconf import settings
    #     settings.setenv()
    #     if settings.DCCSI_GDEBUG: do this
    # ENVARS without prefix are transient and local to session
    # They will not be written to settings the impact of this is,
    # they can not be retreived like (will assert):
    #     foo = settings.DCCSI_GDEBUG
    
    # ensures dccsi python extension lib sandbox is bootstrapped
    dccsi_python_lib = azpy.config_utils.bootstrap_dccsi_py_libs(dccsi_path)
    # dont' append it to _DCCSI_PYTHONPATH = list() to be managed    
    
    # `envvar_prefix` = export envvars with `export DYNACONF_FOO=bar`.
    # `settings_files` = Load this files in the order.
    # here we are modifying or adding to the dynamic config settings on import
    
    # this code block commented out below is how to change the prefix
    # and the order in which to load settings (settings.py,.json already default)
    #settings = Dynaconf(envvar_prefix='DYNACONF',
                        ## the following will also load settings.local.json
                        #settings_files=['settings.json',
                                        #'.secrets.json'])
    
    # global settings
    os.environ["DYNACONF_DCCSI_OS_FOLDER"] = azpy.config_utils.get_os()
    os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)
    os.environ["DYNACONF_DCCSI_DEV_MODE"] = str(_DCCSI_DEV_MODE)
    os.environ['DYNACONF_DCCSI_LOGLEVEL'] = str(_DCCSI_LOGLEVEL)
    
    dccsi_path = Path(dccsi_path)
    os.environ["DYNACONF_PATH_DCCSIG"] = str(dccsi_path.as_posix())
    
    # we store these and will manage them last (as a combined set)
    dccsi_sys_path.append(dccsi_path)

    # we already defaulted to discovering these two early because of importance
    os.environ["DYNACONF_O3DE_DEV"] = str(_O3DE_DEV.as_posix())

    # the project is transient and optional (falls back to dccsi)
    # it's more important to set this for tools that want to work with project
    # which is why you can pass it in explicitly
    if project_path:
        _project_path = Path(project_path)
        try:
            _project_path.exists()
            _PATH_O3DE_PROJECT = _project_path
        except FileExistsError as e:
            _LOGGER.error('~   The project path specified does not appear to exist!')
            _LOGGER.warning('~   project_path: {}'.format(project_path))
            _LOGGER.warning('~   fallback to engine root: {}'.format())
            _PATH_O3DE_PROJECT = _O3DE_DEV

        os.environ['PATH_O3DE_PROJECT'] = str(_PATH_O3DE_PROJECT.as_posix())
        _DCCSI_LOCAL_SETTINGS['PATH_O3DE_PROJECT'] = str(_PATH_O3DE_PROJECT.as_posix())

    # we can pull the O3DE_PROJECT (name) from the project path
    if not project_name:
        project_name = Path(_PATH_O3DE_PROJECT).name
    os.environ["O3DE_PROJECT"] = str(project_name)
    _DCCSI_LOCAL_SETTINGS['O3DE_PROJECT'] = str(project_name)

    # -- O3DE build -- set up \bin\path (for Qt dll access)
    # To Do: note that this doesn't safely discover the build path
    # when using the installer with pre-built engine (CMakeCache doesn't exist)
    _PATH_O3DE_BUILD = Path(azpy.config_utils.get_o3de_build_path(_O3DE_DEV,
                                                                  'CMakeCache.txt'))
    os.environ["DYNACONF_PATH_O3DE_BUILD"] = str(_PATH_O3DE_BUILD.as_posix())
    
    _PATH_O3DE_BIN = Path(STR_PATH_O3DE_BIN.format(_PATH_O3DE_BUILD))
    os.environ["DYNACONF_PATH_O3DE_BIN"] = str(_PATH_O3DE_BIN.as_posix())
    
    # hard check
    if not _PATH_O3DE_BIN.exists():
        raise Exception('PATH_O3DE_BIN does NOT exist: {0}'.format(_PATH_O3DE_BIN))
    else:
        dccsi_sys_path.append(_PATH_O3DE_BIN)
    # --
    
    from azpy.constants import TAG_DIR_DCCSI_TOOLS
    _PATH_DCCSI_TOOLS = Path(dccsi_path, TAG_DIR_DCCSI_TOOLS)
    os.environ["DYNACONF_PATH_DCCSI_TOOLS"] = str(_PATH_DCCSI_TOOLS.as_posix())
    
    from azpy.constants import TAG_DCCSI_NICKNAME
    from azpy.constants import PATH_DCCSI_LOG_PATH
    _DCCSI_LOG_PATH = Path(PATH_DCCSI_LOG_PATH.format(PATH_O3DE_PROJECT=project_path,
                                                      TAG_DCCSI_NICKNAME=TAG_DCCSI_NICKNAME))
    os.environ["DYNACONF_DCCSI_LOG_PATH"] = str(_DCCSI_LOG_PATH.as_posix())    
    
    from azpy.constants import TAG_DIR_REGISTRY, TAG_DCCSI_CONFIG
    _PATH_DCCSI_CONFIG = Path(project_path, TAG_DIR_REGISTRY, TAG_DCCSI_CONFIG)
    os.environ["DYNACONF_PATH_DCCSI_CONFIG"] = str(_PATH_DCCSI_CONFIG.as_posix())
    
    from azpy.constants import TAG_DIR_DCCSI_TOOLS
    _DCCSIG_TOOLS_PATH = Path.joinpath(dccsi_path, TAG_DIR_DCCSI_TOOLS)
    os.environ["DYNACONF_DCCSIG_TOOLS_PATH"] = str(_DCCSIG_TOOLS_PATH.as_posix())
    
    # To Do: Need to add some json validation for settings.json,
    # and settings.local.json, as importing settings will read json data
    # if the json is malformed it will cause an assert like:
    # "json.decoder.JSONDecodeError: Expecting ',' delimiter: line 45 column 5 (char 2463)"
    # but might be hard to quickly find out which serttings file is bad
    from dynaconf import settings    
    
    time_complete = time.process_time() - time_start 
    _LOGGER.info('~   config.init_o3de_core() DONE: {} sec'.format(time_complete))
    
    # we return dccsi_sys_path because we may want to pass it on
    # settings my be passes on and added to or re-initialized with changes
    return dccsi_sys_path, settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init_o3de_python(engine_path=_O3DE_DEV,
                     engine_bin_path=_PATH_O3DE_BIN,
                     dccsi_path=_PATH_DCCSIG,
                     dccsi_sys_path=_DCCSI_SYS_PATH,
                     dccsi_pythonpath=_DCCSI_PYTHONPATH):
    """Initialize the O3DE dynamic Python env and settings.
    Note: """
    
    time_start = time.process_time() 
    
    # pathify
    _O3DE_DEV = Path(engine_path)
    _PATH_O3DE_BIN = Path(engine_bin_path)
    _PATH_DCCSIG = Path(dccsi_path)
    
    # python config
    _PATH_DCCSI_PYTHON = Path(_PATH_DCCSIG,'3rdParty','Python')
    os.environ["DYNACONF_PATH_DCCSI_PYTHON"] = str(_PATH_DCCSI_PYTHON.as_posix())
    
    _PATH_DCCSI_PYTHON_LIB = azpy.config_utils.bootstrap_dccsi_py_libs(_PATH_DCCSIG)
    os.environ["DYNACONF_PATH_DCCSI_PYTHON_LIB"] = str(_PATH_DCCSI_PYTHON_LIB.as_posix())
    dccsi_pythonpath.append(_PATH_DCCSI_PYTHON_LIB)
        
    # this is transient and will always track the exe this script is executing on
    _O3DE_PY_EXE = Path(sys.executable)
    _DCCSI_PY_IDE = Path(_O3DE_PY_EXE)
    os.environ["DCCSI_PY_IDE"] = str(_DCCSI_PY_IDE.as_posix())
    _DCCSI_LOCAL_SETTINGS['DCCSI_PY_IDE']= str(_DCCSI_PY_IDE.as_posix())
    
    _O3DE_PYTHONHOME = Path(_O3DE_PY_EXE.parents[0])
    os.environ["DYNACONF_O3DE_PYTHONHOME"] = str(_O3DE_PYTHONHOME.as_posix())
    dccsi_sys_path.append(_O3DE_PYTHONHOME)
    _LOGGER.info('~   O3DE_PYTHONHOME - is now the folder containing O3DE python executable')
    
    # this will always be the O3DE pythin interpreter
    _PATH_O3DE_PYTHON_INSTALL = Path(_O3DE_DEV, 'python')
    os.environ["DYNACONF_PATH_O3DE_PYTHON_INSTALL"] = str(_PATH_O3DE_PYTHON_INSTALL.as_posix())
    dccsi_sys_path.append(_PATH_O3DE_PYTHON_INSTALL) 

    if sys.platform.startswith('win'):
        _DCCSI_PY_BASE = Path(_PATH_O3DE_PYTHON_INSTALL, 'python.cmd')
    elif sys.platform == "linux":
        _DCCSI_PY_BASE = Path(_PATH_O3DE_PYTHON_INSTALL, 'python.sh')
    elif sys.platform == "darwin":
        _DCCSI_PY_BASE = Path(_PATH_O3DE_PYTHON_INSTALL, 'python.sh')
    else:
        _DCCSI_PY_BASE = None
        
    # this is the system specific shell around O3DE Python install
    if _DCCSI_PY_BASE:
        os.environ["DYNACONF_DCCSI_PY_BASE"] = str(_DCCSI_PY_BASE.as_posix())        

    from dynaconf import settings    
    
    time_complete = time.process_time() - time_start
    _LOGGER.info('~   config.init_o3de_python() ... DONE: {} sec'.format(time_complete))
    
    return dccsi_sys_path, dccsi_pythonpath, settings
# -------------------------------------------------------------------------



# settings.setenv()  # doing this will add the additional DYNACONF_ envars
def get_config_settings(engine_path=_O3DE_DEV,
                        engine_bin_path=None,
                        project_name=None,
                        project_path=_PATH_O3DE_PROJECT,
                        enable_o3de_python=False,
                        enable_o3de_pyside2=False,
                        enable_o3de_pyside2_tools=False,
                        set_env=True,
                        dccsi_path=_PATH_DCCSIG,
                        dccsi_sys_path=_DCCSI_SYS_PATH,
                        dccsi_pythonpath=_DCCSI_PYTHONPATH):
    """Convenience method to initialize and retreive settings directly from module."""
    
    time_start = time.process_time() # start tracking
    
    dccsi_path = Path(dccsi_path)
    
    # ensures dccsi python extension lib sandbox is bootstrapped
    dccsi_python_lib = azpy.config_utils.bootstrap_dccsi_py_libs(dccsi_path)
    # dont' append it to _DCCSI_PYTHONPATH = list() to be managed
    
    # always init the core
    dccsi_sys_path, settings = init_o3de_core(engine_path,
                                              engine_bin_path,
                                              project_name,
                                              project_path,
                                              dccsi_path,
                                              dccsi_sys_path)

    # These should ONLY be set for O3DE and non-DCC environments
    # They will most likely cause other Qt/PySide DCC apps to fail
    # or hopefully they can be overridden for DCC envionments
    if enable_o3de_python:
        dccsi_sys_path, dccsi_pythonpath, settings = init_o3de_python(settings.O3DE_DEV,
                                                                      settings.PATH_O3DE_BIN,
                                                                      settings.PATH_DCCSIG,
                                                                      dccsi_sys_path,
                                                                      dccsi_pythonpath)
    
    # that provide their own Qt dlls and Pyside2
    # _LOGGER.info('QTFORPYTHON_PATH: {}'.format(settings.QTFORPYTHON_PATH))
    # _LOGGER.info('QT_PLUGIN_PATH: {}'.format(settings.QT_PLUGIN_PATH))
    # assume our standalone python tools wants this access?
    # it's safe to do this for dev and from ide
    if enable_o3de_pyside2:
        settings = init_o3de_pyside2(settings.PATH_DCCSIG,
                                     settings.PATH_O3DE_BIN,
                                     dccsi_sys_path)
        
    # final stage, if we have managed path lists set them
    new_PATH = add_path_list_to_envar(dccsi_sys_path)
    new_PYTHONPATH = list_to_addsitedir(dccsi_pythonpath)

    # now standalone we can validate the config. env, settings.
    from dynaconf import settings
    if set_env:
        settings.setenv()
    
    time_complete = time.process_time() - time_start 
    _LOGGER.info('~   config.get_config_settings() DONE TOTAL: {} sec'.format(time_complete))
    
    return settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def export_settings(settings,
                    dccsi_sys_path=_DCCSI_SYS_PATH,
                    dccsi_pythonpath=_DCCSI_PYTHONPATH):
        # to do: need to add malformed json validation to <dccsi>\settings.json
        # this can cause a bad error that is deep and hard to debug
    
        os.environ['DYNACONF_DCCSI_LOCAL_SETTINGS'] = "True"  
    
        # make sure the dnyaconf synthetic env is updated before writting settings
        # this will make it into our settings file as "DCCSI_SYS_PATH":<value>
        add_path_list_to_envar(dccsi_sys_path, 'DYNACONF_DCCSI_SYS_PATH')
        add_path_list_to_envar(dccsi_pythonpath, 'DYNACONF_DCCSI_PYTHONPATH')
        # we did not use list_to_addsitedir() because we are just ensuring
        # the managed envar path list is updated so paths are written to settings
        
        # update settings
        from dynaconf import settings
        settings.setenv()
        
        # writting settings
        from dynaconf import loaders
        from dynaconf.utils.boxing import DynaBox
    
        _settings_dict = settings.as_dict()
        
        # default temp filename
        _settings_file = Path('settings.local.json')
        
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
# --- END -----------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone cli script for testing/debugging"""
    import time
    main_start = time.process_time() # start tracking
    
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
    _LOGGER.info('~ {}.py ... Running script as __main__'.format(_MODULENAME))
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
    parser.add_argument('-sd', '--set-debugger',
                        type=str,
                        required=False,
                        help='Default debugger: WING, (not implemented) others: PYCHARM and VSCODE.')
    parser.add_argument('-dm', '--developer-mode',
                        type=bool,
                        required=False,
                        help='Enables dev mode for early auto attaching debugger.')
    parser.add_argument('-ep', '--engine-path',
                        type=pathlib.Path,
                        required=False,
                        help='The path to the o3de engine.')
    parser.add_argument('-eb', '--engine-bin-path',
                        type=pathlib.Path,
                        required=False,
                        help='The path to the o3de engine binaries (build/bin/profile).')
    parser.add_argument('-bf', '--build-folder',
                        type=str,
                        required=False,
                        help='The name (tag) of the o3de build folder, example build or windows.')
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
                        help='Enables O3DE Qt & PySide2 access.')
    parser.add_argument('-pc', '--project-config',
                        type=bool,
                        required=False,
                        help='(not implemented) Enables reading the < >project >/registry/dccsi_configuration.setreg.')
    parser.add_argument('-es', '--export-settings',
                        type=pathlib.Path,
                        required=False,
                        help='(not implemented) Writes managed settings to specified path.')
    parser.add_argument('-ec', '--export-configuration',
                        type=bool,
                        required=False,
                        help='(not implemented) writes settings as a O3DE < project >/registry/dccsi_configuration.setreg.')
    parser.add_argument('-tp', '--test-pyside2',
                        type=bool,
                        required=False,
                        help='Runs Qt/PySide2 tests and reports.')
    args = parser.parse_args()

    # easy overrides
    if args.global_debug:
        _DCCSI_GDEBUG = True
        os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)

    if args.set_debugger:
        _LOGGER.info('Setting and switching debugger type not implemented (default=WING)')
        # To Do: implement debugger plugin pattern
        
    if args.developer_mode:
        _DCCSI_DEV_MODE = True
        attach_debugger()  # attempts to start debugger
        
    # need to do a little plumbing
    if not args.engine_path:
        args.engine_path=_O3DE_DEV
    if not args.build_folder:
        from azpy.constants import TAG_DIR_O3DE_BUILD_FOLDER
        args.build_folder = TAG_DIR_O3DE_BUILD_FOLDER
    if not args.engine_bin_path:
        args.engine_bin_path=_PATH_O3DE_BIN
    if not args.project_path:
        args.project_path=_PATH_O3DE_PROJECT
        
    if _DCCSI_GDEBUG:
        args.enable_python = True
        args.enable_qt = True

    # now standalone we can validate the config. env, settings.
    settings = get_config_settings(engine_path=args.engine_path,
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
    _LOGGER.info('O3DE_O3DE_BUILD_FOLDER: {}'.format(settings.PATH_O3DE_BUILD))
    _LOGGER.info('PATH_O3DE_BUILD: {}'.format(settings.PATH_O3DE_BUILD))
    _LOGGER.info('PATH_O3DE_BIN: {}'.format(settings.PATH_O3DE_BIN))
    
    _LOGGER.info('PATH_DCCSIG: {}'.format(settings.PATH_DCCSIG))
    _LOGGER.info('DCCSI_LOG_PATH: {}'.format(settings.DCCSI_LOG_PATH))
    _LOGGER.info('PATH_DCCSI_CONFIG: {}'.format(settings.PATH_DCCSI_CONFIG))

    # settings no longer managed with DYNACONF_ in config.py
    #_LOGGER.info('O3DE_PROJECT: {}'.format(settings.O3DE_PROJECT))
    #_LOGGER.info('PATH_O3DE_PROJECT: {}'.format(settings.PATH_O3DE_PROJECT))    
    
    try:
        settings.O3DE_DCCSI_ENV_TEST
        _LOGGER.info('O3DE_DCCSI_ENV_TEST: {}'.format(settings.O3DE_DCCSI_ENV_TEST))
    except:
        pass # don't exist
    
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('')
    
    if args.enable_python:
        _LOGGER.info(STR_CROSSBAR)
        _LOGGER.info('PATH_DCCSI_PYTHON'.format(settings.PATH_DCCSI_PYTHON))
        _LOGGER.info('PATH_DCCSI_PYTHON_LIB: {}'.format(settings.PATH_DCCSI_PYTHON_LIB))
        _LOGGER.info('O3DE_PYTHONHOME'.format(settings.O3DE_PYTHONHOME))
        _LOGGER.info('PATH_O3DE_PYTHON_INSTALL'.format(settings.PATH_O3DE_PYTHON_INSTALL))
        _LOGGER.info('DCCSI_PY_BASE: {}'.format(settings.DCCSI_PY_BASE))
        _LOGGER.info(STR_CROSSBAR)
        _LOGGER.info('')

        # settings no longer managed with DYNACONF_ in config.py
        #_LOGGER.info('DCCSI_PY_IDE'.format(settings.DCCSI_PY_IDE))        
    else:
        _LOGGER.info('Tip: add arg --enable-python (-py) to extend the environment with O3DE python access')
        
    if args.enable_qt:
        _LOGGER.info(STR_CROSSBAR)
        # _LOGGER.info('QTFORPYTHON_PATH: {}'.format(settings.QTFORPYTHON_PATH))
        _LOGGER.info('QT_PLUGIN_PATH: {}'.format(settings.QT_PLUGIN_PATH))
        _LOGGER.info('QT_QPA_PLATFORM_PLUGIN_PATH: {}'.format(settings.QT_QPA_PLATFORM_PLUGIN_PATH))
        try:
            settings.DCCSI_PYSIDE2_TOOLS
            _LOGGER.info('DCCSI_PYSIDE2_TOOLS: {}'.format(settings.DCCSI_PYSIDE2_TOOLS))  
        except:
            pass # don't exist
        _LOGGER.info(STR_CROSSBAR)
        _LOGGER.info('')
    else:
        _LOGGER.info('Tip: add arg --enable-qt (-qt) to extend the environment with O3DE Qt/PySide2 support')      
        _LOGGER.info('Tip: add arg --test-pyside2 (-tp) to test the O3DE Qt/PySide2 support')    
    
    settings.setenv()  # doing this will add/set the additional DYNACONF_ envars
    
    if _DCCSI_GDEBUG or args.export_settings:
        export_settings(settings)
        # this should be set if there are local settings!?
        _LOGGER.debug('DCCSI_LOCAL_SETTINGS: {}'.format(settings.DCCSI_LOCAL_SETTINGS))
        
    # end tracking here, the pyside test exits before hitting the end of script
    _LOGGER.info('DCCsi: config.py TOTAL: {} sec'.format(time.process_time() - main_start)) 
    
    if _DCCSI_GDEBUG or args.test_pyside2:
        test_pyside2()  # test PySide2 access with a pop-up button
        try:
            import pyside2uic
        except ImportError as e:
            _LOGGER.warning("Could not import 'pyside2uic'")
            _LOGGER.warning("Refer to: '{}/3rdParty/Python/README.txt'".format(settings.PATH_DCCSIG))
            _LOGGER.error(e)

    # return
    sys.exit()
# --- END -----------------------------------------------------------------    