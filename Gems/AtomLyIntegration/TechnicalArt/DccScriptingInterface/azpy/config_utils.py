# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# -------------------------------------------------------------------------
"""! This module is part of the O3DE DccScriptingInterface Gem
This module is a set of utils related to config.py, it hase several methods
that can fullfil discovery of paths for use in standing up a synthetic env.
This is particularly useful when the config is used outside of O3DE,
in an external standalone tool with PySide2(Qt). For example, these paths
are discoverable so that we can synthetically derive code access to various
aspects of O3DE outside of the executables.

:notice: In a future PR this module needs to be refactored
- if can be written entirely for py3, using Pathlib, etc.

return_stub_dir()          :discover path by walking from module to file stub
get_stub_check_path()      :discover by walking from known path to file stub
get_o3de_engine_root()     :combines multiple methods to discover engine root
get_o3de_build_path()      :searches for the build path using file stub
get_dccsi_config()         :convenience method to get the dccsi config
get_current_project_cfg()  :will be depricated (don't use)
get_check_global_project() :get global project path from user .o3de data
get_o3de_project_path()    :get the project path while within editor
bootstrap_dccsi_py_libs()  :extends code access (mainly used in Maya py27)
"""
# --------------------------------------------------------------------------
# Now that we no longer need to support py2.7 this module can be improved.

import timeit
_MODULE_START = timeit.default_timer()  # start tracking

import sys
import os
import re
import site
from os.path import expanduser
import logging as _logging

# note: this module originally took into account py2.7 not having pathlib
# and other modules, we no longer intend to write code to support py2.7
# to do: modernize this modules code to py3.7+ with pathlib
# these are new imports since then, this note is left here as a reminder
# for a future PR to refactor
import pathlib
from pathlib import Path
import importlib.util
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
# Global Scope
from DccScriptingInterface.azpy import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.config_utils'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))

__all__ = ['check_is_ascii',
           'attach_debugger',
           'get_os',
           'return_stub_dir',
           'get_stub_check_path',
           'get_o3de_engine_root',
           'get_o3de_build_path',
           'get_dccsi_config',
           'get_current_project_cfg',
           'get_check_global_project',
           'get_o3de_project_path',
           'bootstrap_dccsi_py_libs']

# dccsi site/code access
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?

# note: this module is called in other root modules
# this module avoids cyclical imports by not importing azpy.constants
# to do: cleanup and maybe remove these from azpy.constants?
# CONSTANTS
ENVAR_DCCSI_GDEBUG = 'DCCSI_GDEBUG'
ENVAR_DCCSI_DEV_MODE = 'DCCSI_DEV_MODE'
ENVAR_DCCSI_LOGLEVEL = 'DCCSI_LOGLEVEL'
ENVAR_DCCSI_GDEBUGGER = 'DCCSI_GDEBUGGER'

# turn all of these off/on for local testing
_DCCSI_GDEBUG = False
_DCCSI_DEV_MODE = False
_DCCSI_LOGLEVEL = _logging.INFO
_DCCSI_GDEBUGGER = 'WING'

# Notice: The above ^ in a future refactor should probably attempt to use the following
# # this accesses common global state, e.g. DCCSI_GDEBUG (is True or False)
# from DccScriptingInterface.globals import *

FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"

# this is the DCCsi envar used for discovering the engine path (if set)
ENVAR_O3DE_DEV = 'O3DE_DEV'

# this is a known file at the root of the engine folder we can use as marker
STUB_O3DE_DEV = 'engine.json'

# this is the DCCsi envar used for discovering the project path (if set)
ENVAR_PATH_O3DE_PROJECT = 'PATH_O3DE_PROJECT'

# python related envars and paths
STR_PATH_DCCSI_PYTHON_LIB = '{0}\\3rdParty\\Python\\Lib\\{1}.x\\{1}.{2}.x\\site-packages'

PATH_USER_HOME = expanduser("~")

