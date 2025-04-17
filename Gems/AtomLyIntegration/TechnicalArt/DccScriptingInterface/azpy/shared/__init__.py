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
<DCCsi>/azpy/shared/__init__.py

DCCsi package for shared packages.
"""
# standard imports
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.azpy import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.shared'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug(f'Initializing: {_PACKAGENAME}')

__all__ = ['common', 'ui', 'utils']

from DccScriptingInterface.azpy.env_bool import env_bool
from DccScriptingInterface.azpy.constants import ENVAR_DCCSI_GDEBUG
from DccScriptingInterface.globals import *

_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')

