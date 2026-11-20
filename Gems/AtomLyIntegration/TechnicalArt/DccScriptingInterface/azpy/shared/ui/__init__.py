#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

"""! This is a ui pkg for O3DE, DccScriptingInterface Gem

:file: DccScriptingInterface\\azpy\\shared\\ui\\__init__.py
"""
import site
from pathlib import Path
import logging as _logging

from DccScriptingInterface.azpy.shared import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.ui'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

__all__ = ['utils', 'samples']

from DccScriptingInterface.globals import *

DCCSI_TEST_PYSIDE = False

# turtles all the way down, the paths from there to here
from DccScriptingInterface import PATH_O3DE_TECHART_GEMS
from DccScriptingInterface import PATH_DCCSIG

if DCCSI_TEST_PYSIDE:
    import DccScriptingInterface.config as dccsi_core_config
    settings_core = dccsi_core_config.get_config_settings(enable_o3de_python=False,
                                                           enable_o3de_pyside=True,
                                                           set_env=True)

    try:
        import PySide6  # this triggers the PySide init
        from PySide6 import QtWidgets  # this requires DLLs to be built
    except ImportError as e:
        _LOGGER.error(f'Qt exception: {e}')
        _LOGGER.warning(f'Something is wrong with Qt/PySide imports')
        _LOGGER.warning(f'Make sure the O3DE engine is built')
        raise e
