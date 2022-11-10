#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

"""! The dccsi Core configuration module

:file: < DCCsi >/config.py
:Status: Prototype
:Version: 0.0.1, first significant refactor

This module handles core configuration of the dccsi
-   It initializes and generates a dynamic, and synthetic, environment context,
    and settings.
-   ConfigCore class, inherits from DccScriptingInterface.azpy.config_class.ConfigClass (not yet)
-   This module uses dynaconf (a dynamic configuration and settings package)

This config.py synthetic env can be overridden or extended with a local .env
Example, create such as file:
    "< o3de >/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/.env"

    ^ this is only appropriate to a developer making local settings changes,
    primarily to outside of this module, and want those settings to persist
    across the dccsi modules which draw from this core config.py
    See: example.env.tmp (copy and rename to .env)

    This file should not be committed

The second way to locally override or persist settings, is to make changes in
the file:
    DccScriptingInterface/settings.local.json

    ^ this file can be generated via the core config.py cli
    or from

If you only want to make transient envar settings changes that only persist to
an IDE session, you can create and modify this file:
"C:/Depot/o3de-dev/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/Dev/Windows/Env_Dev.bat"
(copy and rename Env_Dev.bat.example to Env_Dev.bat) this only modifies the launcher environments.
^ this is a good option is good if you only want to alter the behavior while in IDE,
or if you are troubleshooting a broken dccsi and/or modifying config.py

The third option, is there are a few points in this module to can temporarily,
force the default value returned if an envar is not set externally.

# example envar default                      .....
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)

To Do: ensure that we can stack/layer the dynamic env to work with O3DE project config
^ this means the following:
- this module can load a stashed settings file (from anywhere?)
- so any tool using it, can have the core/synthetic env and also load specific settings
- <project>/registry/dccsi_config.setreg provides persistent distributable configuration
- <user home>/.o3de/registry/dccsi_config.setreg, user only extended/override settings
"""
import timeit
_MODULE_START = timeit.default_timer()  # start tracking

# standard imports
import os
import sys
import site
import re
import pathlib
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# module global scope, set up module logging, etc.
from DccScriptingInterface import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.config'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}.')
_MODULE_PATH = Path(__file__)  # what if frozen?
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# dccsi global scope, ensure site access to dccsi
from DccScriptingInterface.constants import *
from DccScriptingInterface.globals import *

import DccScriptingInterface.azpy.test.entry_test
import DccScriptingInterface.azpy.config_utils

# set this manually if you want to raise exceptions/warnings while debugging
DCCSI_STRICT = False
# allow package to capture warnings
#_logging.captureWarnings(capture=True)

if DCCSI_DEV_MODE:
    # if dev mode, this will attempt to auto-attach the debugger
    # at the earliest possible point in this module
    DccScriptingInterface.azpy.test.entry_test.connect_wing()

# we should be able to import dccsi pkg dependencies now
try:
    from dynaconf import Dynaconf
except ImportError as e:
    _LOGGER.warning(f'Could not import dynaconf')
    _LOGGER.warning(f'Most likely the application loading this module does not have DCCsi pkg dependancies installed. Use foundation.py to install pkgs for ')
    _LOGGER.exception(f'{e} , traceback =', exc_info=True)
    raise e
#-------------------------------------------------------------------------


# -------------------------------------------------------------------------
# going to optimize config to speed things up

# the fastest way to know the engine root is to ...
# see if the engine is running (azlmbr.paths.engroot)
# or check if dev set in env, both those things happen here

from DccScriptingInterface import O3DE_DEV
from DccScriptingInterface import PATH_O3DE_PROJECT
from DccScriptingInterface import O3DE_PROJECT
from DccScriptingInterface import PATH_O3DE_BIN
from DccScriptingInterface import PATH_O3DE_3RDPARTY
from DccScriptingInterface import PATH_O3DE_TECHART_GEMS
from DccScriptingInterface import PATH_DCCSIG
from DccScriptingInterface import PATH_DCCSI_PYTHON_LIB

# O3DE_DEV could be set in .env or settings.local.json to override config

# this call with search fallback will be deprecated hopefully
#O3DE_DEV = DccScriptingInterface.azpy.config_utils.get_o3de_engine_root()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# refactor this module into a class object, other configs can inherit
from DccScriptingInterface.azpy.config_class import ConfigClass

