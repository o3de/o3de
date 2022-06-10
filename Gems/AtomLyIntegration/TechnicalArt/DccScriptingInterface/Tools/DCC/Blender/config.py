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
    < DCCsi >:: Tools//DCC//Blender//config.py

This module manages the dynamic config and settings for boostrapping Blender
"""
# -------------------------------------------------------------------------
# standard imports
import sys
import os
import site
import re
import importlib.util
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
#os.environ['PYTHONINSPECT'] = 'True'
# global scope
_MODULENAME = 'Tools.DCC.Blender.config'

_START = timeit.default_timer() # start tracking

# we need to set up basic access to the DCCsi
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_PATH_DCCSIG = Path(_MODULE_PATH, '../../../..').resolve()

# set envar so DCCsi synthetic env bootstraps with it (config.py)
from azpy.constants import ENVAR_PATH_DCCSIG
os.environ[ENVAR_PATH_DCCSIG] = str(_PATH_DCCSIG.as_posix())
site.addsitedir(_PATH_DCCSIG.as_posix())

from azpy.constants import FRMT_LOG_LONG
_logging.basicConfig(level=_logging.DEBUG,
                     format=FRMT_LOG_LONG,
                    datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()')
_LOGGER.debug(f'PATH_DCCSIG: {_PATH_DCCSIG.as_posix()')
# -------------------------------------------------------------------------


# defaults, can be overriden/forced here for development
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_LOGLEVEL = env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO)
_DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.constants import STR_PATH_DCCSI_PYTHON_LIB
from Tools.DCC.Blender.constants import STR_PATH_DCCSI_PYTHON_LIB

# override based on current executable
_PATH_DCCSI_PYTHON_LIB = STR_PATH_DCCSI_PYTHON_LIB.format(_PATH_DCCSIG,
                                                         sys.version_info.major,
                                                         sys.version_info.minor)
_PATH_DCCSI_PYTHON_LIB = Path(_PATH_DCCSI_PYTHON_LIB)
site.addsitedir(_PATH_DCCSI_PYTHON_LIB.as_posix())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# _settings.setenv()  # doing this will add the additional DYNACONF_ envars
def get_dccsi_config(PATH_DCCSIG=_PATH_DCCSIG.resolve()):
    """Convenience method to set and retreive settings directly from module."""

    try:
        Path(PATH_DCCSIG).exists()
    except FileNotFoundError as e:
        _LOGGER.debug(f"File does not exist: {PATH_DCCSIG}")
        return None

    # we can go ahead and just make sure the the DCCsi env is set
    # _config is SO generic this ensures we are importing a specific one
    _spec_dccsi_config = importlib.util.spec_from_file_location("dccsi._config",
                                                                Path(PATH_DCCSIG,
                                                                     "config.py"))
    _dccsi_config = importlib.util.module_from_spec(_spec_dccsi_config)
    _spec_dccsi_config.loader.exec_module(_dccsi_config)

    return _dccsi_config
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_CONFIG = get_dccsi_config()
settings = _CONFIG.get_config_settings(enable_o3de_python=False,
                                        enable_o3de_pyside2=True)
# we don't init the O3DE python env settings!
# that will cause conflicts with the DCC tools python!!!
# we are enabling the O3DE PySide2 (aka QtForPython) access

# now we can extend the environment specific to Blender
# start by grabbing the constants we want to work with as envars
# import others
from Tools.DCC.Blender.constants import *
# import them all, but below are the ones we will use directly
from Tools.DCC.Blender.constants import DCCSI_BLENDER_EXE
from Tools.DCC.Blender.constants import DCCSI_BLENDER_LAUNCHER
from Tools.DCC.Blender.constants import DCCSI_BLENDER_PY_EXE

#_DCCSI_PATH_BLENDER = Path(sys.prefix)
#os.environ["DYNACONF_DCCSI_PATH_BLENDER"] = _DCCSI_PATH_BLENDER.resolve()
#_LOGGER.debug(f"Blender Install: {_DCCSI_PATH_BLENDER}")

from dynaconf import settings
settings.setenv()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_dccsi_blender_settings(settings):
    return settings
# -------------------------------------------------------------------------
###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone cli script for testing/debugging"""
    
    # turn all of these off/on for testing
    _DCCSI_GDEBUG = False
    _DCCSI_DEV_MODE = False
    _DCCSI_LOGLEVEL = _logging.DEBUG
    _DCCSI_GDEBUGGER = 'WING'
    
    while 0: # easy flip these two on
        _DCCSI_GDEBUG = True
        _DCCSI_DEV_MODE = True
        break    