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
"""! This module manages the dynamic config and settings for bootstrapping
Wing Pro 8 IDE integration with o3de inter-op, scripts, extensions, etc.

:file: < DCCsi >/Tools/IDE/Wing/config.py
:Status: Prototype
:Version: 0.0.1
:Future: is unknown
:Notice:
"""
# -------------------------------------------------------------------------
# standard imports
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'DCCsi.Tools.IDE.Wing.config'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

# ensure dccsi and o3de core access
# in a future iteration it is suggested that the core config
# be rewritten from ConfigClass, then WingConfig inherits core
import DccScriptingInterface.config as dccsi_core_config

# this will initialize the core dynamic config, settings and env
# if the Qt/PySide envars are set, it can cause some Qt based apps to fail
# wing happens to be one of them, this is disabled for wing\start.py to perform
_settings_core = dccsi_core_config.get_config_settings(enable_o3de_python=True,
                                                       enable_o3de_pyside2=False,
                                                       set_env=True)

# from within wing, setting the Qt/PySide2 envars before running
# another script/tool performs fine, as the interpreter is a subprocess

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

# can run local tests
if DCCSI_TESTS: # from DccScriptingInterface.globals
    # this will validate pyside bootstrapping
    foo = dccsi_core_config.test_pyside2(exit = False)
    pass

# this is the root path for the wing pkg
from DccScriptingInterface.Tools.IDE.Wing import PATH_DCCSI_TOOLS_IDE_WING
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# now we build the wing config class
from DccScriptingInterface.azpy.config_class import ConfigClass

# For a DCC tool like Maya, where we may have additional tools that need
# their own config/settings, the MayaConfig could inherit from the
# CoreConfig (which is a ConfigClass), then the tool could have it's own
# MyMayaToolConfig that inherits from MayaConfig and extends it further.

# wing does have an API, theoretically we could create tools built on top
# of wing like we do with DCC tools ... but we do not yet have the need
# so the more simple approach is taken here for now, a ConfigClass object

# but it is suggested that when the core <dccsi>\config.py is re-written
# as a ConfigClass, that the WingConfig inherits from that instead
class WingConfig(ConfigClass):
    """doc string"""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        _LOGGER.info(f'Initializing: {self.get_classname()}')

# build config
wing_config = WingConfig(config_name='dccsi_ide_wing', auto_set=True)

# a managed setting to track the eing config is enabled
from Tools.IDE.Wing.constants import ENVAR_DCCSI_CONFIG_IDE_WING
wing_config.add_setting(ENVAR_DCCSI_CONFIG_IDE_WING, True)

# a managed envar for wing version
from Tools.IDE.Wing.constants import ENVAR_DCCSI_WING_VERSION_MAJOR
from Tools.IDE.Wing.constants import SLUG_DCCSI_WING_VERSION_MAJOR
wing_config.add_setting(ENVAR_DCCSI_WING_VERSION_MAJOR,
                        SLUG_DCCSI_WING_VERSION_MAJOR)

# a managed envar setting for WINGHOME (install location)
from Tools.IDE.Wing.constants import ENVAR_WINGHOME
from Tools.IDE.Wing.constants import PATH_WINGHOME
PATH_WINGHOME = Path(PATH_WINGHOME).resolve()
wing_config.add_setting(ENVAR_WINGHOME,
                        PATH_WINGHOME.as_posix(),
                        set_sys_path=True)

# a managed envar for the wing bin folder
from Tools.IDE.Wing.constants import ENVAR_WINGHOME_BIN
from Tools.IDE.Wing.constants import PATH_WINGHOME_BIN
PATH_WINGHOME_BIN = Path(PATH_WINGHOME_BIN).resolve()
wing_config.add_setting(ENVAR_WINGHOME_BIN,
                        PATH_WINGHOME_BIN.as_posix(),
                        set_sys_path=True)

# a managed envar to the wing exe
from Tools.IDE.Wing.constants import ENVAR_WING_EXE
from Tools.IDE.Wing.constants import PATH_WINGHOME_BIN_EXE
PATH_WINGHOME_BIN_EXE = Path(PATH_WINGHOME_BIN_EXE).resolve()
wing_config.add_setting(ENVAR_WING_EXE,
                        PATH_WINGHOME_BIN_EXE.as_posix())


# a managed envar setting for the wing project file
from Tools.IDE.Wing.constants import ENVAR_WING_PROJ
from Tools.IDE.Wing.constants import PATH_DCCSI_TOOLS_IDE_WING_PROJ
PATH_DCCSI_TOOLS_IDE_WING_PROJ = Path(PATH_DCCSI_TOOLS_IDE_WING_PROJ).resolve()
wing_config.add_setting(ENVAR_WING_PROJ,
                        PATH_DCCSI_TOOLS_IDE_WING_PROJ.as_posix())

# initialize configs for DCC tools we want to develop with
# so we get access to their python interpreter, etc.
# it is suggested the in the future, there be a project setreg for wing
# whose settings describe which DCC tools to activate on start

# blender config
import DccScriptingInterface.Tools.DCC.Blender.config as blender_config
blender_settings = blender_config.get_config_settings()

# prefix for defining IDE interpreters 'DCCSI_PY_'
wing_config.add_setting('DCCSI_PY_BLENDER',
                        blender_settings.DCCSI_BLENDER_PY_EXE,
                        set_envar=True)

# maya config
# not implemented yet

# other DCC tools
# also not yet implemented
# -------------------------------------------------------------------------

###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as a standalone cli script for testing/debugging"""

    # this should hit this modules location and load wing settings
    settings = wing_config.get_settings(set_env=True)

    try:
        settings.DCCSI_CONFIG_IDE_WING
        _LOGGER.info('Wing IDE config is enabled')
    except:
        _LOGGER.error('Setting does not exist')

    _LOGGER.info(f'{ENVAR_DCCSI_WING_VERSION_MAJOR} is: {settings.DCCSI_WING_VERSION_MAJOR}')
    _LOGGER.info(f'{ENVAR_WINGHOME} is: {settings.WINGHOME}')
    _LOGGER.info(f'{ENVAR_WING_EXE} is: {settings.WING_EXE}')
    _LOGGER.info(f'{ENVAR_WING_PROJ} is: {settings.WING_PROJ}')
# --- END -----------------------------------------------------------------
