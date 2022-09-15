#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

"""! @brief

<DCCsi>/azpy/test/__init__.py

A lightweight entrytest and debugging module (Wing only currently)
"""
# -------------------------------------------------------------------------
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.azpy import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.test'

__all__ = ['entry_test']
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')

__all__ = ['entry_test']
