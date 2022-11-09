#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! This module manages the dynamic config and settings for bootstrapping
Blender DCC app integration with o3de inter-op, scripts, extensions, etc.

:file: DccScriptingInterface\\Tools\\DCC\\Blender\\config.py
:Status: Prototype
:Version: 0.0.1
:Future: is unknown
:Notice:
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
from DccScriptingInterface.Tools.DCC.Blender import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.config'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

# ensure dccsi and o3de core access
# in a future iteration it is suggested that the core config
# be rewritten from ConfigClass, then BlenderConfig inherits core
import DccScriptingInterface.config as dccsi_core_config

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
from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER
from DccScriptingInterface.Tools.DCC.Blender.constants import PATH_DCCSI_TOOLS_DCC_BLENDER

# blender settings.json
from DccScriptingInterface.Tools.DCC.Blender.constants import PATH_DCCSI_TOOLS_DCC_BLENDER_SETTINGS
# blender settings.local.json
from DccScriptingInterface.Tools.DCC.Blender.constants import PATH_DCCSI_TOOLS_DCC_BLENDER_LOCAL_SETTINGS
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# now we build the wing config class
from DccScriptingInterface.azpy.config_class import ConfigClass

# blender_config is a class object of BlenderConfig
# BlenderConfig is a child class of ConfigClass
class BlenderConfig(ConfigClass):
    """Extend ConfigClass with new blender functionality"""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        _LOGGER.info(f'Initializing: {self.get_classname()}')

# build config object
blender_config = BlenderConfig(config_name='dccsi_dcc_blender',
                               settings_filepath = PATH_DCCSI_TOOLS_DCC_BLENDER_SETTINGS,
                               settings_local_filepath = PATH_DCCSI_TOOLS_DCC_BLENDER_LOCAL_SETTINGS,
                               auto_set=True)

# in another module someone could work this way
# from DccScriptingInterface.Tools.IDE.Wing.config import wing_config
# settings = wing_config.get_settings(set_env=True)
# or
# if wing_config.settings.THIS_SETTING: do this

# now we can extend the environment specific to Blender
# start by grabbing the constants we want to work with as envars
# a managed setting to track the wing config is enabled
from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_DCCSI_CONFIG_DCC_BLENDER
blender_config.add_setting(ENVAR_DCCSI_CONFIG_DCC_BLENDER, True)

from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_TOOLS
from DccScriptingInterface.Tools.DCC.Blender import PATH_DCCSI_TOOLS
PATH_DCCSI_TOOLS = Path(PATH_DCCSI_TOOLS).resolve()
blender_config.add_setting(ENVAR_PATH_DCCSI_TOOLS,
                           PATH_DCCSI_TOOLS.as_posix())

from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER
from DccScriptingInterface.Tools.DCC.Blender.constants import PATH_DCCSI_TOOLS_DCC_BLENDER
PATH_DCCSI_TOOLS_DCC_BLENDER = Path(PATH_DCCSI_TOOLS_DCC_BLENDER).resolve()
blender_config.add_setting(ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER,
                           PATH_DCCSI_TOOLS_DCC_BLENDER.as_posix())

from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS
from DccScriptingInterface.Tools.DCC.Blender.constants import PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS
PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS = Path(PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS).resolve()
blender_config.add_setting(ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS,
                           PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS.as_posix(),
                           set_sys_path=True,
                           set_pythonpath=True)

from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_BLENDER_USER_SCRIPTS
blender_config.add_setting(ENVAR_BLENDER_USER_SCRIPTS,
                           PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS.as_posix(),
                           set_sys_path=True,
                           set_pythonpath=True)

from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_DCCSI_BLENDER_VERSION
from DccScriptingInterface.Tools.DCC.Blender.constants import SLUG_DCCSI_BLENDER_VERSION
blender_config.add_setting(ENVAR_DCCSI_BLENDER_VERSION,
                           SLUG_DCCSI_BLENDER_VERSION)

