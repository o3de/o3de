# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------
from __future__ import unicode_literals

"""
Module Description:
    DccScriptingInterface\\azpy\\shared\\synthetic_env.py

Stands up a synthetic version of the environment in:
    DccScriptingInterface\\Launchers\\windows\\Env.bat

DccScriptingInterface a.k.a. DCCsi

This module (softly) assumes that the cwd is the DCCsi
AND that you can get access to imports from azpy

The configures a synthetic version of the default DCCsi env,
which includes additional code access paths and some other settings.

It does not assume that the external environment is configured.
It provides best guess defaults for the environment (when they are not set.)
It will not overwrite the envars that are configured.

To do this, it uses the pattern:
    _SOME_ENVAR = os.getenv('ENVAR_TAG', <default>)

The environment is conveniently packed into a dictionary,
this allows the dictionary to be imported into another module,
the import will resolve and standup the synthetic environment.

Configures several useful environment config settings and paths,
    [key]               : [value]

    # this is the required base environment
    O3DE_PROJECT          : name of project (project directory)
    O3DE_DEV              : path to Lumberyard \dev root
    PATH_O3DE_PROJECT     : path to project dir
    PATH_DCCSIG         : path to the DCCsi Gem root
    PATH_DCCSI_TOOLS      : path to associated (non-api code) DCC SDK

    # nice to haves in base env to define core support
    DCCSI_GDEBUG        : sets global debug prints
    DCCSI_DEV_MODE      : extends support like early debugger attachment
    DCCSI_GDEBUGGER     : default debugger (WING)

    DCCsi has been (experimentally) tested with : PY27, PY37
    - py27 for DCC tools like Maya
    - py37 for Lumberyard
    - it probably will work with 3.6 (substance)

    :: Default version py37 has a launcher
    (activates the env, starts py interpreter)
    set DCCSI_PY_BASE=%PATH_O3DE_PYTHON_INSTALL%\python.exe

    :: shared location for 64bit python 3.10 BASE location
    set DCCSI_PY_DCCSI=%DCCSI_LAUNCHERS_PATH%\Launch_pyBASE.bat

    :: Override DCCSI_PY_DCCSI to set a default version for the DCCsi

    :: ide and debugger plugs
    :: for instance with WingIDE this defines using the default py interpreter
    ${DCCSI_PY_DEFAULT}

    ::Other python runtimes can be defined, for py27 we use this
    :: shared location for 64bit DCCSI_PY_MAYA python 2.7 DEV location
    set DCCSI_PY_MAYA=%MAYA_LOCATION%\bin\mayapy.exe
    :: wingIDE can use more then one defined/managed interpreters
    :: allowing you to _DCCSI_GDEBUG code in multiple runtimes in one session
    ${DCCSI_PY_MAYA}

    # related to the WING as the default DCCSI_GDEBUGGER
    DCCSI_WING_VERSION_MAJOR            : the major version of Wing IDE (default IDE)
    DCCSI_WING_VERSION_MINOR            : minor version #
    WINGHOME                      : path for wing (for debug) integration

    NOTES:
    The default IDE and debugger is WING primarily because of author preference.
    Other options are available to configure out-of-the-box (PyCharm)
    """
# -------------------------------------------------------------------------
# built in's
import os
import sys
import site
import re
import inspect
import json
import importlib.util

import logging as _logging
from collections import OrderedDict

# Lumberyard and Atom standalone should be launched in py3.7+
# because of use of pathlib (unless externally provided in py2.7)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
os.environ['PYTHONINSPECT'] = 'True'

_MODULE_PATH = os.path.realpath(__file__)  # To Do: what if frozen?
_PATH_DCCSIG = os.path.normpath(os.path.join(_MODULE_PATH, '../..'))
_PATH_DCCSIG = os.getenv('PATH_DCCSIG', _PATH_DCCSIG)
site.addsitedir(_PATH_DCCSIG)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# O3DE extensions
from pathlib import Path

# set up global space, logging etc.
import azpy.env_bool as env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import FRMT_LOG_LONG

