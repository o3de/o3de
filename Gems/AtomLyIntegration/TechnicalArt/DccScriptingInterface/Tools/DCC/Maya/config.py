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
import sys
from pathlib import Path
from typing import Union
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# need to self bootstrap if debugging maya config
_MODULE_PATH = Path(__file__)
PATH_O3DE_TECHART_GEMS = _MODULE_PATH.parents[4].resolve()
sys.path.insert(0, str(PATH_O3DE_TECHART_GEMS))

# global scope
from DccScriptingInterface.Tools.DCC.Maya import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.config'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

# dynaconf boilerplate
from dynaconf import Dynaconf
settings = Dynaconf(envar_prefix='DYNACONF',
                    # the following will also load settings.local.json
                    settings_files=['settings.json', '.secrets.json'])

settings.setenv() # ensure default file based settings are in the env

from DccScriptingInterface import add_site_dir
add_site_dir(PATH_O3DE_TECHART_GEMS) # cleaner add

# ensure dccsi and o3de core access
# in a future iteration it is suggested that the core config
# be rewritten from ConfigClass, then BlenderConfig inherits core
import DccScriptingInterface.config as dccsi_core_config
# logic based dccsi config management
_settings_core = dccsi_core_config.get_config_settings(enable_o3de_python=False,
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
# import all maya consts
from Tools.DCC.Maya.constants import *

# now we build the wing config class
from DccScriptingInterface.azpy.config_class import ConfigClass

class MayaConfig(ConfigClass):
    """Extend ConfigClass with new maya functionality"""
    def __init__(self, version = MAYA_VERSION, *args, **kwargs):
        super().__init__(*args, **kwargs)

        _LOGGER.info(f'Initializing: {self.get_classname()}')

        # defaults for version and location
        self._maya_version = version

        # paths we want to update if version changes
        self._maya_location = Path(MAYA_LOCATION)
        self._maya_bin_path = Path(MAYA_BIN_PATH)

        # initializes version based defaults paths
        self.update_version(self._maya_version)

    # -- properties -------------------------------------------------------

    @property
    def maya_location(self):
        ''':Class property: the location of maya'''
        return self._maya_location

    @maya_location.setter
    def maya_location(self,
                      value: Union[str, Path] = MAYA_LOCATION,
                      envar = ENVAR_MAYA_LOCATION,
                      check_envar = False,
                      set_envar = True,
                      set_sys_path = True):
        ''':param value: set the path for maya location'''
        self._maya_location = Path(value).resolve()

        # store setting
        self.add_setting(key = envar,
                         value = self._maya_location,
                         check_envar = check_envar,
                         set_envar = set_envar,
                         set_sys_path = set_sys_path)

        # update the location to the bin folder
        self.maya_bin_path = f'{self.maya_location}\\bin'

        return self._maya_location

    @maya_location.getter
    def maya_location(self):
        ''':return: the maya version'''
        return self._maya_location

    @property
    def maya_bin_path(self):
        ''':Class property: the location of maya binaries folder (exe)'''
        return self._maya_bin_path

    @maya_bin_path.setter
    def maya_bin_path(self,
                      value: Union[str, Path] = MAYA_BIN_PATH,
                      envar = ENVAR_MAYA_BIN_PATH,
                      check_envar = False,
                      set_envar = True,
                      set_sys_path = True):
        ''':param value: set the path for maya location'''
        self._maya_bin_path = Path(value).resolve()

        # store setting
        self.add_setting(key = envar,
                         value = self._maya_bin_path,
                         check_envar = check_envar,
                         set_envar = set_envar,
                         set_sys_path = set_sys_path)

        # update the various exe paths
        self.add_setting(key = ENVAR_DCCSI_MAYA_EXE,
                         value = Path(f'{self.maya_bin_path}\\{SLUG_MAYA_EXE}'),
                         check_envar = check_envar,
                         set_envar = set_envar)

        self.add_setting(key = ENVAR_DCCSI_PY_MAYA,
                         value = Path(f'{self.maya_bin_path}\\{SLUG_MAYAPY_EXE}'),
                         check_envar = check_envar,
                         set_envar = set_envar)

        self.add_setting(key = ENVAR_DCCSI_MAYABATCH_EXE,
                         value = Path(f'{self.maya_bin_path}\\{SLUG_MAYABATCH_EXE}'),
                         check_envar = check_envar,
                         set_envar = set_envar)

        return self._maya_location

    @maya_bin_path.getter
    def maya_bin_path(self):
        ''':return: the maya version'''
        return self._maya_bin_path

    @property
    def maya_version(self):
        ''':Class property: the version of maya, e.g. 2023'''
        return self._maya_version

    @maya_version.setter
    def maya_version(self,
                     value: Union[int, str] = MAYA_VERSION,
                     envar = ENVAR_MAYA_VERSION,
                     check_envar = False,
                     set_envar = True):
        ''':param value: sets the version of maya and updates related paths.
           :return: the maya version
           '''
        self._maya_version = value

        # store setting
        self.add_setting(envar, value,
                         check_envar = check_envar,
                         set_envar = set_envar)

        # update the maya location
        self.maya_location = Path(f'{PATH_PROGRAMFILES_X64}\\Autodesk\\Maya{self._maya_version}')

        return self._maya_version

    @maya_version.getter
    def maya_version(self):
        ''':return: the maya version'''
        return self._maya_version

    def update_version(self, value: Union[int, str] = MAYA_VERSION):
        '''Class method: updates all path dependant on the version'''

        # doing this first as a normal setting, will perform standard actions
        # for a normal setting such as checking is the envar is set externally
        self.add_setting(ENVAR_MAYA_VERSION, MAYA_VERSION)

        # now we can utilize that default setting to trigger the initialization,
        # or update, to the other paths/setting that are based off version
        self.maya_version = self.settings.MAYA_VERSION
        # even thought this ends up updating the internal version twice,
        # it keeps up from having to write additional code.
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
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

# now we can extend the environment specific to Maya
maya_config.add_setting(ENVAR_DCCSI_CONFIG_DCC_MAYA, DCCSI_CONFIG_DCC_MAYA)

# store the dccsi tools dcc maya location in settings for retreival
maya_config.add_setting(ENVAR_PATH_DCCSI_TOOLS_DCC_MAYA, Path(PATH_DCCSI_TOOLS_DCC_MAYA))

# store the dccsi tools root in settings
maya_config.add_setting(ENVAR_PATH_DCCSI_TOOLS, Path(PATH_DCCSI_TOOLS))

# setting this is specially cased in the MayaConfig, it will store the version
# in settings, then update the bin folder and some other related paths
# maya_config.maya_version = 2023  # this is just the default version
# leaving this comment here to document that feature, however setting it in
# that manner also disbaled checking if the envar is externally set

maya_config.add_setting(ENVAR_MAYA_PROJECT, Path(PATH_O3DE_PROJECT))

# technically there can be more then one path for these, so we should in the future
# refactor this to be handled specially with MayaConfig class object rather then
# a simple niave setting
maya_config.add_setting(ENVAR_DCCSI_MAYA_PLUG_IN_PATH, Path(DCCSI_MAYA_PLUG_IN_PATH))
maya_config.add_setting(ENVAR_DCCSI_MAYA_SHELF_PATH, Path(DCCSI_MAYA_SHELF_PATH))
maya_config.add_setting(ENVAR_DCCSI_MAYA_XBMLANGPATH, Path(DCCSI_MAYA_XBMLANGPATH))
maya_config.add_setting(ENVAR_DCCSI_MAYA_SCRIPT_PATH, Path(DCCSI_MAYA_SCRIPT_PATH), set_pythonpath = True)

# Add the non DCCSI_ maya specific envar versions
# we do these with another method such that they don't persist the maya enavr
# into our settings. These could become a path-list in the future and require a
# a refactor to extend the MayaConfig class with additional special halnding.
maya_config.add_path_to_envar(maya_config.settings.DCCSI_MAYA_PLUG_IN_PATH, ENVAR_MAYA_PLUG_IN_PATH)
maya_config.add_path_to_envar(maya_config.settings.DCCSI_MAYA_SHELF_PATH, ENVAR_MAYA_SHELF_PATH)
maya_config.add_path_to_envar(maya_config.settings.DCCSI_MAYA_XBMLANGPATH, ENVAR_XBMLANGPATH)
maya_config.add_path_to_envar(maya_config.settings.DCCSI_MAYA_SCRIPT_PATH, ENVAR_MAYA_SCRIPT_PATH)

# these are specifically ours and would not potentially be multi-path lists
maya_config.add_setting(ENVAR_DCCSI_MAYA_SCRIPT_MEL_PATH, Path(DCCSI_MAYA_SCRIPT_MEL_PATH))
maya_config.add_setting(ENVAR_DCCSI_MAYA_SCRIPT_PY_PATH, Path(DCCSI_MAYA_SCRIPT_PY_PATH))

# non-path settings
maya_config.add_setting(ENVAR_MAYA_DISABLE_CIP, MAYA_DISABLE_CIP)
maya_config.add_setting(ENVAR_MAYA_DISABLE_CER, MAYA_DISABLE_CER)
maya_config.add_setting(ENVAR_MAYA_DISABLE_CLIC_IPM, MAYA_DISABLE_CLIC_IPM)
maya_config.add_setting(ENVAR_MAYA_DISABLE_ADP, MAYA_DISABLE_ADP)
maya_config.add_setting(ENVAR_MAYA_NO_CONSOLE_WINDOW, str(MAYA_NO_CONSOLE_WINDOW))
maya_config.add_setting(ENVAR_MAYA_SHOW_OUTPUT_WINDOW, str(MAYA_SHOW_OUTPUT_WINDOW))
maya_config.add_setting(ENVAR_DCCSI_MAYA_SET_CALLBACKS, DCCSI_MAYA_SET_CALLBACKS)

# these are possibly windows only, so may need to be refactored in the future
maya_config.add_setting(ENVAR_MAYA_VP2_DEVICE_OVERRIDE, MAYA_VP2_DEVICE_OVERRIDE)
maya_config.add_setting(ENVAR_MAYA_OGS_DEVICE_OVERRIDE, MAYA_OGS_DEVICE_OVERRIDE)
# --- END -----------------------------------------------------------------

# always init defaults
settings = maya_config.get_config_settings()

# make sure the MAYA_SCRIPT_PATH is up front on search path
add_site_dir(maya_config.settings.DCCSI_MAYA_SCRIPT_PATH)

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
        settings.DCCSI_CONFIG_DCC_MAYA
        _LOGGER.info('Maya config is enabled')
    except:
        _LOGGER.error('Setting does not exist')

    _LOGGER.info(f'Exporting local settings: {PATH_DCCSI_TOOLS_DCC_MAYA_LOCAL_SETTINGS}')

    try:
        maya_config.export_settings(set_env=True, log_settings=True)
    except Exception as e:
        _LOGGER.error(f'{e}')
# --- END -----------------------------------------------------------------
