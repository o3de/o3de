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
DCCsi/Tools/DCC/Maya/__init__.py

This init allows us to treat Maya setup as a DCCsi tools python package
"""
# -------------------------------------------------------------------------
# standard imports
import os
import site
import inspect
import traceback
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.Tools.DCC import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.Maya'

__all__ = ['config',
           'constants',
           'setup',
           'start',
           'Scripts']

_LOGGER = _logging.getLogger(_PACKAGENAME)


# set up access to this DCC folder as a pkg
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?

from DccScriptingInterface import STR_CROSSBAR
from DccScriptingInterface import PATH_O3DE_TECHART_GEMS
from DccScriptingInterface import PATH_DCCSIG
from DccScriptingInterface.Tools import PATH_DCCSI_TOOLS
from DccScriptingInterface.globals import *


# dev mode will enable nested import tests
if DCCSI_DEV_MODE:
    from DccScriptingInterface.azpy.shared.utils.init import test_imports
    # If in dev mode this will test imports of __all__
    _LOGGER.debug(f'Testing Imports from {_PACKAGENAME}')
    test_imports(_all=__all__, _pkg=_PACKAGENAME, _logger=_LOGGER)