# it is suggested that the core <dccsi>\config.py is re-written
class CoreConfig(ConfigClass):
    """A class representing the DCCsi core config"""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        _LOGGER.info(f'Initializing: {self.get_classname()}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# build the config
core_config = CoreConfig(config_name='dccsi_core', auto_set=True)

# add settings to config here
# foo
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# local global scope
# start locally prepping known default values for dynamic environment settings
# these global variables are passed as defaults into methods within the module

# special, a global home for stashing PATHs for managed settings
global _DCCSI_SYS_PATH
_DCCSI_SYS_PATH = list()

# special, a global home for stashing PYTHONPATHs for managed settings
global _DCCSI_PYTHONPATH
_DCCSI_PYTHONPATH = list()

# special, stash local PYTHONPATHs in a non-managed way (won't end up in settings.local.json)
global _DCCSI_PYTHONPATH_EXCLUDE
_DCCSI_PYTHONPATH_EXCLUDE = list()

# this is a dict bucket to store none-managed settings (fully local to module)
global _DCCSI_LOCAL_SETTINGS
_DCCSI_LOCAL_SETTINGS = {}

# access the the o3de scripts so we can utilize things like o3de.py
PATH_O3DE_PYTHON_SCRIPTS = Path(O3DE_DEV, 'scripts').resolve()
site.addsitedir(PATH_O3DE_PYTHON_SCRIPTS.as_posix())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# An improvement would be to move this to azpy.config_utils
# and be refactored to utilize py3+ fstrings
def add_path_list_to_envar(path_list=_DCCSI_SYS_PATH,
                           envar='PATH'):
    """!
    Take in a list of Path objects to add to system ENVAR (like PATH).
    This method explicitly adds the paths to the system ENVAR.
    @param path_list: a list() of paths
    @param envar: add paths to this ENVAR
    """

    # this method is called a lot and has become verbose,
    # it's suggested to clean this up when the config is next refactored

    _LOGGER.info('checking envar: {}'.format(envar))

    # get or default to empty
    if envar in os.environ:
        _ENVAR = os.getenv(envar, "< NOT SET >")
    else:
        os.environ[envar]='' # create empty
        _ENVAR = os.getenv(envar)

    # this is separated (split) into a list
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
                # adding directly to sys.path apparently doesn't work
                # for .dll locations like Qt
                # this pattern by extending the ENVAR seems to work correctly
                os.environ[envar] = p.as_posix() + os.pathsep + os.environ[envar]

    return os.environ[envar]
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# An improvement would be to move this to azpy.config_utils
# and be refactored to utilize py3+ fstrings
def add_path_list_to_addsitedir(path_list=_DCCSI_PYTHONPATH,
                                envar='PYTHONPATH'):
    """"!
    Take in a list of Path objects to add to system ENVAR (like PYTHONPATH).
    This makes sure each path is fully added as searchable code access.
    Mainly to use/access site.addsitedir so from imports work in our namespace.
    @param path_list: a list() of paths
    @param envar: add paths to this ENVAR (and site.addsitedir)
    """

    # this method is called a lot and has become verbose,
    # it's suggested to clean this up when the config is next refactored

    _LOGGER.info('checking envar: {}'.format(envar))

    # get or default to empty
    if envar in os.environ:
        _ENVAR = os.getenv(envar, "< NOT SET >")
    else:
        os.environ[envar]='' # create empty
        _ENVAR = os.getenv(envar)

    # this is separated (split) into a list
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
def init_o3de_pyside2(dccsi_sys_path=_DCCSI_SYS_PATH):
    """!
    Initialize the DCCsi Qt/PySide dynamic env and settings
    sets access to lumberyards Qt dlls and PySide access,
    which are built into the /bin folder
    @param dccsi_path
    @param engine_bin_path
    @param dccsi_sys_path
    """
    time_start = timeit.default_timer()  # start tracking

    # we don't do this often but we want to stash to global dict
    global _DCCSI_LOCAL_SETTINGS # non-dynaconf managed settings

    if not PATH_O3DE_BIN.exists():
        raise Exception(f'_PATH_O3DE_BIN does NOT exist: {PATH_O3DE_BIN}')
    else:
        pass

    # # allows to retrieve from settings.QTFORPYTHON_PATH
    # from DccScriptingInterface.azpy.constants import STR_QTFORPYTHON_PATH  # a path string constructor
    # QTFORPYTHON_PATH = Path(STR_QTFORPYTHON_PATH.format(O3DE_DEV)).as_posix()
    # os.environ["DYNACONF_QTFORPYTHON_PATH"] = str(QTFORPYTHON_PATH)
    # site.addsitedir(str(QTFORPYTHON_PATH))  # PYTHONPATH

    QT_PLUGIN_PATH = Path.joinpath(PATH_O3DE_BIN, 'EditorPlugins').resolve()
    os.environ["QT_PLUGIN_PATH"] = str(QT_PLUGIN_PATH.as_posix())
    _DCCSI_LOCAL_SETTINGS['QT_PLUGIN_PATH'] = QT_PLUGIN_PATH.as_posix()
    dccsi_sys_path.append(QT_PLUGIN_PATH)

    QT_QPA_PLATFORM_PLUGIN_PATH = Path.joinpath(QT_PLUGIN_PATH, 'platforms').resolve()
    # if the line below is removed external standalone apps can't load PySide2
    os.environ["QT_QPA_PLATFORM_PLUGIN_PATH"] = QT_QPA_PLATFORM_PLUGIN_PATH.as_posix()
    # ^^ bypass trying to set only with DYNACONF environment
    _DCCSI_LOCAL_SETTINGS['QT_QPA_PLATFORM_PLUGIN_PATH'] = QT_QPA_PLATFORM_PLUGIN_PATH.as_posix()
    dccsi_sys_path.append(QT_QPA_PLATFORM_PLUGIN_PATH)

    # Access to QtForPython:
    # PySide2 is added to the O3DE Python install during CMake built against O3DE Qt dlls
    # the only way import becomes an actual issue is if running this config script
    # within a completely foreign python interpreter,
    # and/or a non-Qt app (or a DCC tool that is a Qt app but doesn't provide PySide2 bindings)
    # To Do: tackle this when it becomes and issue (like Blender?)

    from dynaconf import settings

    time_complete = timeit.default_timer() - time_start
    _LOGGER.info(f'~   config.init_o3de_pyside() DONE: {time_complete} sec')

    return settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init_o3de_pyside2_tools():
    global _DCCSI_PYTHONPATH
    # set up the pyside2-tools (pyside2uic)
    _DCCSI_PYSIDE2_TOOLS = Path(PATH_DCCSI_PYTHON, 'pyside2-tools').resolve()
    if _DCCSI_PYSIDE2_TOOLS.exists():
        os.environ["DYNACONF_DCCSI_PYSIDE2_TOOLS"] = _DCCSI_PYSIDE2_TOOLS.as_posix()
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
        _LOGGER.warning(f'~   No PySide2 Tools: {_DCCSI_PYSIDE2_TOOLS.as_posix()}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def validate_o3de_pyside2():
    # To Do: finish implementing
    # To Do: doc strings
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
def test_pyside2(exit=True):
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

    if exit:
        sys.exit(app.exec_())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init_o3de_core(engine_path=O3DE_DEV,
                   engine_bin_path=PATH_O3DE_BIN,
                   engine_3rdParty=PATH_O3DE_3RDPARTY,
                   project_name=O3DE_PROJECT,
                   project_path=PATH_O3DE_PROJECT,
                   dccsi_path=PATH_DCCSIG,
                   dccsi_sys_path=_DCCSI_SYS_PATH,
                   dccsi_pythonpath=_DCCSI_PYTHONPATH):
    """Initialize the DCCsi Core dynamic env and settings."""

    time_start = timeit.default_timer()  # start tracking for perf

    # we don't do this often but we want to stash to global dict
    global _DCCSI_LOCAL_SETTINGS # non-dynaconf managed settings

    # ENVARS with DYNACONF_ are managed and exported to settings
    # These envars are set like the following
    #     os.environ["DYNACONF_DCCSI_EXAMPLE_KEY"] = Path('example','path).as_posix()
    #     from dynaconf import settings
    #     settings.setenv()
    #     if settings.DCCSI_EXAMPLE_KEY: do this code
    # ENVARS without prefix are transient and local to session
    # They will not be written to settings the impact of this is,
    # they can not be retrieved like this (will assert):
    #     os.environ["DCCSI_EXAMPLE_KEY"] = Path('example','path).as_posix()
    #     foo = settings.DCCSI_GDEBUG

    # `envar_prefix` = export envars with `export DYNACONF_FOO=bar`.
    # `settings_files` = Load these files in this order.
    # here we are modifying or adding to the dynamic config settings on import

    # this code block commented out below is how to change the prefix
    # and the order in which to load settings (settings.py,.json already default)
    #settings = Dynaconf(envar_prefix='DYNACONF',
                        ## the following will also load settings.local.json
                        #settings_files=['settings.json',
                                        #'.secrets.json'])

    # for validation of this core config module
    # as we may layer and fold config's together.
    os.environ['DYNACONF_DCCSI_CONFIG_CORE'] = "True"

    # global settings
    os.environ["DYNACONF_DCCSI_GDEBUG"] = str(DCCSI_GDEBUG)  # cast bools
    os.environ["DYNACONF_DCCSI_DEV_MODE"] = str(DCCSI_DEV_MODE)
    os.environ['DYNACONF_DCCSI_LOGLEVEL'] = str(DCCSI_LOGLEVEL)

    dccsi_path = Path(dccsi_path)
    os.environ["DYNACONF_PATH_DCCSIG"] = dccsi_path.as_posix()

    # we store these and will manage them last (as a combined set)
    dccsi_sys_path.append(dccsi_path)

    # we already defaulted to discovering these two early because of importance
    os.environ["DYNACONF_O3DE_DEV"] = O3DE_DEV.as_posix()

    # quick hack to shoehorn this new envar in, refactor when config is re-written
    os.environ["DYNACONF_PATH_O3DE_3RDPARTY"] = PATH_O3DE_3RDPARTY.as_posix()

    # ensure access to o3de python scripts
    os.environ["DYNACONF_PATH_O3DE_PYTHON_SCRIPTS"] = PATH_O3DE_PYTHON_SCRIPTS.as_posix()
    dccsi_pythonpath.append(PATH_O3DE_PYTHON_SCRIPTS)

    # the project is transient and optional (falls back to dccsi)
    # it's more important to set this for tools that want to work with projects
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
            _PATH_O3DE_PROJECT = O3DE_DEV

        os.environ['PATH_O3DE_PROJECT'] = str(_PATH_O3DE_PROJECT.as_posix())
        _DCCSI_LOCAL_SETTINGS['PATH_O3DE_PROJECT'] = str(_PATH_O3DE_PROJECT.as_posix())

    # we can pull the O3DE_PROJECT (name) from the project path
    if not project_name:
        project_name = Path(_PATH_O3DE_PROJECT).name

    os.environ["O3DE_PROJECT"] = str(project_name)
    _DCCSI_LOCAL_SETTINGS['O3DE_PROJECT'] = str(project_name)

    # -- O3DE build -- set up \bin\path (for Qt dll access)
    if engine_bin_path:
        _PATH_O3DE_BIN = Path(engine_bin_path)
        if _PATH_O3DE_BIN.exists():
            os.environ["DYNACONF_PATH_O3DE_BIN"] = PATH_O3DE_BIN.as_posix()
    else:
        _LOGGER.warning(f'The engine binary folder not set/found, '
                        f'method input var: engine_bin_path, '
                        f'the ENVAR is PATH_O3DE_BIN')
        _PATH_O3DE_BIN = Path('< not set >')

    # hard check (but more forgiving now)
    if _PATH_O3DE_BIN.exists():
        dccsi_sys_path.append(_PATH_O3DE_BIN)
    else:
        _LOGGER.warning(f'PATH_O3DE_BIN does NOT exist: {str(_PATH_O3DE_BIN)}')
        if DCCSI_STRICT:
            raise NotADirectoryError
    # --

    from DccScriptingInterface.azpy.constants import TAG_DIR_DCCSI_TOOLS
    _PATH_DCCSI_TOOLS = Path(dccsi_path, TAG_DIR_DCCSI_TOOLS)
    os.environ["DYNACONF_PATH_DCCSI_TOOLS"] = str(_PATH_DCCSI_TOOLS.as_posix())

    from DccScriptingInterface.azpy.constants import TAG_DCCSI_NICKNAME
    from DccScriptingInterface.azpy.constants import PATH_DCCSI_LOG_PATH
    _DCCSI_LOG_PATH = Path(PATH_DCCSI_LOG_PATH.format(PATH_O3DE_PROJECT=project_path,
                                                      TAG_DCCSI_NICKNAME=TAG_DCCSI_NICKNAME))
    os.environ["DYNACONF_DCCSI_LOG_PATH"] = str(_DCCSI_LOG_PATH.as_posix())

    from DccScriptingInterface.azpy.constants import SLUG_DIR_REGISTRY, TAG_DCCSI_CONFIG
    _PATH_DCCSI_CONFIG = Path(project_path, SLUG_DIR_REGISTRY, TAG_DCCSI_CONFIG)
    os.environ["DYNACONF_PATH_DCCSI_CONFIG"] = str(_PATH_DCCSI_CONFIG.as_posix())

    from DccScriptingInterface.azpy.constants import TAG_DIR_DCCSI_TOOLS
    _DCCSIG_TOOLS_PATH = Path.joinpath(dccsi_path, TAG_DIR_DCCSI_TOOLS)
    os.environ["DYNACONF_DCCSIG_TOOLS_PATH"] = str(_DCCSIG_TOOLS_PATH.as_posix())

    # To Do: Need to add some json validation for settings.json,
    # and settings.local.json, as importing settings will read json data
    # if the json is malformed it will cause an assert like:
    # "json.decoder.JSONDecodeError: Expecting ',' delimiter: line 45 column 5 (char 2463)"
    # but might be hard to quickly find out which settings file is bad
    from dynaconf import settings

    time_complete = timeit.default_timer() - time_start
    _LOGGER.info('~   config.init_o3de_core() DONE: {} sec'.format(time_complete))

    # we return dccsi_sys_path because we may want to pass it on
    # settings my be passes on and added to or re-initialized with changes
    return dccsi_sys_path, settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init_o3de_python(engine_path=O3DE_DEV,
                     engine_bin_path=PATH_O3DE_BIN,
                     dccsi_path=PATH_DCCSIG,
                     dccsi_sys_path=_DCCSI_SYS_PATH,
                     dccsi_pythonpath=_DCCSI_PYTHONPATH):
    """Initialize the O3DE dynamic Python env and settings.
    Note: """

    global _DCCSI_LOCAL_SETTINGS
    global _DCCSI_PYTHONPATH_EXCLUDE

    time_start = timeit.default_timer()

    # pathify
    _O3DE_DEV = Path(engine_path)
    _PATH_O3DE_BIN = Path(engine_bin_path)
    _PATH_DCCSIG = Path(dccsi_path)

    # python config
    _PATH_DCCSI_PYTHON = Path(_PATH_DCCSIG,'3rdParty','Python')
    os.environ["DYNACONF_PATH_DCCSI_PYTHON"] = str(_PATH_DCCSI_PYTHON.as_posix())

    _PATH_DCCSI_PYTHON_LIB = DccScriptingInterface.azpy.config_utils.bootstrap_dccsi_py_libs(_PATH_DCCSIG)
    os.environ["PATH_DCCSI_PYTHON_LIB"] = str(_PATH_DCCSI_PYTHON_LIB.as_posix())
    _DCCSI_LOCAL_SETTINGS['PATH_DCCSI_PYTHON_LIB']= str(_PATH_DCCSI_PYTHON_LIB.as_posix())
    #dccsi_pythonpath.append(_PATH_DCCSI_PYTHON_LIB)
    # ^ adding here will cause this to end up in settings.local.json
    # and we don't want that, since this LIB location is transient/procedural

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

    time_complete = timeit.default_timer() - time_start
    _LOGGER.info(f'~   config.init_o3de_python() ... DONE: {time_complete} sec')

    return dccsi_sys_path, dccsi_pythonpath, settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# settings.setenv()  # doing this will add the additional DYNACONF_ envars
def get_config_settings(enable_o3de_python=False,
                        enable_o3de_pyside2=False,
                        enable_o3de_pyside2_tools=False,
                        set_env=True,
                        dccsi_sys_path=_DCCSI_SYS_PATH,
                        dccsi_pythonpath=_DCCSI_PYTHONPATH):
    """Convenience method to initialize and retrieve settings directly from module."""

    # always init the core
    dccsi_sys_path, settings = init_o3de_core(dccsi_sys_path=_DCCSI_SYS_PATH,
                                              dccsi_pythonpath=_DCCSI_PYTHONPATH)

    # These should ONLY be set for O3DE and non-DCC environments
    # They will most likely cause other Qt/PySide DCC apps to fail
    # or hopefully they can be overridden for DCC environments
    if enable_o3de_python:
        dccsi_sys_path,
        dccsi_pythonpath,
        settings = init_o3de_python(dccsi_sys_path=_DCCSI_SYS_PATH,
                                    dccsi_pythonpath=_DCCSI_PYTHONPATH)

    # Many DCC apps provide their own Qt dlls and Pyside2
    # _LOGGER.info('QTFORPYTHON_PATH: {}'.format(settings.QTFORPYTHON_PATH))
    # _LOGGER.info('QT_PLUGIN_PATH: {}'.format(settings.QT_PLUGIN_PATH))
    # assume our standalone python tools wants this access?
    # it's safe to do this for dev and from ide
    if enable_o3de_pyside2:
        settings = init_o3de_pyside2(dccsi_sys_path=_DCCSI_SYS_PATH)

    # final stage, if we have managed path lists set them
    new_PATH = add_path_list_to_envar(dccsi_sys_path)
    new_PYTHONPATH = add_path_list_to_addsitedir(dccsi_pythonpath)

    # now standalone we can validate the config. env, settings.
    from dynaconf import settings
    if set_env:
        settings.setenv()

    return settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# get file name slug for exporting/caching local setting
from DccScriptingInterface.azpy.constants import DCCSI_SETTINGS_LOCAL_FILENAME

def export_settings(settings,
                    dccsi_sys_path=_DCCSI_SYS_PATH,
                    dccsi_pythonpath=_DCCSI_PYTHONPATH,
                    settings_filepath=DCCSI_SETTINGS_LOCAL_FILENAME,
                    use_dynabox=False,
                    env=None,
                    merge=False,
                    log_settings=False):

    # To Do: when running script from IDE settings.local.json are
    # correctly written to the < dccsi > root folder (script is cwd?)
    # when I run this from cli using O3DE Python:
    #     "C:\Depot\o3de-dev\python\python.cmd"
    # the setting end up in that folder:
    #     C:\Depot\o3de-dev\python\settings.local.json"
    # we need to ensure that the settings are written to the < dccsi >

    # To Do: multiple configs in different states can actually be written,
    # so provide a output path and name so a collection can be made in
    #     < dccsi >\configs\settings.core.json
    #     < dccsi >\configs\settings.python.json
    #     < dccsi >\configs\settings.pyside.json, etc.
    # dynaconf has way to load specific settings files,
    # which could be useful for tools?

    # It would be an improvement to add malformed json validation
    # this can easily cause a bad error that is deep and hard to debug

    # this just ensures that the settings.local.json file has a marker
    # we can use this marker to know the local settings exist
    os.environ['DYNACONF_DCCSI_LOCAL_SETTINGS'] = "True"

    # make sure the dynaconf synthetic env is updated before writing settings
    # this will make it into our settings file as "DCCSI_SYS_PATH":<value>
    add_path_list_to_envar(dccsi_sys_path, 'DYNACONF_DCCSI_SYS_PATH')
    add_path_list_to_envar(dccsi_pythonpath, 'DYNACONF_DCCSI_PYTHONPATH')
    # we did not use list_to_addsitedir() because we are just ensuring
    # the managed envar path list is updated so paths are written to settings

    if not use_dynabox:
        # update settings
        from dynaconf import settings
        settings.setenv()

        _settings_dict = settings.as_dict()

        # default temp filename
        _settings_file = Path(settings_filepath)

        # we want to possibly modify or stash our settings into a o3de .setreg
        from box import Box
        _settings_box = Box(_settings_dict)

        _LOGGER.info('Pretty print, _settings_box: {}'.format(_settings_file))

        if log_settings:
            _LOGGER.info(str(_settings_box.to_json(sort_keys=True,
                                                   indent=4)))

        # writes settings box
        _settings_box.to_json(filename=_settings_file.as_posix(),
                               sort_keys=True,
                               indent=4)
        return _settings_box

    # experimental, have not utilized this yet but it would be the native
    # way for dynaconf to write settings, i went with a Box dictionary
    # as it is ordered and I can ensure the settings are pretty with indenting
    elif use_dynabox:
        # writing settings using dynabox
        from dynaconf import loaders
        from dynaconf.utils.boxing import DynaBox

        _settings_box = DynaBox(settings).to_dict()

        if not env:
            # writes to a file, the format is inferred by extension
            # can be .yaml, .toml, .ini, .json, .py
            loaders.write(_settings_file.as_posix(), _settings_box)
            return _settings_box

        elif env:
            # the env can also be written, though this isn't yet utilized by config.py
            # To do: we should probably set up each DCC tool as it's own env group!
            #loaders.write(_settings_file, DynaBox(data).to_dict(), merge=False, env='development')
            loaders.write(_settings_file.as_posix(), _settings_box, merge, env)
            return _settings_box

        else:
            _LOGGER.warning('something went wrong')
            return

    else:
        _LOGGER.warning('something went wrong')
        return
# --- END -----------------------------------------------------------------

# always init defaults
settings = get_config_settings(enable_o3de_python=False,
                               enable_o3de_pyside2=False,
                               set_env=True)

###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone cli script for testing/debugging"""

    from DccScriptingInterface.azpy.env_bool import env_bool
    # temp internal debug flag, toggle values for manual testing
    DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
    DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
    DCCSI_LOGLEVEL = env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO)
    DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')

    from DccScriptingInterface.azpy.shared.utils.arg_bool import arg_bool
    # Suggestion for next iteration, args set to anything but None will
    # evaluate as True

    from DccScriptingInterface.azpy.constants import STR_CROSSBAR

    _MODULENAME = 'DCCsi.config.cli'

    # default loglevel to info unless set
    DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))
    if DCCSI_GDEBUG:
        # override loglevel if running debug
        DCCSI_LOGLEVEL = _logging.DEBUG

    # configure basic logger
    # note: not using a common logger to reduce cyclical imports
    _logging.basicConfig(level=DCCSI_LOGLEVEL,
                        format=FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')

    _LOGGER = _logging.getLogger(_MODULENAME)
    _LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
    _LOGGER.debug('site.addsitedir({})'.format(PATH_DCCSIG))
    _LOGGER.debug('_DCCSI_GDEBUG: {}'.format(DCCSI_GDEBUG))
    _LOGGER.debug('_DCCSI_DEV_MODE: {}'.format(DCCSI_DEV_MODE))
    _LOGGER.debug('_DCCSI_LOGLEVEL: {}'.format(DCCSI_LOGLEVEL))

    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info(f'~ {_MODULENAME} ... Running module as __main__')
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

    parser.add_argument('-ep', '--engine-path',
                        type=pathlib.Path,
                        required=False,
                        default=Path('{ to do: implement }'),
                        help='The path to the o3de engine.')

    parser.add_argument('-eb', '--engine-bin-path',
                        type=pathlib.Path,
                        required=False,
                        default=Path('{ to do: implement }'),
                        help='The path to the o3de engine binaries (build/bin/profile).')

    parser.add_argument('-bf', '--build-folder',
                        type=str,
                        required=False,
                        default='build',
                        help='The name (tag) of the o3de build folder, example build or windows.')

    parser.add_argument('-pp', '--project-path',
                        type=pathlib.Path,
                        required=False,
                        default=Path('{ to do: implement }'),
                        help='The path to the project.')

    parser.add_argument('-pn', '--project-name',
                        type=str,
                        required=False,
                        default='{ to do: implement }',
                        help='The name of the project.')

    parser.add_argument('-py', '--enable-python',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables O3DE python access.')

    parser.add_argument('-qt', '--enable-qt',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables O3DE Qt & PySide2 access.')

    parser.add_argument('-pc', '--project-config',
                        type=bool,
                        required=False,
                        default=False,
                        help='(not implemented) Enables reading the < >project >/registry/dccsi_configuration.setreg.')

    parser.add_argument('-cls', '--cache-local-settings',
                        type=bool,
                        required=False,
                        default=True,
                        help='Ensures setting.local.json is written to cache settings')

    parser.add_argument('-es', '--export-settings',
                        type=pathlib.Path,
                        required=False,
                        help='Writes managed settings to specified path.')

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

    parser.add_argument('-tp', '--test-pyside2',
                        type=bool,
                        required=False,
                        default=False,
                        help='Runs Qt/PySide2 tests and reports.')

    parser.add_argument('-lps', '--log-print-settings',
                        type=bool,
                        required=False,
                        default=True,
                        help='Well dump settings results into the log.')

    parser.add_argument('-ex', '--exit',
                        type=bool,
                        required=False,
                        help='Exits python. Do not exit if you want to be in interactive interpreter after config')

    args = parser.parse_args()

    from DccScriptingInterface.azpy.shared.utils.arg_bool import arg_bool

    # easy overrides
    if arg_bool(args.global_debug, desc="args.global_debug"):
        from DccScriptingInterface.azpy.constants import ENVAR_DCCSI_GDEBUG
        DCCSI_GDEBUG = True
        _LOGGER.setLevel(_logging.DEBUG)
        _LOGGER.info(f'Global debug is set, {ENVAR_DCCSI_GDEBUG}={DCCSI_GDEBUG}')

    if arg_bool(args.developer_mode, desc="args.developer_mode"):
        DCCSI_DEV_MODE = True
        # attempts to start debugger
        DccScriptingInterface.azpy.test.entry_test.connect_wing()

    if args.set_debugger:
        _LOGGER.info('Setting and switching debugger type not implemented (default=WING)')
        # To Do: implement debugger plugin pattern

    # need to do a little plumbing
    if not args.engine_path:
        args.engine_path=O3DE_DEV

    if not args.build_folder:
        from DccScriptingInterface.azpy.constants import TAG_DIR_O3DE_BUILD_FOLDER
        args.build_folder = TAG_DIR_O3DE_BUILD_FOLDER

    if not args.engine_bin_path:
        args.engine_bin_path=PATH_O3DE_BIN

    if not args.project_path:
        args.project_path=PATH_O3DE_PROJECT

    if DCCSI_GDEBUG:
        args.enable_python = True
        args.enable_qt = True

    # now standalone we can validate the config. env, settings.
    # for now hack to disable passing in other args
    # going to refactor anyway
    settings = get_config_settings(enable_o3de_python=args.enable_python,
                                   enable_o3de_pyside2=args.enable_qt)

    ## CORE
    _LOGGER.info(STR_CROSSBAR)
    # not using fstrings in this module because it might run in py2.7 (maya)
    _LOGGER.info('DCCSI_GDEBUG: {}'.format(settings.DCCSI_GDEBUG))
    _LOGGER.info('DCCSI_DEV_MODE: {}'.format(settings.DCCSI_DEV_MODE))
    _LOGGER.info('DCCSI_LOGLEVEL: {}'.format(settings.DCCSI_LOGLEVEL))

    _LOGGER.info('O3DE_DEV: {}'.format(settings.O3DE_DEV))
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
        _LOGGER.info('PATH_DCCSI_PYTHON_LIB: {}'.format(_DCCSI_LOCAL_SETTINGS['PATH_DCCSI_PYTHON_LIB']))
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
        _LOGGER.info('QT_PLUGIN_PATH: {}'.format(_DCCSI_LOCAL_SETTINGS['QT_PLUGIN_PATH']))
        _LOGGER.info('QT_QPA_PLATFORM_PLUGIN_PATH: {}'.format(_DCCSI_LOCAL_SETTINGS['QT_QPA_PLATFORM_PLUGIN_PATH']))
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

    # # if paths are in settings.local.json we need them activated
    # add_path_list_to_envar(settings.DCCSI_SYS_PATH)
    # add_path_list_to_addsitedir(settings.DCCSI_PYTHONPATH)

    if DCCSI_GDEBUG or args.cache_local_settings:
        if args.log_print_settings:
            log_settings = True
        else:
            log_settings = False

        if args.export_settings:
            export_settings_path = Path(args.export_settings).resolve()
            export_settings(settings=settings,
                            settings_filepath=export_settings_path.as_posix(),
                            log_settings=log_settings)
        else:
            export_settings(settings=settings,
                            log_settings=log_settings)
            # this should be set if there are local settings!?
            _LOGGER.debug('DCCSI_LOCAL_SETTINGS: {}'.format(settings.DCCSI_LOCAL_SETTINGS))

    # end tracking here, the pyside test exits before hitting the end of script

    _MODULE_END = timeit.default_timer() - _MODULE_START
    _LOGGER.info(f'CLI {_MODULENAME} took: {_MODULE_END} sec')

    if DCCSI_GDEBUG or args.test_pyside2:
        test_pyside2()  # test PySide2 access with a pop-up button
        try:
            import pyside2uic
        except ImportError as e:
            _LOGGER.warning("Could not import 'pyside2uic'")
            _LOGGER.warning("Refer to: '{}/3rdParty/Python/README.txt'".format(settings.PATH_DCCSIG))
            _LOGGER.error(e)

    # custom prompt
    sys.ps1 = f"[{_MODULENAME}]>>"

    if args.exit:
        # return
        sys.exit()
# --- END -----------------------------------------------------------------