STR_CROSSBAR = str('{0}'.format('-' * 74))
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
# we don't have access yet to the DCCsi Lib\site-packages
# (1) this will give us import access to azpy (always?)
# we know where the dccsi root should be from here
_PATH_DCCSIG = Path(_MODULE_PATH, '../../..').resolve()
# it can be set or overrriden by dev with envar
_PATH_DCCSIG = Path(os.getenv('PATH_DCCSIG', _PATH_DCCSIG)).resolve()
# ^ we assume this config is in the root of the DCCsi
# if it's not, be sure to set envar 'PATH_DCCSIG' to ensure it
site.addsitedir(_PATH_DCCSIG.as_posix())  # must be done for azpy
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# notice: this can be removed in a future refactor, this was some early
# test inspection when the framework was being stood up.
# just a quick check to ensure what paths have code access
if _DCCSI_GDEBUG:
    known_paths = list()
    for p in sys.path:
        known_paths.append(p)
    _LOGGER.debug(known_paths)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# notice: this block can be removed in a future refactor
# this import can fail in Maya 2020 (and earlier) stuck on py2.7
# wrapped in a try, to trap and providing messaging to help user correct
# to do: deprecate this code block when Maya boostrapping is refactored
try:
    from pathlib import Path  # note: we provide this in py2.7
    # so using it here suggests some boostrapping has occured before using azpy
except Exception as e:
    _LOGGER.warning('Maya 2020 and below, use py2.7')
    _LOGGER.warning('py2.7 does not include pathlib')
    _LOGGER.warning('Try installing the O3DE DCCsi py2.7 requirements.txt')
    _LOGGER.warning("See instructions: 'C:\\< your o3de engine >\\Gems\\AtomLyIntegration\\TechnicalArt\\DccScriptingInterface\\SDK\Maya\\readme.txt'")
    _LOGGER.warning("Other code in this module with fail!!!")
    _LOGGER.error(e)
    pass  # fail gracefully, note: code accesing Path will fail!
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def check_is_ascii(value):
    """checks that passed value is ascii str"""
    try:
        value.encode('ascii')
        return True
    except (AttributeError, UnicodeEncodeError):
        return False
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# to do: move to a azpy.dev module with plugins for multiple IDEs
def attach_debugger(debugger_type=_DCCSI_GDEBUGGER):
    """! This method will attempt to attach the WING debugger

    :param debugger_type: set the debugger ide type (currently only WING)

    To Do: other IDEs for debugging not yet implemented.

    This method should be replaced with a plugin based dev package.
    """

    # notice: This is a temporary method, in a future refactor this
    # would be replaced with a azpy.dev.debugger module
    # using a pluggy pattern, to support plugins for multiple ides

    # we only support wing right now
    # the default version is Wing Pro 8 (others not tested)


    # WINGHOME defaults to Wing Pro 8.x (other versions not tested)
    # Assumes the default install path: "C:\Program Files (x86)\Wing Pro WING_VERSION_MAJOR"
    # WING_APPDATA defaults to:
    # "C:\Users\[Your User Name]\AppData\Roaming\Wing Pro WING_VERSION_MAJOR"

    # after installing wing, copy the WINGHOME\wingdbstub.py file to WING_APPDATA

    # Open the WING_APPDATA\wingdbstub.py file and modify line 96 to
    #kEmbedded = 1

    # or alternatively you can manually edit the copy in the ide install location.
    # such as: "WINGHOME": "C:\Program Files (x86)\Wing Pro 8\wingdbstub.py"

    if debugger_type == 'WING':
        from azpy.test.entry_test import connect_wing
        _debugger = connect_wing()
    else:
        _LOGGER.warning(f'Debugger type: {debugger_type}, is Not Implemented!')

    return _debugger
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# this block is related to .o3de data
# os.path.expanduser("~") returns different values in py2.7 vs 3
# Note: py27 support will be deprecated in the future
_LOGGER.debug('user home: {}'.format(PATH_USER_HOME))

# special case, make sure didn't return <user>\documents
user_home_parts = os.path.split(PATH_USER_HOME)

if str(user_home_parts[1].lower()) == 'documents':
    PATH_USER_HOME = user_home_parts[0]
    _LOGGER.debug('user home CORRECTED: {}'.format(PATH_USER_HOME))

