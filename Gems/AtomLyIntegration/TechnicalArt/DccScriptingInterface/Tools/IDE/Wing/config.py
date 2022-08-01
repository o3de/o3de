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

# this import will ensure proper ode and dccsi access, and we use this path
import DccScriptingInterface.config as dccsi_core_config

# i.e. this can operate standalone, allowing wing configuration via cli
from DccScriptingInterface.Tools.IDE.Wing import PATH_DCCSI_TOOLS_IDE_WING
# -------------------------------------------------------------------------
# local dccsi imports
# this accesses common global state, e.g. DCCSI_GDEBUG (is True or False)
from DccScriptingInterface.globals import *
# this will auto-attach ide debugging at the earliest possible point
from azpy.config_utils import attach_debugger
if DCCSI_DEV_MODE:
    attach_debugger(debugger_type=DCCSI_GDEBUGGER)

# now standalone we can validate the config, env, settings.
_SETTINGS = dccsi_core_config.get_config_settings(enable_o3de_python=False,
                                                  enable_o3de_pyside2=False,
                                                  set_env=True)

# -------------------------------------------------------------------------
def get_config_settings():

    print('Module Not Implemented')

    return None
# --- END -----------------------------------------------------------------
