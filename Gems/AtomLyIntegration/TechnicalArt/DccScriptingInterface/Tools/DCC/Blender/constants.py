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
    < DCCsi >:: Tools//DCC//Blender//constants.py

This module contains default values for commonly used constants & strings.
We can make an update here easily that is propagated elsewhere.

Notice: this module should not actually set ENVARs in the os.environ
That would be the responsibility of a module like config.py
"""
# -------------------------------------------------------------------------
import timeit
_MODULE_START = timeit.default_timer()  # start tracking

# standard imports
import sys
import os
import site
import timeit
import inspect
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'Tools.DCC.Blender.constants'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
USER_HOME = Path.home()

# locally derived constants, working from cwd back to dccsi root
# mainly so we can ensure code access to the dccsi root
_MODULE_PATH = Path(os.path.abspath(inspect.getfile(inspect.currentframe())))
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')

ENVAR_PATH_DCCSI_TOOLS_BLENDER = "PATH_DCCSI_TOOLS_BLENDER"
_PATH_DCCSI_TOOLS_BLENDER = Path(_MODULE_PATH.parent)
_PATH_DCCSI_TOOLS_BLENDER = Path(os.getenv(ENVAR_PATH_DCCSI_TOOLS_BLENDER,
                                           _PATH_DCCSI_TOOLS_BLENDER.as_posix()))

ENVAR_PATH_DCCSI_TOOLS_DCC = "PATH_DCCSI_TOOLS_DCC"
_PATH_DCCSI_TOOLS_DCC = Path(_PATH_DCCSI_TOOLS_BLENDER.parent)
_PATH_DCCSI_TOOLS_DCC = Path(os.getenv(ENVAR_PATH_DCCSI_TOOLS_DCC,
                                       _PATH_DCCSI_TOOLS_DCC.as_posix()))

ENVAR_PATH_DCCSI_TOOLS = "PATH_DCCSI_TOOLS"
_PATH_DCCSI_TOOLS = Path(_PATH_DCCSI_TOOLS_DCC.parent)
_PATH_DCCSI_TOOLS = Path(os.getenv(ENVAR_PATH_DCCSI_TOOLS,
                                   _PATH_DCCSI_TOOLS.as_posix()))

# now we can set up basic access to the DCCsi
ENVAR_PATH_DCCSIG = "PATH_DCCSIG"
_PATH_DCCSIG = Path(_PATH_DCCSI_TOOLS.parent)
_PATH_DCCSIG = Path(os.getenv(ENVAR_PATH_DCCSIG, _PATH_DCCSIG.as_posix()))
site.addsitedir(_PATH_DCCSIG.as_posix())
_LOGGER.debug(f'_PATH_DCCSIG: {_PATH_DCCSIG.as_posix()}')

# now we have azpy api access
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER

# defaults, can be overridden/forced here for development
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_LOGLEVEL = env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO)
_DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# making these accessible for import
PATH_DCCSIG = _PATH_DCCSIG
PATH_DCCSI_TOOLS = _PATH_DCCSI_TOOLS
PATH_DCCSI_TOOLS_DCC = _PATH_DCCSI_TOOLS_DCC
PATH_DCCSI_TOOLS_BLENDER = _PATH_DCCSI_TOOLS_BLENDER

# Note: we've developed and tested with Blender 3.0 (experimental)
# change at your own risk, we are just future proofing.

# our dccsi location for substance designer <DCCsi>\Tools\DCC\Blender
ENVAR_DCCSI_TOOLS_BLENDER = "DCCSI_TOOLS_BLENDER"
PATH_DCCSI_TOOLS_BLENDER = Path(_MODULE_PATH.parent)
PATH_DCCSI_TOOLS_BLENDER = Path(os.getenv(ENVAR_DCCSI_TOOLS_BLENDER,
                                          PATH_DCCSI_TOOLS_BLENDER.as_posix()))

# DCCsi tools dir
# This could be improved, ENVAR Key should be refactored across files to 'DCCSI_TOOLS'
# ENVAR_PATH_DCCSI_TOOLS = "PATH_DCCSI_TOOLS"  # resolves to <DCCsi>\Tools
# if it is already defined in a higher up file pull from there
# to ensure a continual cascade
from azpy.constants import ENVAR_PATH_DCCSI_TOOLS
PATH_PATH_DCCSI_TOOLS = Path(PATH_DCCSI_TOOLS_BLENDER.parent.parent)
PATH_PATH_DCCSI_TOOLS = Path(os.getenv(ENVAR_PATH_DCCSI_TOOLS,
                                       PATH_PATH_DCCSI_TOOLS.as_posix()))

ENVAR_DCCSI_BLENDER_VERSION = "DCCSI_BLENDER_VERSION"
TAG_DCCSI_BLENDER_VERSION = "3.1"

ENVAR_DCCSI_BLENDER_PROJECT = "DCCSI_BLENDER_PROJECT"  # project name
EBVAR_DCCSI_BLENDER_PROJECT = "DCCSI_BLENDER_PROJECT"

ENVAR_DCCSI_BLENDER_ROOT_LOC = 'PATH_DCCSI_BLENDER_ROOT_LOC'
STR_DCCSI_BLENDER_ROOT_LOC = f'C:/Program Files/Blender Foundation/Blender'
PATH_DCCSI_BLENDER_LOC = Path(STR_DCCSI_BLENDER_ROOT_LOC).resolve()

ENVAR_PATH_DCCSI_BLENDER_LOC = "PATH_DCCSI_BLENDER_LOC"
STR_PATH_DCCSI_BLENDER_LOCATION = f'{STR_DCCSI_BLENDER_ROOT_LOC} {TAG_DCCSI_BLENDER_VERSION}'
PATH_DCCSI_BLENDER_LOC = Path(STR_PATH_DCCSI_BLENDER_LOCATION).resolve()

ENVAR_PATH_DCCSI_BLENDER_SCRIPTS = "PATH_DCCSI_BLENDER_SCRIPTS"
PATH_DCCSI_BLENDER_SCRIPTS = Path(PATH_DCCSI_TOOLS_BLENDER, 'Scripts').resolve()
PATH_DCCSI_BLENDER_SCRIPTS = Path(os.getenv(ENVAR_PATH_DCCSI_BLENDER_SCRIPTS,
                                            PATH_DCCSI_BLENDER_SCRIPTS.as_posix()))

# I think this one will launch with a console
TAG_BLENDER_EXE = "blender.exe"
ENVAR_PATH_DCCSI_BLENDER_EXE = "PATH_DCCSI_BLENDER_EXE"
STR_PATH_DCCSI_BLENDER_EXE = f"{PATH_DCCSI_BLENDER_LOC}/{TAG_BLENDER_EXE}"
PATH_DCCSI_BLENDER_EXE = Path(STR_PATH_DCCSI_BLENDER_EXE).resolve()

# this is the standard launcher that prevents the command window from popping up on start
# https://developer.blender.org/rBf3944cf503966a93a124e389d9232d7f833c0077
TAG_BLENDER_LAUNCHER_EXE = "blender-launcher.exe"
ENVAR_DCCSI_BLENDER_LAUNCHER_EXE = "DCCSI_BLENDER_LAUNCHER_EXE"
STR_DCCSI_BLENDER_LAUNCHER_EXE = f'{PATH_DCCSI_BLENDER_LOC}/{TAG_BLENDER_LAUNCHER_EXE}'
PATH_DCCSI_BLENDER_LAUNCHER_EXE = Path(STR_DCCSI_BLENDER_LAUNCHER_EXE).resolve()

# know where to find Blenders python if we ever need it
ENVAR_DCCSI_BLENDER_PYTHON_LOC = "PATH_DCCSI_BLENDER_PYTHON_LOC"
STR_DCCSI_BLENDER_PYTHON_LOC = f"{PATH_DCCSI_BLENDER_LOC}/{TAG_DCCSI_BLENDER_VERSION}/python"
PATH_DCCSI_BLENDER_PYTHON_LOC = Path(STR_DCCSI_BLENDER_PYTHON_LOC).resolve()

ENVAR_DCCSI_BLENDER_PY_EXE = "DCCSI_BLENDER_PY_EXE"
STR_DCCSI_BLENDER_PY_EXE = f'{PATH_DCCSI_BLENDER_PYTHON_LOC}/bin/python.exe'
PATH_DCCSI_BLENDER_PY_EXE = Path(STR_DCCSI_BLENDER_PY_EXE).resolve()

# our dccsi default start up script for blender aka bootstrap
TAG_DCCSI_BLENDER_BOOTSTRAP = "bootstrap.py"
ENVAR_PATH_DCCSI_BLENDER_BOOTSTRAP = "PATH_DCCSI_BLENDER_BOOTSTRAP"
STR_PATH_DCCSI_BLENDER_BOOTSTRAP = f'{PATH_DCCSI_TOOLS_BLENDER}/{TAG_DCCSI_BLENDER_BOOTSTRAP}'
PATH_DCCSI_BLENDER_BOOTSTRAP = Path(STR_PATH_DCCSI_BLENDER_BOOTSTRAP).resolve()

URL_DCCSI_BLENDER_WIKI = 'https://github.com/o3de/o3de/wiki/O3DE-DCCsi-Tools-DCC-Blender'
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone script"""

    # turn all of these off/on for testing
    _DCCSI_GDEBUG = False
    _DCCSI_DEV_MODE = False
    _DCCSI_LOGLEVEL = _logging.INFO
    _DCCSI_GDEBUGGER = 'WING'

    # configure basic logger
    from azpy.constants import FRMT_LOG_LONG
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                         format=FRMT_LOG_LONG,
                         datefmt='%m-%d %H:%M')

    _LOGGER = _logging.getLogger(_MODULENAME)

    from azpy.constants import STR_CROSSBAR
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info(f'~ {_MODULENAME} ... Running script as __main__')
    _LOGGER.info(STR_CROSSBAR)

    #  this is just a debug developer convenience print (for testing access)
    import pkgutil
    _LOGGER.info(f'Current working dir: {os.getcwd()}')
    search_path = ['.']  # set to None to see all modules importable from sys.path
    all_modules = [x[1] for x in pkgutil.iter_modules(path=search_path)]
    _LOGGER.info(f'All Available Modules in working dir: {all_modules}')

    #  test anything procedurally generated
    _LOGGER.info('Testing procedural env paths ...')
    from pathlib import Path

    _stash_dict = {}
    _stash_dict['PATH_DCCSIG'] = Path(PATH_DCCSIG)
    _stash_dict['PATH_DCCSI_TOOLS'] = Path(PATH_DCCSI_TOOLS)
    _stash_dict['PATH_DCCSI_TOOLS_DCC'] = Path(PATH_DCCSI_TOOLS_DCC)
    _stash_dict['PATH_DCCSI_TOOLS_BLENDER'] = Path(PATH_DCCSI_TOOLS_BLENDER)

    _stash_dict['PATH_DCCSI_BLENDER_LOC'] = Path(PATH_DCCSI_BLENDER_LOC)
    _stash_dict['DCCSI_BLENDER_EXE'] = Path(PATH_DCCSI_BLENDER_EXE)
    _stash_dict['DCCSI_BLENDER_LAUNCHER_EXE'] = Path(PATH_DCCSI_BLENDER_LAUNCHER_EXE)
    _stash_dict['PATH_DCCSI_BLENDER_PYTHON_LOC'] = Path(PATH_DCCSI_BLENDER_PYTHON_LOC)
    _stash_dict['DCCSI_BLENDER_PY_EXE'] = Path(PATH_DCCSI_BLENDER_PY_EXE)
    _stash_dict['PATH_DCCSI_BLENDER_BOOTSTRAP'] = Path(PATH_DCCSI_BLENDER_BOOTSTRAP)
    _stash_dict['URL_DCCSI_BLENDER_WIKI'] = Path(URL_DCCSI_BLENDER_WIKI)

    # ---------------------------------------------------------------------
    # py 2 and 3 compatible iter
    def get_items(dict_object):
        for key in dict_object:
            yield key, dict_object[key]

    for key, value in get_items(_stash_dict):
        # check if path exists
        try:
            value.exists()
            _LOGGER.info(F'{key}: {value}')
        except Exception as e:
            _LOGGER.warning(f'FAILED PATH: {e}')

    # custom prompt
    sys.ps1 = f"[{_MODULENAME}]>>"

_LOGGER.debug('{0} took: {1} sec'.format(_MODULENAME, timeit.default_timer() - _MODULE_START))
# --- END -----------------------------------------------------------------
