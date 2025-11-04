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
                                                           enable_o3de_pyside2=True,
                                                           set_env=True)

    try:
        import PySide2  # this triggers the PySide2 init
        from PySide2 import QtWidgets  # this requires DLLs to be built
    except ImportError as e:
        _LOGGER.error(f'Qt exception: {e}')
        _LOGGER.warning(f'Something is wrong with Qt/PySide imports')
        _LOGGER.warning(f'Make sure the O3DE engine is built')
        raise e

    from DccScriptingInterface.config import _DCCSI_LOCAL_SETTINGS

    # see if the pyside2tools are installed, if so add modules
    PYSIDE2_TOOLS = Path(_DCCSI_LOCAL_SETTINGS['PATH_DCCSI_PYTHON_LIB'], 'pyside2tools')
    try:
        PYSIDE2_TOOLS = PYSIDE2_TOOLS.resolve(strict=True)
        site.addsitedir(PYSIDE2_TOOLS.as_posix())
        import pyside2tools
        _LOGGER.info(f'pyside2tools installed: {pyside2tools}')
        _LOGGER.info(f'pyside2tools path: {PYSIDE2_TOOLS.as_posix()}')
    except Exception as e:
        PYSIDE2_TOOLS = None
        _LOGGER.warning(f'pyside2tools not installed.')

    if PYSIDE2_TOOLS:
        __all__.append('puic_utils')