_DCCSI_GDEBUG = env_bool.env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool.env_bool(ENVAR_DCCSI_DEV_MODE, False)

_PACKAGENAME = 'DCCsi.azpy.synthetic_env'

# set up module logging
for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)
_LOGGER = _logging.getLogger(_PACKAGENAME)
_logging.basicConfig(format=FRMT_LOG_LONG)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

_LOGGER.debug('_PATH_DCCSIG: {}'.format(_PATH_DCCSIG))
_LOGGER.debug('_DCCSI_GDEBUG: {}'.format(_DCCSI_GDEBUG))
_LOGGER.debug('_DCCSI_DEV_MODE: {}'.format(_DCCSI_DEV_MODE))

if _DCCSI_DEV_MODE:
    from azpy.test.entry_test import connect_wing
    foo = connect_wing()

# we can go ahead and just make sure the the DCCsi env is set
# config is SO generic this ensures we are importing a specific one
_spec_dccsi_config = importlib.util.spec_from_file_location("dccsi.config",
                                                            Path(_PATH_DCCSIG,
                                                                 "config.py"))
_dccsi_config = importlib.util.module_from_spec(_spec_dccsi_config)
_spec_dccsi_config.loader.exec_module(_dccsi_config)

settings = _dccsi_config.get_config_settings()
# -------------------------------------------------------------------------

# Lumberyard extensions
from azpy.constants import *
from azpy.shared.common.core_utils import walk_up_dir
from azpy.shared.common.core_utils import get_stub_check_path

_PATH_DCCSI_PYTHON_LIB = os.getenv(ENVAR_PATH_DCCSI_PYTHON_LIB,
                                   PATH_DCCSI_PYTHON_LIB)
_LOGGER.debug('Dccsi Lib Path: {0}'.format(_PATH_DCCSI_PYTHON_LIB))

if os.path.exists(_PATH_DCCSI_PYTHON_LIB):
    site.addsitedir(_PATH_DCCSI_PYTHON_LIB)  # add access

# -------------------------------------------------------------------------
#  post-bootstrap global space
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
# -------------------------------------------------------------------------


FRMT_LOG_LONG = ("[%(name)s][%(levelname)s] >> "
                 "%(message)s (%(asctime)s; %(filename)s:%(lineno)d)")
_PACKAGENAME = 'azpy.synthetic_env'

