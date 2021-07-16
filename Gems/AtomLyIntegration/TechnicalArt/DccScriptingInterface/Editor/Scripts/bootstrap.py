# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------
"""This module is for use in boostrapping the DccScriptingInterface Gem
with Lumberyard. Note: this boostrap is only designed fo be py3 compatible.
If you need DCCsi access in py27 (Autodesk Maya for instance) you may need
to implement your own boostrapper module. Currently this is boostrapped
from add_dccsi.py, as a temporty measure related to this Jira:
SPEC-2581"""
# standard imports
import sys
import os
import site
import importlib.util
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# we don't use dynaconf setting here as we might not yet have access
# to that site-dir.

_MODULE = 'DCCsi.bootstrap'

# we need to set up basic access to the DCCsi
_MODULE_PATH = os.path.realpath(__file__)  # To Do: what if frozen?
_DCCSIG_PATH = os.path.normpath(os.path.join(_MODULE_PATH, '../../..'))
_DCCSIG_PATH = os.getenv('DCCSIG_PATH', _DCCSIG_PATH)
site.addsitedir(_DCCSIG_PATH)

# we can get basic access to the DCCsi.azpy api now
import azpy

# early attach WingIDE debugger (can refactor to include other IDEs later)
while 0:  # flag on to attemp to connect wingIDE debugger
    from azpy.env_bool import env_bool
    if not env_bool('DCCSI_DEBUGGER_ATTACHED', False):
        # if not already attached lets do it here
        from azpy.test.entry_test import connect_wing
        foo = connect_wing()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# settings.setenv()  # doing this will add the additional DYNACONF_ envars
def get_dccsi_config(DCCSIG_PATH=_DCCSIG_PATH):
    """Convenience method to set and retreive settings directly from module."""
    
    # we can go ahead and just make sure the the DCCsi env is set
    # config is SO generic this ensures we are importing a specific one
    _spec_dccsi_config = importlib.util.spec_from_file_location("dccsi.config",
                                                                Path(DCCSIG_PATH,
                                                                     "config.py"))
    _dccsi_config = importlib.util.module_from_spec(_spec_dccsi_config)
    _spec_dccsi_config.loader.exec_module(_dccsi_config)

    return _dccsi_config
# -------------------------------------------------------------------------

# set and retreive the base settings on import
config = get_dccsi_config()
settings = config.get_config_settings()
# done with basic setup
# --- END -----------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""

    _G_DEBUG = False
    _G_TEST_PYSIDE = False
    
    _config = get_dccsi_config()
    _settings = config.get_config_settings()

    _log_level = int(_settings.DCCSI_LOGLEVEL)
    if _G_DEBUG:
        _log_level = int(10)  # force debug level
    _LOGGER = azpy.initialize_logger(_MODULE,
                                     log_to_file=True,
                                     default_log_level=_log_level)
    
    # we can now grab values from the DCCsi.config.py dynamic env settings
    # the rest of this block is basic debug testing the dynamic settings at boot
    _LOGGER.info(f'Running module: {_MODULE}')
    _LOGGER.info(f'DCCSIG_PATH: {_settings.DCCSIG_PATH}')
    _LOGGER.info(f'DCCSI_G_DEBUG: {_settings.DCCSI_GDEBUG}')
    _LOGGER.info(f'DCCSI_DEV_MODE: {_settings.DCCSI_DEV_MODE}')

    _LOGGER.info(f'OS_FOLDER: {_settings.OS_FOLDER}')
    _LOGGER.info(f'LY_PROJECT: {_settings.LY_PROJECT}')
    _LOGGER.info(f'LY_PROJECT_PATH: {_settings.LY_PROJECT_PATH}')
    _LOGGER.info(f'LY_DEV: {_settings.LY_DEV}')
    _LOGGER.info(f'LY_BUILD_PATH: {_settings.LY_BUILD_PATH}')
    _LOGGER.info(f'LY_BIN_PATH: {_settings.LY_BIN_PATH}')
    
    _LOGGER.info(f'DCCSIG_PATH: {_settings.DCCSIG_PATH}')
    _LOGGER.info(f'DCCSI_PYTHON_LIB_PATH: {_settings.DCCSI_PYTHON_LIB_PATH}')
    _LOGGER.info(f'DDCCSI_PY_BASE: {_settings.DDCCSI_PY_BASE}')

    if _G_TEST_PYSIDE:
        try:
            import PySide2
        except:
            # set up Qt/PySide2 access and test
            _settings = _config.get_config_settings(setup_ly_pyside=True)
            import PySide2

        _LOGGER.info(f'PySide2: {PySide2}')
        _LOGGER.info(f'LY_BIN_PATH: {_settings.LY_BIN_PATH}')
        _LOGGER.info(f'QT_PLUGIN_PATH: {_settings.QT_PLUGIN_PATH}')
        _LOGGER.info(f'QT_QPA_PLATFORM_PLUGIN_PATH: {_settings.QT_QPA_PLATFORM_PLUGIN_PATH}')
        
        _config.test_pyside2()
# --- END -----------------------------------------------------------------