# the global project may be defined in the registry
PATH_USER_O3DE = Path(PATH_USER_HOME, '.o3de')
PATH_USER_O3DE_REGISTRY = Path(PATH_USER_O3DE, 'Registry')
PATH_USER_O3DE_BOOTSTRAP = Path(PATH_USER_O3DE_REGISTRY, 'bootstrap.setreg')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# first define all the methods for the module
def get_os():
    """returns lumberyard dir names used in python path"""
    if sys.platform.startswith('win'):
        os_folder = "windows"
    elif sys.platform.startswith('darwin'):
        os_folder = "mac"
    elif sys.platform.startswith('linux'):
        os_folder = "linux_x64"
    else:
        message = str("DCCsi.azpy.config_utils.py: "
                      "Unexpectedly executing on operating system '{}'"
                      "".format(sys.platform))

        raise RuntimeError(message)
    return os_folder
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# from azpy.core import get_datadir
# there was a method here refactored out to add py2.7 support for Maya 2020
#"DccScriptingInterface\azpy\core\py2\utils.py get_datadir()"
#"DccScriptingInterface\azpy\core\py3\utils.py get_datadir()"
# Warning: planning to deprecate py2 support
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def return_stub_dir(stub_file='dccsi_stub'):
    '''discover and return path by walking from module to file stub
    Input: a file name (stub_file)
    Output: returns the directory of the file (stub_file)'''
    _dir_to_last_file = None
    # To Do: refactor to use pathlib object oriented Path
    if _dir_to_last_file is None:
        path = os.path.abspath(__file__)
        while 1:
            path, tail = os.path.split(path)
            if (os.path.isfile(os.path.join(path, stub_file))):
                break
            if (len(tail) == 0):
                path = ""
                _LOGGER.debug('I was not able to find the path to that file '
                              '({}) in a walk-up from currnet path'
                              ''.format(stub_file))
                break

        _dir_to_last_file = path

    return _dir_to_last_file
# --------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_stub_check_path(in_path=os.getcwd(), check_stub=STUB_O3DE_DEV):
    '''
    Returns the branch root directory of the dev\\'engine.json'
    (... or you can pass it another known stub) so we can safely build
    relative filepaths within that branch.

    Input:  a starting directory, default is os.getcwd()
    Input:  a file name stub (to search for)
    Output: a path (the stubs parent directory)

    If the stub is not found, it returns None
    '''
    path = Path(in_path).absolute()

    while 1:
        test_path = Path(path, check_stub)

        if test_path.is_file():
            return Path(test_path).parent

        else:
            path, tail = (path.parent, path.name)

            if (len(tail) == 0):
                return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_o3de_engine_root(check_stub=STUB_O3DE_DEV):
    '''Discovers the engine root
    Input:  a file name stub, default engine.json
    Output: engine root path (if found)

    Notice: This method will be deprecated
    '''
    # get the O3DE engine root folder
    # if we are running within O3DE we can ensure which engine is running
    _O3DE_DEV = None
    try:
        import azlmbr  # this file will fail outside of O3DE
    except ImportError as e:
        # if that fails, we can search up
        # search up to get \dev
        _O3DE_DEV = get_stub_check_path(check_stub=STUB_O3DE_DEV)
        # To Do: What if engine.json doesn't exist?
    else:
        # execute if no exception, allow for external ENVAR override
        _O3DE_DEV = Path(os.getenv(ENVAR_O3DE_DEV, azlmbr.paths.engroot))
    finally:
        if _DCCSI_GDEBUG: # to verbose, used often
            # note: can't use fstrings as this module gets called with py2.7 in maya
            _LOGGER.info('O3DE engine root: {}'.format(_O3DE_DEV.resolve()))
    return _O3DE_DEV
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_o3de_build_path(root_directory=get_o3de_engine_root(),
                        marker='CMakeCache.txt'):
    """Returns a path for the O3DE\build root if found. Searches down from a
    known engine root path.
    Input:  a root directory, default is to discover the engine root
    Output: the path of the build folder (if found)
    """

    root_directory = Path(root_directory)

    import timeit
    start = timeit.default_timer()

    for root, dirs, files in os.walk(root_directory.as_posix()):
        if marker in files:
            if _DCCSI_GDEBUG:
                _LOGGER.debug('Find PATH_O3DE_BUILD took: {} sec'
                              ''.format(timeit.default_timer() - start))
            return Path(root)
    else:
        if _DCCSI_GDEBUG:
            _LOGGER.debug('Not fidning PATH_O3DE_BUILD took: {} sec'
                          ''.format(timeit.default_timer() - start))
        return None

