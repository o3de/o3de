
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
#
"""! Pyside2uic utilities

:file: < DCCsi >/azpy/shared/ui/puic_utils.py
:Status: Prototype
:Version: 0.0.1

Notice: This module requires the pyside2uic from PySide2-tools,
These are not installed by O3DE or the DCCsi directly.
See the READ.me in this pkgs folder for help.

URL: https://github.com/pyside/pyside2-tools
"""
from pathlib import Path
import logging as _logging
import xml.etree.ElementTree as xml  # Qt .ui files are xml
# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'azpy.shared.ui.puic_utils'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')
# -------------------------------------------------------------------------

from DccScriptingInterface.azpy.shared.ui import PATH_DCCSI_PYTHON_LIB

from pyside2tools import pyside2uic
