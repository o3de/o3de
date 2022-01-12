# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# note: this module should reamin py2.7 compatible (Maya) so no f'strings
# -------------------------------------------------------------------------
"""@module docstring
This module is part of the O3DE DccScriptingInterface Gem
This module is a set of utils related to config.py, it hase several methods
that can fullfil discovery of paths for use in standing up a synthetic env.
This is particularly useful when the config is used outside of O3DE,
in an external standalone tool with PySide2(Qt).  Foe example, these paths
are discoverable so that we can synthetically derive code access to various
aspects of O3DE outside of the executables.

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
import time
start = time.process_time() # start tracking

import sys
import os
import re
import site
import logging as _logging
# -------------------------------------------------------------------------


# --------------------------------------------------------------------------
# global scope
_MODULENAME = 'azpy.config_utils'
    
__all__ = ['get_os',
           'return_stub',
           'get_stub_check_path',
           'get_dccsi_config',
           'get_current_project']

# dccsi site/code access
#os.environ['PYTHONINSPECT'] = 'True'
_MODULE_PATH = os.path.abspath(__file__)

# we don't have access yet to the DCCsi Lib\site-packages
# (1) this will give us import access to azpy (always?)
# we know where the dccsi root should be from here
_PATH_DCCSIG = os.path.abspath(os.path.dirname(os.path.dirname(_MODULE_PATH)))
# it can be set or overrriden by dev with envar
_PATH_DCCSIG = os.getenv('PATH_DCCSIG', _PATH_DCCSIG)
# ^ we assume this config is in the root of the DCCsi
# if it's not, be sure to set envar 'PATH_DCCSIG' to ensure it
site.addsitedir(_PATH_DCCSIG)  # must be done for azpy
    
# note: this module is called in other root modules
# must avoid cyclical imports, no imports from azpy.constants
ENVAR_DCCSI_GDEBUG = 'DCCSI_GDEBUG'
ENVAR_DCCSI_DEV_MODE = 'DCCSI_DEV_MODE'
ENVAR_DCCSI_GDEBUGGER = 'DCCSI_GDEBUGGER'
ENVAR_DCCSI_LOGLEVEL = 'DCCSI_LOGLEVEL'
FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"

from azpy.env_bool import env_bool
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
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# just a quick check to ensure what paths have code access
if _DCCSI_GDEBUG:
    known_paths = list()
    for p in sys.path:
        known_paths.append(p)
    _LOGGER.debug(known_paths)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# this import can fail in Maya 2020 (and earlier) stuck on py2.7
# wrapped in a try, to trap and providing messaging to help user correct
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
# exapnd the global scope and module CONST

# this is the DCCsi envar used for discovering the engine path (if set)
ENVAR_O3DE_DEV = 'O3DE_DEV'
STUB_O3DE_DEV = 'engine.json'

# this block is related to .o3de data
# os.path.expanduser("~") returns different values in py2.7 vs 3
# Note: py27 support will be deprecated in the future
from os.path import expanduser
PATH_USER_HOME = expanduser("~")
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

# this is the DCCsi envar used for discovering the project path (if set)
ENVAR_PATH_O3DE_PROJECT = 'PATH_O3DE_PROJECT'

# python related envars and paths
STR_PATH_DCCSI_PYTHON_LIB = '{0}\\3rdParty\\Python\\Lib\\{1}.x\\{1}.{2}.x\\site-packages'
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
    """Returns a path for the O3DE\build root if found. Searchs down from a 
    known engine root path.
    Input:  a root directory, default is to discover the engine root
    Output: the path of the build folder (if found)
    """
    
    if _DCCSI_GDEBUG:
        import time
        start = time.process_time()        
    
    for root, dirs, files in os.walk(root_directory):
        if marker in files:
            if _DCCSI_GDEBUG:
                _LOGGER.debug('Find PATH_O3DE_BUILD took: {} sec'
                              ''.format(time.process_time() - start))            
            return Path(root)
    else:
        if _DCCSI_GDEBUG:
            _LOGGER.debug('Not fidning PATH_O3DE_BUILD took: {} sec'
                          ''.format(time.process_time() - start))
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


# -------------------------------------------------------------------------
# settings.setenv()  # doing this will add the additional DYNACONF_ envars
def get_dccsi_config(dccsi_dirpath=return_stub_dir()):
    """Convenience method to set and retreive settings directly from module."""

    # we can go ahead and just make sure the the DCCsi env is set
    # config is SO generic this ensures we are importing a specific one
    _module_tag = "dccsi.config"
    _dccsi_path = Path(dccsi_dirpath, "config.py")
    if _dccsi_path.exists():
        if sys.version_info.major >= 3:
            import importlib  # note: py2.7 doesn't have importlib.util
            import importlib.util
            #from importlib import util
            _spec_dccsi_config = importlib.util.spec_from_file_location(_module_tag,
                                                                        str(_dccsi_path.resolve()))
            _dccsi_config = importlib.util.module_from_spec(_spec_dccsi_config)
            _spec_dccsi_config.loader.exec_module(_dccsi_config)

            _LOGGER.debug('Executed config: {}'.format(_spec_dccsi_config))
        else:  # py2.x
            import imp
            _dccsi_config = imp.load_source(_module_tag, str(_dccsi_path.resolve()))
            _LOGGER.debug('Imported config: {}'.format(_spec_dccsi_config))
        return _dccsi_config

    else:
        return None
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
            bootstrap_box = box.Box.from_json(filename=str(json_file_path.resolve()),
                                              encoding="utf-8",
                                              errors="strict",
                                              object_pairs_hook=OrderedDict)
        except IOError as e:
            # this file runs in py2.7 for Maya 2020, FileExistsError is not defined
            _LOGGER.error('Bad file interaction: {}'.format(json_file_path.resolve()))
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
            _LOGGER.debug('O3DE project root: {}'.format(_PATH_O3DE_PROJECT.resolve()))
    return _PATH_O3DE_PROJECT
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def bootstrap_dccsi_py_libs(dccsi_dirpath=return_stub_dir()):
    """Builds and adds local site dir libs based on py version"""
    _PATH_DCCSI_PYTHON_LIB = Path(STR_PATH_DCCSI_PYTHON_LIB.format(dccsi_dirpath,
                                                                   sys.version_info[0],
                                                                   sys.version_info[1]))

    if _PATH_DCCSI_PYTHON_LIB.exists():
        site.addsitedir(_PATH_DCCSI_PYTHON_LIB.resolve())  # PYTHONPATH
        _LOGGER.debug('Performed site.addsitedir({})'
                      ''.format(_PATH_DCCSI_PYTHON_LIB.resolve()))
        return _PATH_DCCSI_PYTHON_LIB
    else:
        message = "Doesn't exist: {}".format(_PATH_DCCSI_PYTHON_LIB)
        _LOGGER.error(message)
        raise NotADirectoryError(message)
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone cli script for testing/debugging"""
    
    # global scope
    _MODULENAME = 'azpy.config_utils'
        
    # enable debug
    _DCCSI_GDEBUG = False # enable here to force temporarily
    _DCCSI_DEV_MODE = False
    _DCCSI_LOGLEVEL = _logging.INFO   
    
    # parse the command line args
    import argparse
    parser = argparse.ArgumentParser(
        description='O3DE DCCsi: {}'.format(_MODULENAME),
        epilog="Coomandline args enable deeper testing and info from commandline")
    
    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        help='Enables global debug flag.')
    
    parser.add_argument('-sd', '--set-debugger',
                        type=str,
                        required=False,
                        help='Default debugger: WING, others: PYCHARM, VSCODE (not yet implemented).')
    
    parser.add_argument('-dm', '--developer-mode',
                        type=bool,
                        required=False,
                        help='Enables dev mode for early auto attaching debugger.')
    
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

    # happy print
    _LOGGER.info("# {0} #".format('-' * 72))
    _LOGGER.info('~ config_utils.py ... Running script as __main__')
    _LOGGER.info("# {0} #".format('-' * 72))
    
    from pathlib import Path

    # built in simple tests and info from commandline
    _LOGGER.info('Current Work dir: {0}'.format(os.getcwd()))

    _LOGGER.info('OS: {}'.format(get_os()))
    
    _PATH_DCCSIG = Path(return_stub_dir('dccsi_stub'))
    _LOGGER.info('PATH_DCCSIG: {}'.format(_PATH_DCCSIG.resolve()))

    _O3DE_DEV = get_o3de_engine_root(check_stub='engine.json')
    _LOGGER.info('O3DE_DEV: {}'.format(_O3DE_DEV.resolve()))
    
    _PATH_O3DE_BUILD = get_o3de_build_path(_O3DE_DEV, 'CMakeCache.txt')
    _LOGGER.info('PATH_O3DE_BUILD: {}'.format(_PATH_O3DE_BUILD.resolve()))

    # new o3de version
    _PATH_O3DE_PROJECT = get_check_global_project()
    _LOGGER.info('PATH_O3DE_PROJECT: {}'.format(_PATH_O3DE_PROJECT.resolve()))

    _PATH_DCCSI_PYTHON_LIB = bootstrap_dccsi_py_libs(_PATH_DCCSIG)
    _LOGGER.info('PATH_DCCSI_PYTHON_LIB: {}'.format(_PATH_DCCSI_PYTHON_LIB.resolve()))

    _DCCSI_CONFIG = get_dccsi_config(_PATH_DCCSIG)
    _LOGGER.info('PATH_DCCSI_CONFIG: {}'.format(_DCCSI_CONFIG))
    # ---------------------------------------------------------------------
    
    # custom prompt
    sys.ps1 = "[azpy]>>"
    
_LOGGER.debug('DCCsi: config_utils.py took: {} sec'.format(time.process_time() - start)) 
# --- END -----------------------------------------------------------------