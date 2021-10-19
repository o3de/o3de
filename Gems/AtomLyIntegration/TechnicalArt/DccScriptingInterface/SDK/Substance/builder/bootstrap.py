# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# -- This line is 75 characters -------------------------------------------
"""Empty Doc String"""  # To Do: add documentation
# -------------------------------------------------------------------------
#  built-ins
import sys
import os
import site
import importlib.util
#import logging as _logging

# if running in py2.7 we won't have access to pathlib yet until we boostrap
# the DCCsi
_MODULE_PATH = os.path.realpath(__file__)  # To Do: what if frozen?
_DCCSIG_PATH = os.path.normpath(os.path.join(_MODULE_PATH, '../../../..'))
_DCCSIG_PATH = os.getenv('DCCSIG_PATH', _DCCSIG_PATH)
site.addsitedir(_DCCSIG_PATH)
# print(_DCCSIG_PATH)

# Lumberyard DCCsi site extensions
from pathlib import Path

# set up global space, logging etc.
import azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

# these are for module debugging, set to false on submit
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_PACKAGENAME = 'DCCsi.SDK.substance.builder.bootstrap'

_log_level = int(20)
if _DCCSI_GDEBUG:
    _log_level = int(10)
_LOGGER = azpy.initialize_logger(_PACKAGENAME,
                                 log_to_file=True,
                                 default_log_level=_log_level)
_LOGGER.debug('Starting up:  {0}.'.format({_PACKAGENAME}))
_LOGGER.debug('_DCCSIG_PATH: {}'.format(_DCCSIG_PATH))
_LOGGER.debug('_G_DEBUG: {}'.format(_DCCSI_GDEBUG))
_LOGGER.debug('_DCCSI_DEV_MODE: {}'.format(_DCCSI_DEV_MODE))

if _DCCSI_DEV_MODE:
    from azpy.test.entry_test import connect_wing
    foo = connect_wing()

# To Do: now that we have figured out the pattern and this is working
# we should consider initializing this earlier and reduce code lines?    
# we can go ahead and just make sure the the DCCsi env is set
# config is SO generic this ensures we are importing a specific one
_spec_dccsi_config = importlib.util.spec_from_file_location("dccsi.config",
                                                            Path(_DCCSIG_PATH,
                                                                 "config.py"))
_dccsi_config = importlib.util.module_from_spec(_spec_dccsi_config)
_spec_dccsi_config.loader.exec_module(_dccsi_config)

from dynaconf import settings

try:
    from PySide2.QtWidgets import QApplication
except:
    _dccsi_config.init_o3de_pyside(settings.O3DE_DEV)  # init for standalone
    # running in the editor if the QtForPython Gem is enabled
    # you should already have access and shouldn't need to set up
    
settings.setenv() # initialize the dynamic env and settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# substance automation toolkit (aka pysbs)
# to do: move this into SDK\Substance using per-app dynaconf config extension
from azpy.constants import PATH_PROGRAMFILES_X64
# this could be moved into a constants file (where?)
_PYSBS_DIR_PATH = Path(PATH_PROGRAMFILES_X64,
                       'Allegorithmic',
                       'Substance Automation Toolkit',
                       'Python API',
                       'install').resolve()
os.environ["DYNACONF_QPYSBS_DIR_PATH"] = str(_PYSBS_DIR_PATH)
os.environ["PYSBS_DIR_PATH"] = str(_PYSBS_DIR_PATH)

# standard paths we may use downstream
# To Do: move these into a dynaconf config extension specific to this tool?
from azpy.constants import ENVAR_O3DE_DEV
_O3DE_DEV = Path(os.getenv(ENVAR_O3DE_DEV,
                         settings.O3DE_DEV)).resolve()

from azpy.constants import ENVAR_O3DE_PROJECT_PATH
_O3DE_PROJECT_PATH = Path(os.getenv(ENVAR_O3DE_PROJECT_PATH,
                                  settings.O3DE_PROJECT_PATH)).resolve()

from azpy.constants import ENVAR_DCCSI_SDK_PATH
_DCCSI_SDK_PATH = Path(os.getenv(ENVAR_DCCSI_SDK_PATH,
                                 settings.DCCSIG_SDK_PATH)).resolve()

# build some reuseable path parts for the substance builder
_PROJECT_ASSETS_PATH = Path(_O3DE_PROJECT_PATH, 'Assets').resolve()
_PROJECT_MATERIALS_PATH = Path(_PROJECT_ASSETS_PATH, 'Materials').resolve()
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == "__main__":
    """Run this file as main"""

    _LOGGER.info('_O3DE_DEV: {}'.format(_O3DE_DEV))
    _LOGGER.info('_O3DE_PROJECT_PATH: {}'.format(_O3DE_PROJECT_PATH))
    _LOGGER.info('_DCCSI_SDK_PATH: {}'.format(_DCCSI_SDK_PATH))
    
    _LOGGER.info('_PYSBS_DIR_PATH: {}'.format(_PYSBS_DIR_PATH))
    _LOGGER.info('_PROJECT_ASSETS_PATH: {}'.format(_PROJECT_ASSETS_PATH))
    _LOGGER.info('_PROJECT_MATERIALS_PATH: {}'.format(_PROJECT_MATERIALS_PATH))

    if _DCCSI_GDEBUG:
        _dccsi_config.test_pyside2()  # runs a small PySdie2 test
    
    # remove the logger
    del _LOGGER
# ---- END ---------------------------------------------------------------