_logging.basicConfig(level=_logging.INFO,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')
_LOGGER = _logging.getLogger(_PACKAGENAME)

_log_level = int(10)
console_handler = _logging.StreamHandler(sys.stdout)
console_handler.setLevel(_log_level)
formatter = _logging.Formatter(FRMT_LOG_LONG)
console_handler.setFormatter(formatter)
_LOGGER.addHandler(console_handler)
_LOGGER.setLevel(_log_level)

_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

# -------------------------------------------------------------------------
# This module is semi-standalone (no azpy YET) to avoid circular imports
# To Do: figure out how to bets NOPT dup this methods (there are also moudles)
# we want to run this potentially with no preformed env
# and thus we may not have full acess to the DCCsi and azpy package
# -------------------------------------------------------------------------
def get_current_project(dev_folder):
    boostrap_filepath = Path(dev_folder, "bootstrap.cfg")
    bootstrap = open(boostrap_filepath, "r")
    game_project_regex = re.compile("^sys_game_folder\s*=\s*(.*)")
    for line in bootstrap:
        game_folder_match = game_project_regex.match(line)
        if game_folder_match:
            return game_folder_match.group(1)
    return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# TO DO: Move to a util package or module
def return_stub(stub='dccsi_stub'):
    '''Take a file name (stub) and returns the directory of the file (stub)'''
    _dir_to_last_file = None
    if _dir_to_last_file is None:
        path = Path(__file__).absolute()
        while 1:
            path, tail = Path(path).split()
            if Path(path, stub).is_file():
                break
            if (len(tail) == 0):
                path = ""
                if _DCCSI_GDEBUG:
                    _LOGGER.debug('~Not able to find the path to that file '
                                  '(stub) in a walk-up from currnet path.')
                break
        _dir_to_last_file = path

    return _dir_to_last_file
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
def get_stub_check_path(in_path, check_stub='engineroot.txt'):
    '''
    Returns the branch root directory of the dev\'engineroot.txt'
    (... or you can pass it another known stub)

    so we can safely build relative filepaths within that branch.

    If the stub is not found, it returns None
    '''
    path = Path(in_path).absolute()

    while 1:
        test_path = Path(path, check_stub)

        if test_path.is_file():
            return Path(test_path)

        else:
            path, tail = (path.parent, path.name)

            if (len(tail) == 0):
                return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# TO DO: Move to a util package or module
def resolve_envar_path(envar='O3DE_DEV',
                       start_path=__file__,
                       check_stub='engineroot.txt',
                       dir_name='dev',
                       return_posix_str=False):
    """This is meant to resolve a '\dev' path for LY

    First we validate the start_path (or we can't walk)

    Then we walk up the startPath looking for stub
    engineroot.txt is the default file marker for lumberyard

    That is a pretty safe indicator that we found the right '\dev'

    Second it checks if the env var 'O3DE_DEV' is set, use that instead!

    """

    fallback = None

    # amke sure the start_path exists, otherwise we have nothing to walk up
    try:
        start_path = Path(start_path)
        start_path.exists()
        if start_path.is_dir():
            fallback = start_path
        elif start_path.is_file():
            fallback = start_path.parent
    except Exception as e:
        _LOGGER.info('Does NOT exist, no valid path: {0}'.format(start_path))
        raise EnvironmentError

    # generate a fallback based on finding know stub
    # known stub is probably more reliable then dir name

    fallback_stub = get_stub_check_path(fallback, check_stub)
    #  there is alos a chance this comes back None

    # cast from is_file to directory
    if fallback_stub and fallback_stub.is_file():
        fallback = fallback_stub.parent

    # check if it's set in env and fetch, or set to fallback
    envar_path = Path(os.getenv(envar, fallback))
    try:
        envar_path.exists()
        envar_path.is_dir()
        envar_path.name.lower == dir_name
    except Exception as e:
        _LOGGER.info('Not a lumbertard \dev path: {0}'.format(e))
        raise EnvironmentError

    # last resort see if you can walk up to the dir name
    if not envar_path:
        envar_path = walk_up_dir(start_path, dir_name)

    if envar_path and Path(envar_path).exists():
        # box module can't serialize WindowsPath objects
        # so Path object being stashed in this dict need .resolve() or .as_posix()
        if return_posix_str:
            return Path(envar_path).as_posix()
        else:
            return Path(envar_path)
    else:
        return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# setting up storage dict, only using a Box (super dict) instead
_SYNTH_ENV_DICT = OrderedDict()
# -------------------------------------------------------------------------


# --- Build Defaults ------------------------------------------------------
def stash_env(_SYNTH_ENV_DICT = OrderedDict()):
    """This block attempts to fetch or derive best guess fallback"""

    # basic default fall backs (in case no environment exists)
    # this first set is just the expected base_env

    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('~ stash_env(), setting module fallbacks')

    # box module can't serialize WindowsPath objects
    # so Path object being stashed in this dict need .resolve() or .as_posix()
    _THIS_MODULE_PATH = Path(__file__).as_posix()

    # company name from env or default
    # TAG_DEFAULT_COMPANY = str('Amazon.Lumberyard')
    _G_COMPANY_NAME = os.getenv(ENVAR_COMPANY,
                                TAG_DEFAULT_COMPANY)

    # we want to pack these all into a easy access dict
    _SYNTH_ENV_DICT[ENVAR_COMPANY] = _G_COMPANY_NAME

    # <ly>\dev Lumberyard ROOT PATH
    # someone decided to use this as a root stub (for similar reasons in C++?)
    # STUB_O3DE_DEV = str('engineroot.txt')
    #  I don't own \dev so I didn't want to check in anything new there
    _O3DE_DEV = resolve_envar_path(ENVAR_O3DE_DEV,  # envar
                                 _THIS_MODULE_PATH,  # search path
                                 STUB_O3DE_DEV,  # stub
                                 TAG_DIR_O3DE_DEV)  # dir

    _SYNTH_ENV_DICT[ENVAR_O3DE_DEV] = _O3DE_DEV.as_posix()

    # project name is a string, it should be project dir name
    # for siloed testing and a purely synthetc env (nothing previously set)
    # the default should probably be the 'DccScriptingInterface' (DCCsi)
    # we also have a 'MockProject' sanndbox/silo, but we want to reseve that
    # for testing overrides of the default synthetic env

    # we can do two things here,
    # first we can try to fetch from the env os.getenv('O3DE_PROJECT')
    # If comes back None, allows you to specify a default fallback
    # changed to just make the fallback what is set in boostrap
    # so now it's less of a fallnack and more correct if not
    # explicitly set
    _O3DE_PROJECT = os.getenv(ENVAR_O3DE_PROJECT)
    _SYNTH_ENV_DICT[ENVAR_O3DE_PROJECT] = _O3DE_PROJECT

    _O3DE_BUILD_DIR_NAME = os.getenv(ENVAR_O3DE_BUILD_DIR_NAME,
                                   TAG_DIR_O3DE_BUILD_FOLDER)
    _SYNTH_ENV_DICT[ENVAR_O3DE_BUILD_DIR_NAME] = _O3DE_BUILD_DIR_NAME

    #  pattern for the above is (and will be repeated)
    #  _SOME_ENVAR = resolve_envar_path('ENVAR',
    #                                   'path\\to\\start\\search',
    #                                   'knownStubFileName',
    #                                   'knownDirName')
    # checks env for ENVAR first,
    # then walks up search path looking for stub (fallback 0),
    # then walks up niavely looking for a dirName (fallback 1)

    # other paths that don't have a stub to search for as a fallback
    # can be stashed in this manner

    # variable containers for all of the default/base env config dccsi entry
    # setting most basic defaults, not using an module import dependancies
    # we can't know where the user installed or set up any of these,
    # so we guess based on how I set up the original dev environment

    # -- envar --
    _PATH_O3DE_BUILD = Path(os.getenv(ENVAR_PATH_O3DE_BUILD,
                                    PATH_O3DE_BUILD))
    _SYNTH_ENV_DICT[ENVAR_PATH_O3DE_BUILD] = _PATH_O3DE_BUILD.as_posix()

    # -- envar --
    _PATH_O3DE_BIN = Path(os.getenv(ENVAR_PATH_O3DE_BIN,
                                  PATH_O3DE_BIN))
    # some of these need hard checks
    if not _PATH_O3DE_BIN.exists():
        raise Exception('PATH_O3DE_BIN does NOT exist: {0}'.format(_PATH_O3DE_BIN))
    else:
        _SYNTH_ENV_DICT[ENVAR_PATH_O3DE_BIN] = _PATH_O3DE_BIN.as_posix()
        # adding to sys.path apparently doesn't work for .dll locations like Qt
        os.environ['PATH'] = _PATH_O3DE_BIN.as_posix() + os.pathsep + os.environ['PATH']

    # -- envar --
    # if that stub marker doesn't exist assume DCCsi path (fallback 1)
    _PATH_O3DE_PROJECT = Path(os.getenv(ENVAR_PATH_O3DE_PROJECT,
                                      Path(_O3DE_DEV, _O3DE_PROJECT)))
    _SYNTH_ENV_DICT[ENVAR_PATH_O3DE_PROJECT] = _PATH_O3DE_PROJECT.as_posix()

    # -- envar --
    _PATH_DCCSIG = resolve_envar_path(ENVAR_PATH_DCCSIG,  # envar
                                           _THIS_MODULE_PATH,  # search path
                                           STUB_O3DE_ROOT_DCCSI,  # stub name
                                         TAG_DEFAULT_PROJECT)  # dir
    _SYNTH_ENV_DICT[ENVAR_PATH_DCCSIG] = _PATH_DCCSIG.as_posix()

    # -- envar --
    _AZPY_PATH = Path(os.getenv(ENVAR_DCCSI_AZPY_PATH,
                                Path(_PATH_DCCSIG, TAG_DIR_DCCSI_AZPY)))
    _SYNTH_ENV_DICT[ENVAR_DCCSI_AZPY_PATH] = _AZPY_PATH.as_posix()

    # -- envar --
    _PATH_DCCSI_TOOLS = Path(os.getenv(ENVAR_PATH_DCCSI_TOOLS,
                                     Path(_PATH_DCCSIG, TAG_DIR_DCCSI_TOOLS)))
    _SYNTH_ENV_DICT[ENVAR_PATH_DCCSI_TOOLS] = _PATH_DCCSI_TOOLS.as_posix()

    # -- envar --
    # external dccsi site-packages
    _PATH_DCCSI_PYTHON_LIB = Path(os.getenv(ENVAR_PATH_DCCSI_PYTHON_LIB,
                                            PATH_DCCSI_PYTHON_LIB))
    _SYNTH_ENV_DICT[ENVAR_PATH_DCCSI_PYTHON_LIB] = _PATH_DCCSI_PYTHON_LIB.as_posix()

    # -- envar --
    # extend to py36 (conda env) and interpreter (wrapped as a .bat file)
    _DEFAULT_PY_PATH = Path(_PATH_DCCSIG, TAG_DEFAULT_PY)
    _DEFAULT_PY_PATH = Path(os.getenv(ENVAR_DCCSI_PY_DEFAULT,
                                      _DEFAULT_PY_PATH))
    _SYNTH_ENV_DICT[ENVAR_DCCSI_PY_DEFAULT] = _DEFAULT_PY_PATH.as_posix()

    # -- envar --
    # wing ide vars
    _WINGHOME_DEFAULT_PATH = Path(os.getenv(ENVAR_WINGHOME,
                                            PATH_DEFAULT_WINGHOME))
    _SYNTH_ENV_DICT[ENVAR_WINGHOME] = _WINGHOME_DEFAULT_PATH.as_posix()

    # -- done --
    return _SYNTH_ENV_DICT
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init_ly_pyside(env_dict=_SYNTH_ENV_DICT):
    """sets access to lumberyards Qt dlls and PySide"""

    # -- envar --
    _QTFORPYTHON_PATH = Path(os.getenv(ENVAR_QTFORPYTHON_PATH,
                                       PATH_QTFORPYTHON_PATH))
    # some of these need hard checks
    if not _QTFORPYTHON_PATH.exists():
        raise Exception('QTFORPYTHON_PATH does NOT exist: {0}'.format(_QTFORPYTHON_PATH))
    else:
        _SYNTH_ENV_DICT[ENVAR_QTFORPYTHON_PATH] = _QTFORPYTHON_PATH.as_posix()
        # ^ some of these should be put on sys.path and/or PYTHONPATH or PATH
        #os.environ['PATH'] = _QTFORPYTHON_PATH.as_posix() + os.pathsep + os.environ['PATH']
        site.addsitedir(_QTFORPYTHON_PATH.as_posix())  # PYTHONPATH

    # -- envar --
    _QT_PLUGIN_PATH = Path(os.getenv(ENVAR_QT_PLUGIN_PATH,
                                     PATH_QT_PLUGIN_PATH))
    # some of these need hard checks
    if not _QT_PLUGIN_PATH.exists():
        raise Exception('QT_PLUGIN_PATH does NOT exist: {0}'.format(_QT_PLUGIN_PATH))
    else:
        _SYNTH_ENV_DICT[ENVAR_QT_PLUGIN_PATH] = _QT_PLUGIN_PATH.as_posix()
        # https://stackoverflow.com/questions/214852/python-module-dlls
        os.environ['PATH'] = _QT_PLUGIN_PATH.as_posix() + os.pathsep + os.environ['PATH']



    QTFORPYTHON_PATH = Path.joinpath(O3DE_DEV,
                                     'Gems',
                                     'QtForPython',
                                     '3rdParty',
                                     'pyside2',
                                     'windows',
                                     'release').resolve()
    os.environ["DYNACONF_QTFORPYTHON_PATH"] = str(QTFORPYTHON_PATH)
    os.environ["QTFORPYTHON_PATH"] = str(QTFORPYTHON_PATH)
    sys.path.insert(1, str(QTFORPYTHON_PATH))
    site.addsitedir(str(QTFORPYTHON_PATH))

    PATH_O3DE_BIN = Path.joinpath(O3DE_DEV,
                                'windows',
                                'bin',
                                'profile').resolve()
    os.environ["DYNACONF_PATH_O3DE_BIN"] = str(PATH_O3DE_BIN)
    os.environ["PATH_O3DE_BIN"] = str(PATH_O3DE_BIN)
    site.addsitedir(str(PATH_O3DE_BIN))
    sys.path.insert(1, str(PATH_O3DE_BIN))

    QT_PLUGIN_PATH = Path.joinpath(PATH_O3DE_BIN,
                                   'EditorPlugins').resolve()
    os.environ["DYNACONF_QT_PLUGIN_PATH"] = str(QT_PLUGIN_PATH)
    os.environ["QT_PLUGIN_PATH"] = str(QT_PLUGIN_PATH)
    site.addsitedir(str(QT_PLUGIN_PATH))
    sys.path.insert(1, str(QT_PLUGIN_PATH))

    QT_QPA_PLATFORM_PLUGIN_PATH = Path.joinpath(QT_PLUGIN_PATH,
                                                'platforms').resolve()
    os.environ["DYNACONF_QT_QPA_PLATFORM_PLUGIN_PATH"] = str(QT_QPA_PLATFORM_PLUGIN_PATH)
    os.environ["QT_QPA_PLATFORM_PLUGIN_PATH"] = str(QT_QPA_PLATFORM_PLUGIN_PATH)
    site.addsitedir(str(QT_QPA_PLATFORM_PLUGIN_PATH))
    sys.path.insert(1, str(QT_QPA_PLATFORM_PLUGIN_PATH))

    # add Qt binaries to the Windows path to handle findings DLL file dependencies
    if sys.platform.startswith('win'):
        path = os.environ['PATH']
        newPath = ''
        newPath += str(PATH_O3DE_BIN) + os.pathsep
        newPath += str(Path.joinpath(QTFORPYTHON_PATH,
                                     'shiboken2').resolve()) + os.pathsep
        newPath += str(Path.joinpath(QTFORPYTHON_PATH,
                                     'PySide2').resolve()) + os.pathsep
        newPath += path
        os.environ['PATH']=newPath
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

    return status
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# py 2 and 3 compatible iter
def get_items(dict_object):
    for key in dict_object:
        yield key, dict_object[key]

def set_env(dict_object):
    for key, value in get_items(dict_object):
        try:
            os.environ[key] = value
        except EnvironmentError as e:
            _LOGGER.error('ERROR: {e}')
    return dict_object

# will trigger on any import
# suggested use: from synthetic_env import _SYNTH_ENV_DICT
#_SYNTH_ENV_DICT = set_env(_SYNTH_ENV_DICT)

# -------------------------------------------------------------------------
def test_Qt():
    try:
        import PySide2
        _LOGGER.info('PySide2: {0}'.format(Path(PySide2.__file__).as_posix()))
        # builtins.ImportError: DLL load failed: The specified procedure could not be found.
        from PySide2 import QtCore
        from PySide2 import QtWidgets
    except IOError as e:
        _LOGGER.error('ERROR: {0}'.format(e))
        raise e

    try:
        qapp = QtWidgets.QApplication([])
    except:
        # already exists
        qapp = QtWidgets.QApplication.instance()

    try:
        buttonFlags = QtWidgets.QMessageBox.information(QtWidgets.QApplication.activeWindow(), 'title', 'ok')
        qapp.instance().quit
        qapp.exit()
    except Exception as e:
        _LOGGER.error('ERROR: {0}'.format(e))
        raise e
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def main(argv, env_dict_object, debug=False, devmode=False):
    import getopt
    try:
        opts, args = getopt.getopt(argv, "hvt:", ["verbose=", "test="])
    except getopt.GetoptError as e:
        # not logging, print to cmd line console
        _LOGGER.error('synthetic_env.py -v <print_dict> -t <run_test>')
        sys.exit(2)

    for opt, arg in opts:
        if opt == '-h':
            _LOGGER.info('synthetic_env.py -v <print_dict> -t <run test>')
            sys.exit()

        elif opt in ("-t", "--test"):
            debug = True
            devmode = True
            test()

        elif opt in ("-v", "--verbose"):
            try:
                from box import Box
            except ImportError as e:
                _LOGGER.error('ERROR: {0}'.format(e))
                raise e
            try:
                env_dict_object = Box(env_dict_object)
                _LOGGER.info(str(env_dict_object.to_json(sort_keys=False,
                                              indent=4)))
            except Exception as e:
                _LOGGER.error('ERROR: {0}'.format(e))
                raise e
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    # run simple tests?
    _DCCSI_GDEBUG = True
    _DCCSI_DEV_MODE = True

    if _DCCSI_DEV_MODE:
        try:
            import azpy.test.entry_test
            _LOGGER.info('SUCCESS: import azpy.test.entry_test')
            azpy.test.entry_test.main(verbose=True, connect_debugger=True)
        except ImportError as e:
            _LOGGER.error('ERROR: {0}'.format(e))
            raise e

    # init, stash and then activate
    _SYNTH_ENV_DICT = OrderedDict()
    _SYNTH_ENV_DICT = stash_env(_SYNTH_ENV_DICT)
    _SYNTH_ENV_DICT = set_env(_SYNTH_ENV_DICT)

    main(sys.argv[1:], _SYNTH_ENV_DICT, _DCCSI_GDEBUG, _DCCSI_DEV_MODE)

    if _DCCSI_GDEBUG:

        tempBoxJsonFilePath = Path(_SYNTH_ENV_DICT['PATH_DCCSIG'], '.temp')
        tempBoxJsonFilePath = Path(tempBoxJsonFilePath, 'boxDumpTest.json')
        _LOGGER.info(f'tempBoxJsonFilePath: {tempBoxJsonFilePath}')

        try:
            tempBoxJsonFilePath.mkdir(parents=True, exist_ok=True)
            tempBoxJsonFilePath.touch(mode=0o777, exist_ok=True)
        except Exception as e:
            _LOGGER.info(e)

        _LOGGER.info('~ writting with Box.to_json')
        try:
            from box import Box
            _SYNTH_ENV_DICT = Box(_SYNTH_ENV_DICT)
            _LOGGER.info(type(_SYNTH_ENV_DICT))
        except Exception as e:
            _LOGGER.info(e)

        # -- BOX STORE ------
        _LOGGER.info(str(_SYNTH_ENV_DICT.to_json(sort_keys=False,
                                                 indent=4)))
        try:
            #_SYNTH_ENV_DICT.to_json(filename=None, encoding='utf-8', errors='strict')
            #from os import fspath
            #tempBoxJsonFilePath = fspath(tempBoxJsonFilePath)
            _SYNTH_ENV_DICT.to_json(filename=tempBoxJsonFilePath.as_posix(),
                                    sort_keys=False,
                                    indent=4)
            _LOGGER.info('~ Box.to_json SUCCESS')
        except Exception as e:
            _LOGGER.info(e)
            # if this raises an exception related to WindowsPath
            # that a path obj was stashed, use .as_posix() when stashing
            raise e

        _LOGGER.info('listing envar keys: {0}'.format(_SYNTH_ENV_DICT.keys()))

        # -- BOX READ ------
        _LOGGER.info('~ read Box.from_json')

        parseJsonBox = Box.from_json(filename=tempBoxJsonFilePath,
                                    encoding="utf-8",
                                     errors="strict",
                                     object_pairs_hook=OrderedDict)

        _LOGGER.info('~ pretty print parsed Box.from_json')

        _LOGGER.info(json.dumps(parseJsonBox, indent=4, sort_keys=False, ensure_ascii=False))

        # also run the Qt/PySide2 test
        test_Qt()

    del _LOGGER
