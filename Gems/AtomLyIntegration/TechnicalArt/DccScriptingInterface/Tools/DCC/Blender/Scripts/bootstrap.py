#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
import os
import logging as _logging

from DccScriptingInterface.Tools.DCC.Blender import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.config'

from DccScriptingInterface.Tools.DCC.Blender import ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS
print(os.getenv(ENVAR_PATH_DCCSI_TOOLS_DCC_BLENDER_SCRIPTS, None))

# get the global dccsi state
from DccScriptingInterface.globals import DCCSI_GDEBUG
from DccScriptingInterface.globals import DCCSI_DEV_MODE

from azpy.constants import FRMT_LOG_LONG
_logging.basicConfig(level=_logging.DEBUG,
                     format=FRMT_LOG_LONG,
                    datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)

# auto-attach ide debugging at the earliest possible point in module
if DCCSI_DEV_MODE:
    import DccScriptingInterface.azpy.test.entry_test
    DccScriptingInterface.azpy.test.entry_test.connect_wing()

# check access by grabbing this config from import
_boot_msg = 'O3DE DccScriptingInterface BOOTSTRAP'
print(_boot_msg)
_LOGGER.debug(f'{_boot_msg}')

try:
    from dynaconf import settings
    from DccScriptingInterface.Tools.DCC.Blender.config import blender_config
except ImportError as e:
    print(f'ERROR: {e}')
    _LOGGER.exception(f'{e} , traceback =', exc_info=True)
    raise e

settings = blender_config.get_config_settings()

print('O3DE support, not implemented yet!')
