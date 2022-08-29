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

_PACKAGENAME = 'azpy.shared.ui'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

__all__ = ['utils', 'samples']

# turtles all the way down, the paths from there to here
from DccScriptingInterface import PATH_O3DE_TECHART_GEMS
from DccScriptingInterface import PATH_DCCSIG

import azpy.config_utils
PATH_DCCSI_PYTHON_LIB = azpy.config_utils.bootstrap_dccsi_py_libs(PATH_DCCSIG)
site.addsitedir(PATH_DCCSI_PYTHON_LIB.as_posix())

# see if the pyside2tools are installed, if so add modules
PYSIDE2_TOOLS = Path(PATH_DCCSI_PYTHON_LIB, 'pyside2tools')
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

# # -------------------------------------------------------------------------
# if _DCCSI_DEV_MODE:
#     # If in dev mode this will test imports of __all__
#     from azpy import test_imports
#     _LOGGER.debug('Testing Imports from {0}'.format(_PACKAGENAME))
#     test_imports(__all__,
#                  _pkg=_PACKAGENAME,
#                  _logger=_LOGGER)
# # -------------------------------------------------------------------------

del _LOGGER