from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_DCCSI_BLENDER_LOCATION
from DccScriptingInterface.Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_LOCATION
PATH_DCCSI_BLENDER_LOCATION = Path(PATH_DCCSI_BLENDER_LOCATION).resolve()
blender_config.add_setting(ENVAR_DCCSI_BLENDER_LOCATION,
                           PATH_DCCSI_BLENDER_LOCATION.as_posix(),
                           set_sys_path=True)

from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_BLENDER_EXE
from DccScriptingInterface.Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_EXE
PATH_DCCSI_BLENDER_EXE = Path(PATH_DCCSI_BLENDER_EXE).resolve()
blender_config.add_setting(ENVAR_PATH_DCCSI_BLENDER_EXE,
                           PATH_DCCSI_BLENDER_EXE.as_posix())

from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_DCCSI_BLENDER_LAUNCHER_EXE
from DccScriptingInterface.Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_LAUNCHER_EXE
PATH_DCCSI_BLENDER_LAUNCHER_EXE = Path(PATH_DCCSI_BLENDER_LAUNCHER_EXE).resolve()
blender_config.add_setting(ENVAR_DCCSI_BLENDER_LAUNCHER_EXE,
                           PATH_DCCSI_BLENDER_LAUNCHER_EXE.as_posix())

from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_DCCSI_BLENDER_PYTHON_LOC
from DccScriptingInterface.Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_PYTHON_LOC
PATH_DCCSI_BLENDER_PYTHON_LOC = Path(PATH_DCCSI_BLENDER_PYTHON_LOC).resolve()
blender_config.add_setting(ENVAR_DCCSI_BLENDER_PYTHON_LOC,
                           PATH_DCCSI_BLENDER_PYTHON_LOC.as_posix(),
                           set_sys_path=True)

from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_DCCSI_BLENDER_PY_EXE
from DccScriptingInterface.Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_PY_EXE
PATH_DCCSI_BLENDER_PY_EXE = Path(PATH_DCCSI_BLENDER_PY_EXE).resolve()
blender_config.add_setting(ENVAR_DCCSI_BLENDER_PY_EXE,
                           PATH_DCCSI_BLENDER_PY_EXE.as_posix())

from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_PATH_DCCSI_BLENDER_BOOTSTRAP
from DccScriptingInterface.Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_BOOTSTRAP
PATH_DCCSI_BLENDER_BOOTSTRAP = Path(PATH_DCCSI_BLENDER_BOOTSTRAP).resolve()
blender_config.add_setting(ENVAR_PATH_DCCSI_BLENDER_BOOTSTRAP,
                           PATH_DCCSI_BLENDER_BOOTSTRAP.as_posix())

from DccScriptingInterface.Tools.DCC.Blender.constants import ENVAR_URL_DCCSI_BLENDER_WIKI
from DccScriptingInterface.Tools.DCC.Blender.constants import URL_DCCSI_BLENDER_WIKI
blender_config.add_setting(ENVAR_URL_DCCSI_BLENDER_WIKI,
                           str(URL_DCCSI_BLENDER_WIKI))
# --- END -----------------------------------------------------------------

settings = blender_config.get_config_settings()

_MODULE_END = timeit.default_timer() - _MODULE_START
_LOGGER.debug(f'{_MODULENAME} took: {_MODULE_END} sec')
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone cli script for testing/debugging"""

    # this should hit this modules location and load wing settings
    settings = blender_config.get_config_settings()

    try:
        settings.CONFIG_DCC_BLENDER
        _LOGGER.info('Blender config is enabled')
    except:
        _LOGGER.error('Setting does not exist')

    _LOGGER.info(f'Exporting local settings: {PATH_DCCSI_TOOLS_DCC_BLENDER_LOCAL_SETTINGS}')

    try:
        blender_config.export_settings(set_env=True,
                                       log_settings=True)
    except Exception as e:
        _LOGGER.error(f'{e}')
# --- END -----------------------------------------------------------------