# note: if we use this method to find PATH_O3DE_BUILD
# by searching for the 'CMakeCache.txt' it can take 1 or more seconds
# this will slow down boot times!
#
# this works fine for a engine dev, but is not really suitable for end users
# it assumes that the engine is being built and 'CMakeCache.txt' exists
# but the engine could be pre-built or packaged somehow
#
# other ways to deal with it:
# 1 - Use the running application .exe to discover the build path?
# 2 - If developer set PATH_O3DE_BUILD envar in
#        "C:\Depot\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\.env"
# 3 - Set envar in commandline before executing script (or from .bat file)
# 4 - To Do (maybe): Set in a dccsi_configuration.setreg?
# -------------------------------------------------------------------------


# # -------------------------------------------------------------------------
# # DEPRECATION: this is a warning that this version will be deprecated.
# # This version was written to support py2.7 as pre-Maya2020 was stuck on that
# # We no longer intend to support py2.7
# def get_dccsi_config(dccsi_dirpath=return_stub_dir()):
    # """Convenience method to set and retreive settings directly from module."""

    # # we can go ahead and just make sure the the DCCsi env is set
    # # config is SO generic this ensures we are importing a specific one
    # _module_tag = "dccsi.config"
    # _dccsi_path = Path(dccsi_dirpath, "config.py")
    # if _dccsi_path.exists():
        # if sys.version_info.major >= 3:
            # import importlib  # note: py2.7 doesn't have importlib.util
            # import importlib.util
            # #from importlib import util
            # _spec_dccsi_config = importlib.util.spec_from_file_location(_module_tag,
                                                                        # str(_dccsi_path.as_posix()))
            # _dccsi_config = importlib.util.module_from_spec(_spec_dccsi_config)
            # _spec_dccsi_config.loader.exec_module(_dccsi_config)

            # _LOGGER.debug('Executed config: {}'.format(_spec_dccsi_config))
        # else:  # py2.x
            # import imp
            # _dccsi_config = imp.load_source(_module_tag, str(_dccsi_path.as_posix()))
            # _LOGGER.debug('Imported config: {}'.format(_spec_dccsi_config))
        # return _dccsi_config

    # else:
        # return None
# # After getting the config you can do the following.
# # settings.setenv()  # doing this will add the additional DYNACONF_ envars
# # -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_dccsi_config(PATH_DCCSIG=_PATH_DCCSIG.resolve()):
    """Convenience method to set and retreive settings directly from module."""

    try:
        Path(PATH_DCCSIG).exists()
    except FileNotFoundError as e:
        _LOGGER.debug(f"File does not exist: {PATH_DCCSIG}")
        return None

    # we can go ahead and just make sure the the DCCsi env is set
    # module name config.py is SO generic this ensures we are importing a specific one
    _spec_dccsi_config = importlib.util.spec_from_file_location("dccsi._DCCSI_CORE_CONFIG",
                                                                Path(PATH_DCCSIG,
                                                                     "config.py"))
    _dccsi_config = importlib.util.module_from_spec(_spec_dccsi_config)
    _spec_dccsi_config.loader.exec_module(_dccsi_config)

    return _dccsi_config
