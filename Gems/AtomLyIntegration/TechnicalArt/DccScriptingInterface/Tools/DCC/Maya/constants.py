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
    < DCCsi >:: Tools/DCC/Maya/constants.py

This module contains default values for commony used constants & strings.
We can make an update here easily that is propogated elsewhere.
"""
# -------------------------------------------------------------------------
# built-ins
import sys
import os
import site
import timeit
import inspect
from os.path import expanduser
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_START = timeit.default_timer() # start tracking

# global scope
_MODULENAME = 'Tools.DCC.Maya.constants'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Maya is frozen
# module path when frozen
_MODULE_PATH = Path(os.path.abspath(inspect.getfile(inspect.currentframe())))
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH))

_PATH_DCCSI_TOOLS_MAYA = Path(_MODULE_PATH.parent)
_PATH_DCCSI_TOOLS_MAYA = Path(os.getenv('PATH_DCCSI_TOOLS_MAYA',
                                        _PATH_DCCSI_TOOLS_MAYA.as_posix()))

_PATH_DCCSI_TOOLS_DCC = Path(_PATH_DCCSI_TOOLS_MAYA.parent)
_PATH_DCCSI_TOOLS_DCC = Path(os.getenv('PATH_DCCSI_TOOLS_DCC',
                                       _PATH_DCCSI_TOOLS_DCC.as_posix()))

_PATH_DCCSI_TOOLS = Path(_PATH_DCCSI_TOOLS_DCC.parent)
_PATH_DCCSI_TOOLS = Path(os.getenv('PATH_DCCSI_TOOLS',
                                   _PATH_DCCSI_TOOLS.as_posix()))

# we need to set up basic access to the DCCsi
_PATH_DCCSIG = Path(_PATH_DCCSI_TOOLS.parent)
_PATH_DCCSIG = Path(os.getenv('PATH_DCCSIG', _PATH_DCCSIG.as_posix()))
site.addsitedir(_PATH_DCCSIG.as_posix())

_LOGGER.debug('_PATH_DCCSIG: {}'.format(_PATH_DCCSIG.as_posix()))

# this is the shared default requirements.txt file to install for python 3.6.x+
DCCSI_PYTHON_REQUIREMENTS = Path(_PATH_DCCSIG, 'requirements.txt').as_posix()
# if using maya 2020 or less with py2.7 override with and use the one here:
# "DccScriptingInterface\Tools\DCC\Maya\requirements.txt"

# now we have azpy api access
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.constants import * # but here are the specific ones we are gonna use
from azpy.constants import PATH_PROGRAMFILES_X64
from azpy.constants import TAG_PY_MAJOR
from azpy.constants import TAG_PY_MINOR
from azpy.constants import PATH_USER_HOME
from azpy.constants import PATH_USER_O3DE
from azpy.constants import ENVAR_O3DE_DEV
from azpy.constants import PATH_O3DE_DEV
from azpy.constants import ENVAR_PATH_DCCSIG
from azpy.constants import PATH_DCCSIG
from azpy.constants import ENVAR_DCCSI_LOG_PATH
from azpy.constants import PATH_DCCSI_LOG_PATH
from azpy.constants import ENVAR_DCCSI_PY_VERSION_MAJOR
from azpy.constants import ENVAR_DCCSI_PY_VERSION_MINOR
from azpy.constants import ENVAR_PATH_DCCSI_PYTHON_LIB
from azpy.constants import STR_PATH_DCCSI_PYTHON_LIB
from azpy.constants import PATH_DCCSI_PYTHON_LIB
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# dcc: Maya ENVAR constants
ENVAR_DCCSI_PY_VERSION_MAJOR=str("DCCSI_PY_VERSION_MAJOR")
ENVAR_DCCSI_PY_VERSION_MINOR=str("DCCSI_PY_VERSION_MINOR")
ENVAR_DCCSI_PY_VERSION_RELEASE=str("DCCSI_PY_VERSION_RELEASE")

ENVAR_MAYA_NO_CONSOLE_WINDOW = str("MAYA_NO_CONSOLE_WINDOW")
ENVAR_MAYA_SHOW_OUTPUT_WINDOW = str("MAYA_SHOW_OUTPUT_WINDOW")

TAG_O3DE_DCC_MAYA_MEL = 'dccsi_setup.mel'
TAG_MAYA_WORKSPACE = 'workspace.mel'

ENVAR_DCCSI_PY_MAYA = str('DCCSI_PY_MAYA')

ENVAR_MAYA_VERSION = str('MAYA_VERSION')
ENVAR_MAYA_LOCATION = str('MAYA_LOCATION')

ENVAR_PATH_DCCSI_TOOLS_MAYA = str('PATH_DCCSI_TOOLS_MAYA')
ENVAR_MAYA_MODULE_PATH = str('MAYA_MODULE_PATH')
ENVAR_MAYA_BIN_PATH = str('MAYA_BIN_PATH')

ENVAR_DCCSI_MAYA_PLUG_IN_PATH = str('DCCSI_MAYA_PLUG_IN_PATH')
ENVAR_MAYA_PLUG_IN_PATH = str('MAYA_PLUG_IN_PATH')

ENVAR_DCCSI_MAYA_SHELF_PATH = str('DCCSI_MAYA_SHELF_PATH')
ENVAR_MAYA_SHELF_PATH = str('MAYA_SHELF_PATH')

ENVAR_DCCSI_MAYA_XBMLANGPATH = str('DCCSI_MAYA_XBMLANGPATH')
ENVAR_XBMLANGPATH = str('XBMLANGPATH')

ENVAR_DCCSI_MAYA_SCRIPT_MEL_PATH = str('DCCSI_MAYA_SCRIPT_MEL_PATH')
ENVAR_DCCSI_MAYA_SCRIPT_PY_PATH = str('DCCSI_MAYA_SCRIPT_PY_PATH')
ENVAR_DCCSI_MAYA_SCRIPT_PATH = str("DCCSI_MAYA_SCRIPT_PATH")
ENVAR_MAYA_SCRIPT_PATH = str('MAYA_SCRIPT_PATH')

ENVAR_DCCSI_MAYA_SET_CALLBACKS = str('DCCSI_MAYA_SET_CALLBACKS')

ENVAR_MAYA_VP2_DEVICE_OVERRIDE=str("MAYA_VP2_DEVICE_OVERRIDE")
ENVAR_MAYA_OGS_DEVICE_OVERRIDE=str("MAYA_OGS_DEVICE_OVERRIDE")
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Maya consts
#USER_HOME = Path.home()
# mimicing all values from: "DccScriptingInterface\Tools\Dev\Windows\Env_DCC_Maya.bat"
# note: these are just default values, they are only initially CONST
# if/when imported from here (constants.py)
DCCSI_PY_VERSION_MAJOR   = 3
DCCSI_PY_VERSION_MINOR   = 7
DCCSI_PY_VERSION_RELEASE = 7

# override with maya defaults
PATH_DCCSI_PYTHON_LIB = STR_PATH_DCCSI_PYTHON_LIB.format(_PATH_DCCSIG,
                                                         DCCSI_PY_VERSION_MAJOR,
                                                         DCCSI_PY_VERSION_MINOR)

# not actually a maya envar, to do: could rename DCCSI_MAYA_VERSION
MAYA_VERSION=2022

# is a maya envar
MAYA_PROJECT = _PATH_DCCSIG.as_posix()

PATH_DCCSI_TOOLS_MAYA = _PATH_DCCSI_TOOLS_MAYA.as_posix()

# is a maya envar
MAYA_MODULE_PATH = _PATH_DCCSI_TOOLS_MAYA.as_posix()

# is a maya envar
MAYA_LOCATION = Path(PATH_PROGRAMFILES_X64,'Autodesk', 'Maya{}'.format(MAYA_VERSION)).as_posix()

# is a maya envar
MAYA_BIN_PATH = Path(MAYA_LOCATION, 'bin').as_posix()

DCCSI_MAYA_SET_CALLBACKS = True

# is a maya envar
MAYA_NO_CONSOLE_WINDOW = False
MAYA_SHOW_OUTPUT_WINDOW = True

DCCSI_MAYA_EXE = Path(MAYA_BIN_PATH, 'maya.exe') 

DCCSI_MAYABATCH_EXE = Path(MAYA_BIN_PATH, 'mayabatch.exe') 

DCCSI_PY_MAYA = Path(MAYA_BIN_PATH, 'mayapy.exe') 
        
# this is transient and will always track the exe this script is executing on
O3DE_PY_EXE = Path(sys.executable).as_posix()
DCCSI_PY_IDE = Path(DCCSI_PY_MAYA).as_posix()

DCCSI_MAYA_PLUG_IN_PATH = Path(PATH_DCCSI_TOOLS_MAYA,'plugins').as_posix()

# is a maya envar
MAYA_PLUG_IN_PATH = Path(DCCSI_MAYA_PLUG_IN_PATH).as_posix() # extend %MAYA_PLUG_IN_PATH%
# to do: remove or extend next PR, technically there can be more then one plugin path
#while MAYA_PLUG_IN_PATH:
    #if ENVAR_MAYA_PLUG_IN_PATH in os.environ:
        #maya_plug_pathlist = os.getenv(ENVAR_MAYA_PLUG_IN_PATH).split(os.pathsep)
        #maya_plug_new_pathlist = maya_plug_pathlist.copy()
        #maya_plug_new_pathlist.insert(0, Path(DCCSI_MAYA_PLUG_IN_PATH).as_posix())
        #os.environ[ENVAR_MAYA_PLUG_IN_PATH] = os.pathsep.join(maya_plug_new_pathlist)
    #else:
        #os.environ[ENVAR_MAYA_PLUG_IN_PATH] = DCCSI_MAYA_PLUG_IN_PATH
    
    #MAYA_PLUG_IN_PATH = os.getenv(ENVAR_MAYA_PLUG_IN_PATH, "< NOT SET >")
    #break

DCCSI_MAYA_SHELF_PATH = Path(PATH_DCCSI_TOOLS_MAYA, 'Prefs', 'Shelves').as_posix()

DCCSI_MAYA_XBMLANGPATH = Path(PATH_DCCSI_TOOLS_MAYA, 'Prefs', 'icons').as_posix()

# is a maya envar
# maya resources, very oddly named
XBMLANGPATH = Path(DCCSI_MAYA_XBMLANGPATH).as_posix() # extend %XBMLANGPATH%
# to do: remove or extend next PR, technically there can be more then one resource path specified
#while XBMLANGPATH:
    #if ENVAR_XBMLANGPATH in os.environ:
        #maya_xbm_pathlist = os.getenv(ENVAR_XBMLANGPATH).split(os.pathsep)
        #maya_xbm_new_pathlist = maya_xbm_pathlist.copy()
        #maya_xbm_new_pathlist.insert(0, Path(DCCSI_MAYA_XBMLANGPATH).as_posix())
        #os.environ[ENVAR_XBMLANGPATH] = os.pathsep.join(maya_xbm_new_pathlist)
    #else:
        #os.environ[ENVAR_XBMLANGPATH] = DCCSI_MAYA_XBMLANGPATH
    
    #XBMLANGPATH = os.getenv(ENVAR_XBMLANGPATH, "< NOT SET >")
    #break

DCCSI_MAYA_SCRIPT_PATH = Path(PATH_DCCSI_TOOLS_MAYA, 'Scripts').as_posix()
DCCSI_MAYA_SCRIPT_MEL_PATH = Path(PATH_DCCSI_TOOLS_MAYA, 'Scripts', 'Mel').as_posix()
DCCSI_MAYA_SCRIPT_PY_PATH = Path(PATH_DCCSI_TOOLS_MAYA, 'Scripts', 'Python').as_posix()

MAYA_SCRIPT_PATH = Path(DCCSI_MAYA_SCRIPT_PATH).as_posix() # extend %MAYA_SCRIPT_PATH%
# to do: remove or extend next PR, technically there can be more then one script path specified
#while MAYA_SCRIPT_PATH:
    #if ENVAR_MAYA_SCRIPT_PATH in os.environ:
        #maya_script_pathlist = os.getenv(ENVAR_MAYA_SCRIPT_PATH).split(os.pathsep)
        #maya_script_new_pathlist = maya_script_pathlist.copy()
        #maya_script_new_pathlist.insert(0, DCCSI_MAYA_SCRIPT_MEL_PATH)
        #maya_script_new_pathlist.insert(0, DCCSI_MAYA_SCRIPT_PY_PATH)
        #maya_script_new_pathlist.insert(0, DCCSI_MAYA_SCRIPT_PATH)
        #os.environ[ENVAR_MAYA_SCRIPT_PATH] = os.pathsep.join(maya_script_new_pathlist)
    #else:
        #os.environ[ENVAR_MAYA_SCRIPT_PATH] = os.pathsep.join( (DCCSI_MAYA_SCRIPT_PATH,
                                                               #DCCSI_MAYA_SCRIPT_PY_PATH,
                                                               #DCCSI_MAYA_SCRIPT_MEL_PATH) )
    
    #MAYA_SCRIPT_PATH = os.getenv(ENVAR_MAYA_SCRIPT_PATH, "< NOT SET >")
    #break

# is a maya envar
MAYA_VP2_DEVICE_OVERRIDE="VirtualDeviceDx11"
MAYA_OGS_DEVICE_OVERRIDE="VirtualDeviceDx11"

DCCSI_MAYA_WIKI_URL =  'https://github.com/o3de/o3de/wiki/O3DE-DCCsi-Tools-DCC-Maya'

# reference, here is a list of Maya envars
# https://github.com/mottosso/Maya-Environment-Variables/blob/master/README.md
# -------------------------------------------------------------------------    


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone script"""
    
    # happy print
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('~ {}.py ... Running script as __main__'.format(_MODULENAME))
    _LOGGER.info(STR_CROSSBAR)
    
    # global debug stuff
    _DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, True)
    _DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, True)    
    _DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))
    if _DCCSI_GDEBUG:
        # override loglevel if runnign debug
        _DCCSI_LOGLEVEL = _logging.DEBUG
        
    # configure basic logger
    # note: not using a common logger to reduce cyclical imports
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                        format=FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')
    
    # re-configure basic logger for debug
    _LOGGER = _logging.getLogger(_MODULENAME)

    #  this is just a debug developer convenience print (for testing acess)
    import pkgutil
    _LOGGER.info('Current working dir: {0}'.format(os.getcwd()))
    search_path = ['.']  # set to None to see all modules importable from sys.path
    all_modules = [x[1] for x in pkgutil.iter_modules(path=search_path)]
    _LOGGER.info('All Available Modules in working dir: {0}'.format(all_modules))
    
    # override based on current executable
    PATH_DCCSI_PYTHON_LIB = STR_PATH_DCCSI_PYTHON_LIB.format(_PATH_DCCSIG,
                                                             sys.version_info.major,
                                                             sys.version_info.minor)
    PATH_DCCSI_PYTHON_LIB = Path(PATH_DCCSI_PYTHON_LIB).as_posix()

    #  test anything procedurally generated
    _LOGGER.info('Testing procedural env paths ...')
    from pathlib import Path

    _stash_dict = {}
    _stash_dict['O3DE_DEV'] = Path(PATH_O3DE_DEV)
    _stash_dict['PATH_DCCSIG'] = Path(PATH_DCCSIG)
    _stash_dict['DCCSI_AZPY_PATH'] = Path(PATH_DCCSI_AZPY_PATH)
    _stash_dict['PATH_DCCSI_TOOLS'] = Path(PATH_DCCSI_TOOLS)
    _stash_dict['PATH_DCCSI_PYTHON_LIB'] = Path(PATH_DCCSI_PYTHON_LIB)
    _stash_dict['PATH_DCCSI_TOOLS_MAYA'] = Path(PATH_DCCSI_TOOLS_MAYA)
    _stash_dict['MAYA_LOCATION'] = Path(MAYA_LOCATION)
    _stash_dict['DCCSI_MAYA_EXE'] = Path(DCCSI_MAYA_EXE)
    _stash_dict['DCCSI_PY_MAYA'] = Path(DCCSI_PY_MAYA)
    _stash_dict['MAYA_SCRIPT_PATH'] = Path(MAYA_SCRIPT_PATH)

    # ---------------------------------------------------------------------
    # py 2 and 3 compatible iter    
    def get_items(dict_object):
        for key in dict_object:
            yield key, dict_object[key]

    for key, value in get_items(_stash_dict):
        # check if path exists
        try:
            value.exists()
            _LOGGER.info('{0}: {1}'.format(key, value))
        except Exception as e:
            _LOGGER.warning('FAILED PATH: {}'.format(e)) 

    # custom prompt
    sys.ps1 = "[{}]>>".format(_MODULENAME)

_LOGGER.debug('{0} took: {1} sec'.format(_MODULENAME, timeit.default_timer() - _START)) 
# --- END -----------------------------------------------------------------
        
