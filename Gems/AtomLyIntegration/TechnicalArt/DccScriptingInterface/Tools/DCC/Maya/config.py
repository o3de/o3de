#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! This module manages the dynamic config and settings for bootstrapping
Maya DCC app integration with o3de inter-op, scripts, extensions, etc.

:file: DccScriptingInterface\\Tools\\DCC\\Maya\\config.py
:Status: Prototype
:Version: 0.0.1
:Notice: Currently only supports Maya 2022 and 2023 with Python3
"""
# -------------------------------------------------------------------------
import timeit
_MODULE_START = timeit.default_timer()  # start tracking

# standard imports
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------

# This module and others like Substance/config.py have a lot of duplicate
# boilerplate code (as we are early, these are the first versions to stand up)
# They could possibly be improved by unifying into a Class object designed
# with extensibility

# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.Tools.DCC.Maya import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.config'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

# ensure dccsi and o3de core access
# in a future iteration it is suggested that the core config
# be rewritten from ConfigClass, then BlenderConfig inherits core
import DccScriptingInterface.config as dccsi_core_config

_settings_core = dccsi_core_config.get_config_settings(enable_o3de_python=True,
                                                       enable_o3de_pyside2=False,
                                                       set_env=True)

# local dccsi imports
# this accesses common global state, e.g. DCCSI_GDEBUG (is True or False)
from DccScriptingInterface.globals import *

# this will auto-attach ide debugging at the earliest possible point in module
from azpy.config_utils import attach_debugger
if DCCSI_DEV_MODE: # from DccScriptingInterface.globals
    attach_debugger(debugger_type=DCCSI_GDEBUGGER)

# if the dccsi core config and it's settings are loaded this should pass
try:
    _settings_core.DCCSI_CONFIG_CORE
except EnvironmentError as e:
    _LOGGER.error('Setting does not exist: DCCSI_CONFIG_CORE')
    _LOGGER.warning(f'EnvironmentError: {e}')

# this is the root path for the wing pkg
from DccScriptingInterface.Tools.DCC.Maya import ENVAR_PATH_DCCSI_TOOLS_DCC_MAYA
from DccScriptingInterface.Tools.DCC.Maya import PATH_DCCSI_TOOLS_DCC_MAYA

from DccScriptingInterface.Tools import ENVAR_PATH_DCCSI_TOOLS
from DccScriptingInterface.Tools import PATH_DCCSI_TOOLS

from DccScriptingInterface import ENVAR_PATH_O3DE_PROJECT
from DccScriptingInterface import PATH_O3DE_PROJECT

from DccScriptingInterface import SETTINGS_FILE_SLUG
PATH_DCCSI_TOOLS_DCC_MAYA_SETTINGS = PATH_DCCSI_TOOLS_DCC_MAYA.joinpath(SETTINGS_FILE_SLUG).resolve()

from DccScriptingInterface import LOCAL_SETTINGS_FILE_SLUG
PATH_DCCSI_TOOLS_DCC_MAYA_LOCAL_SETTINGS = PATH_DCCSI_TOOLS_DCC_MAYA.joinpath(LOCAL_SETTINGS_FILE_SLUG).resolve()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# now we build the wing config class
from DccScriptingInterface.azpy.config_class import ConfigClass

class MayaConfig(ConfigClass):
    """Extend ConfigClass with new maya functionality"""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        _LOGGER.info(f'Initializing: {self.get_classname()}')

# build config object
maya_config = MayaConfig(config_name='dccsi_dcc_maya',
                         settings_filepath = PATH_DCCSI_TOOLS_DCC_MAYA_SETTINGS,
                         settings_local_filepath = PATH_DCCSI_TOOLS_DCC_MAYA_LOCAL_SETTINGS,
                         auto_set=True)

# in another module someone could work this way
# from DccScriptingInterface.Tools.DCC.Maya.config import maya_config
# settings = maya_config.get_settings(set_env=True)
# or
# if maya_config.settings.THIS_SETTING: do this

from Tools.DCC.Maya.constants import *

# now we can extend the environment specific to Maya
maya_config.add_setting(ENVAR_DCCSI_CONFIG_DCC_MAYA, DCCSI_CONFIG_DCC_MAYA)

PATH_DCCSI_TOOLS_DCC_MAYA = Path(PATH_DCCSI_TOOLS_DCC_MAYA).resolve()
maya_config.add_setting(ENVAR_PATH_DCCSI_TOOLS_DCC_MAYA,
                        PATH_DCCSI_TOOLS_DCC_MAYA.as_posix())

PATH_DCCSI_TOOLS = Path(PATH_DCCSI_TOOLS).resolve()
maya_config.add_setting(ENVAR_PATH_DCCSI_TOOLS,
                           PATH_DCCSI_TOOLS.as_posix())

PATH_DCCSI_TOOLS_DCC_MAYA = Path(PATH_DCCSI_TOOLS_DCC_MAYA).resolve()
maya_config.add_setting(ENVAR_PATH_DCCSI_TOOLS_DCC_MAYA,
                        PATH_DCCSI_TOOLS_DCC_MAYA.as_posix())

maya_config.add_setting(ENVAR_DCCSI_PY_VERSION_MAJOR, DCCSI_PY_VERSION_MAJOR)
maya_config.add_setting(ENVAR_DCCSI_PY_VERSION_MINOR, DCCSI_PY_VERSION_MINOR)
maya_config.add_setting(ENVAR_DCCSI_PY_VERSION_RELEASE, DCCSI_PY_VERSION_RELEASE)
maya_config.add_setting(ENVAR_MAYA_VERSION, MAYA_VERSION)

PATH_O3DE_PROJECT = Path(PATH_O3DE_PROJECT).resolve()
maya_config.add_setting(ENVAR_PATH_O3DE_PROJECT, PATH_O3DE_PROJECT.as_posix())

MAYA_LOCATION = Path(MAYA_LOCATION).resolve()
maya_config.add_setting(ENVAR_MAYA_LOCATION, MAYA_LOCATION.as_posix())

MAYA_BIN_PATH = Path(MAYA_BIN_PATH).resolve()
maya_config.add_setting(ENVAR_MAYA_BIN_PATH, MAYA_BIN_PATH.as_posix())

DCCSI_MAYA_PLUG_IN_PATH = Path(DCCSI_MAYA_PLUG_IN_PATH).resolve()
maya_config.add_setting(ENVAR_DCCSI_MAYA_PLUG_IN_PATH, DCCSI_MAYA_PLUG_IN_PATH.as_posix())

# --- END -----------------------------------------------------------------

settings = maya_config.get_config_settings()

_MODULE_END = timeit.default_timer() - _MODULE_START
_LOGGER.debug(f'{_MODULENAME} took: {_MODULE_END} sec')
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone cli script for testing/debugging"""

    # this should hit this modules location and load wing settings
    settings = maya_config.get_config_settings()

    try:
        settings.CONFIG_DCC_BLENDER
        _LOGGER.info('Blender config is enabled')
    except:
        _LOGGER.error('Setting does not exist')

    _LOGGER.info(f'Exporting local settings: {PATH_DCCSI_TOOLS_DCC_BLENDER_LOCAL_SETTINGS}')

    try:
        maya_config.export_settings(set_env=True,
                                       log_settings=True)
    except Exception as e:
        _LOGGER.error(f'{e}')
# --- END -----------------------------------------------------------------