# After getting the config you can do the following.
# settings.setenv()  # doing this will add the additional DYNACONF_ envars
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_current_project_cfg(dev_folder=get_stub_check_path()):
    """Uses regex in lumberyard Dev\\bootstrap.cfg to retreive project tag str
    Note: boostrap.cfg will be deprecated.  Don't use this method anymore."""
    boostrap_filepath = Path(dev_folder, "bootstrap.cfg")
    if boostrap_filepath.exists():
        bootstrap = open(str(boostrap_filepath), "r")
        regex_str = r"^project_path\s*=\s*(.*)"
        game_project_regex = re.compile(regex_str)
        for line in bootstrap:
            game_folder_match = game_project_regex.match(line)
            if game_folder_match:
                _LOGGER.debug('Project is: {}'.format(game_folder_match.group(1)))
                return game_folder_match.group(1)
    return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_check_global_project():
    """Gets o3de project via .o3de data in user directory"""

    from collections import OrderedDict
    import box

    bootstrap_box = None
    json_file_path = Path(PATH_USER_O3DE_BOOTSTRAP)
    if json_file_path.exists():
        try:
            bootstrap_box = box.Box.from_json(filename=str(json_file_path.as_posix()),
                                              encoding="utf-8",
                                              errors="strict",
                                              object_pairs_hook=OrderedDict)
        except IOError as e:
            # this file runs in py2.7 for Maya 2020, FileExistsError is not defined
            _LOGGER.error('Bad file interaction: {}'.format(json_file_path.as_posix()))
            _LOGGER.error('Exception is: {}'.format(e))
            pass
    if bootstrap_box:
        # this seems fairly hard coded - what if the data changes?
        project_path=Path(bootstrap_box.Amazon.AzCore.Bootstrap.project_path)
        return project_path
    else:
        return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_o3de_project_path():
    """figures out the o3de project path if not found defaults to the engine folder"""
    _PATH_O3DE_PROJECT = None
    try:
        import azlmbr  # this file will fail outside of O3DE
    except ImportError as e:
        # (fallback 1) this checks if a global project is set
        # This check user home for .o3de data
        _PATH_O3DE_PROJECT = get_check_global_project()
    else:
        # execute if no exception, this would indicate we are in O3DE land
        # allow for external ENVAR override
        _PATH_O3DE_PROJECT = Path(os.getenv(ENVAR_PATH_O3DE_PROJECT, azlmbr.paths.projectroot))
    finally:
        # (fallback 2) if None, fallback to engine folder
        if not _PATH_O3DE_PROJECT:
            _PATH_O3DE_PROJECT = get_o3de_engine_root()

        if _DCCSI_GDEBUG: # to verbose, used often
            # note: can't use fstrings as this module gets called with py2.7 in maya
            _LOGGER.debug('O3DE project root: {}'.format(_PATH_O3DE_PROJECT.as_posix()))
    return _PATH_O3DE_PROJECT
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_dccsi_py_libs(dccsi_dirpath=return_stub_dir()):
    """Builds and adds local site dir libs based on py version"""
    _PATH_DCCSI_PYTHON_LIB = Path(STR_PATH_DCCSI_PYTHON_LIB.format(dccsi_dirpath,
                                                                   sys.version_info[0],
                                                                   sys.version_info[1]))

    if _PATH_DCCSI_PYTHON_LIB.exists():
        site.addsitedir(_PATH_DCCSI_PYTHON_LIB.as_posix())  # PYTHONPATH
        _LOGGER.debug('Performed site.addsitedir({})'
                      ''.format(_PATH_DCCSI_PYTHON_LIB.as_posix()))
        return _PATH_DCCSI_PYTHON_LIB
    else:
        message = "Doesn't exist: {}".format(_PATH_DCCSI_PYTHON_LIB)
        _LOGGER.error(message)
        raise NotADirectoryError(message)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_MODULE_END = timeit.default_timer() - _MODULE_START
