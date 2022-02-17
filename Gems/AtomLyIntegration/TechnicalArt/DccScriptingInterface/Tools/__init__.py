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
<DCCsi>/Tools/__init__.py

This init allows us to treat the Tools folder as a package.
"""
# -------------------------------------------------------------------------
# standard imports
import os
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
# global scope
_PACKAGENAME = 'Tools'

__all__ = ['DCC',
           'Python']  # to do: add others when they are set up

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug(f'Initializing: {_PACKAGENAME}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# set up access to this DCC folder as a pkg
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')
_PATH_DCCSI_TOOLS_DCC = Path(_MODULE_PATH.parent)
_PATH_DCCSI_TOOLS_DCC = Path(os.getenv('ATH_DCCSI_TOOLS_DCC', _PATH_DCCSI_TOOLS_DCC.as_posix()))
_LOGGER.debug(f'PATH_DCCSI_TOOLS_DCC: {_PATH_DCCSI_TOOLS_DCC}')
# -------------------------------------------------------------------------