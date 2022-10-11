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
_PACKAGENAME = 'azpy.shared'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug(f'Initializing: {_PACKAGENAME}')

__all__ = ['common', 'ui', 'utils']

from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)

_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.shared.utils.init import test_imports
if _DCCSI_GDEBUG:
    # If in dev mode this will test imports of __all__
    _LOGGER.debug(f'Testing Imports from {_PACKAGENAME}')
    test_imports(_all=__all__,_pkg=_PACKAGENAME,_logger=_LOGGER)
# -------------------------------------------------------------------------
