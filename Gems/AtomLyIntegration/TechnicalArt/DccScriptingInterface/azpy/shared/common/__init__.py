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
"""! @brief
<DCCsi>/azpy/shared/common/__init__.py
"""
# standard imports
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.azpy.shared import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.common'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug(f'Initializing: {_PACKAGENAME}')

__all__ = ['core_utils', 'envar_utils']

from DccScriptingInterface.globals import *

_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')