_LOGGER.debug(f'Module {_MODULENAME} took: {_MODULE_END} sec')
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone cli script for testing/debugging"""

    _MODULENAME = 'DCCsi.azpy.config_utils.cli'

    # default loglevel to info unless set
    _DCCSI_LOGLEVEL = _logging.INFO
    if _DCCSI_GDEBUG:
        # override loglevel if runnign debug
        _DCCSI_LOGLEVEL = _logging.DEBUG

    # configure basic logger
    # note: not using a common logger to reduce cyclical imports
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                        format=FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')
    _LOGGER = _logging.getLogger(_MODULENAME)

    _LOGGER.info("# {0} #".format('-' * 72))
    _LOGGER.info(f'~ {_MODULENAME} ... Running module as __main__')
    _LOGGER.info("# {0} #".format('-' * 72))

    # parse the command line args
    import argparse
    parser = argparse.ArgumentParser(
        description=f'O3DE DCCsi: {_MODULENAME}',
        epilog="Coomandline args enable deeper testing and info from commandline")

    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        default=True,
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
                        help='(NOT IMPLEMENTED) Default debugger: WING, thers: PYCHARM and VSCODE.')

    parser.add_argument('-rt', '--run-tests',
                        type=bool,
                        required=False,
                        default=True,
                        help='Runs local module tests from cli (default=True).')

    parser.add_argument('-ex', '--exit',
                        type=bool,
                        required=False,
                        help='(NOT IMPLEMENTED) Exits python. Do not exit if you want to be in interactive interpreter after config')

    args = parser.parse_args()

    # easy overrides
    if args.global_debug:
        _DCCSI_GDEBUG = True
        _DCCSI_LOGLEVEL = _logging.DEBUG
        _LOGGER.setLevel(_DCCSI_LOGLEVEL)

    if args.set_debugger:
        _LOGGER.info('Setting and switching debugger type not implemented (default=WING)')
        # To Do: implement debugger plugin pattern

    if args.developer_mode or _DCCSI_DEV_MODE:
        _DCCSI_DEV_MODE = True
        attach_debugger()  # attempts to start debugger

    # built in simple tests and info from commandline
    _LOGGER.info('Current Work dir: {0}'.format(os.getcwd()))

    if args.run_tests:
        _LOGGER.info(f'Running local module tests ...')

        _LOGGER.info('Active OS: {}'.format(get_os()))

        # attempts to retreiv the global o3de project
        _PATH_O3DE_PROJECT = Path(get_check_global_project()).resolve()
        _LOGGER.info(f'PATH_O3DE_PROJECT: {_PATH_O3DE_PROJECT.as_posix()}')

        # used a stub file name to walk to a location
        _PATH_DCCSIG = Path(return_stub_dir('dccsi_stub')).resolve()
        _LOGGER.info(f'PATH_DCCSIG: {_PATH_DCCSIG.as_posix()}')

        # retreives the core dccsi config module
        _DCCSI_CONFIG = get_dccsi_config(_PATH_DCCSIG)
        _LOGGER.info(f'PATH_DCCSI_CONFIG: {_DCCSI_CONFIG}')

        # bootstraps site lib access to dccsi installed package dependancies
        _PATH_DCCSI_PYTHON_LIB = Path(bootstrap_dccsi_py_libs(_PATH_DCCSIG)).resolve()
        _LOGGER.info(f'PATH_DCCSI_PYTHON_LIB: {_PATH_DCCSI_PYTHON_LIB.resolve()}')

        # uses a known file to walk and discover the engine root
        _O3DE_DEV = Path(get_o3de_engine_root(check_stub='engine.json')).resolve()
        _LOGGER.info(f'O3DE_DEV: {_O3DE_DEV.as_posix()}')

        # uses a cmake file to try and find the build directory
        # to do: this method is not great, it doesn't work with installer
        # need to find a better approach
        # possibly the o3de.py package to retreive from manifest?
        _PATH_O3DE_BUILD = Path(get_o3de_build_path(_O3DE_DEV,
                                                    'CMakeCache.txt')).resolve()
        _LOGGER.info('PATH_O3DE_BUILD: {}'.format(_PATH_O3DE_BUILD.as_posix()))

    _MODULE_END = timeit.default_timer() - _MODULE_START
    _LOGGER.info(f'CLI {_MODULENAME} took: {_MODULE_END} sec')

    # custom prompt
    sys.ps1 = f"[{_MODULENAME}]>>"

    if args.exit:
        # return
        sys.exit()
# --- END -----------------------------------------------------------------
