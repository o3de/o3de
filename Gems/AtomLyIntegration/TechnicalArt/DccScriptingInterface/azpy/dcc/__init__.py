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
<DCCsi>//azpy/dcc/__init__.py

dcc is a sub-module of the azpy pure-python api.
"""
# -------------------------------------------------------------------------
# standard imports
import os
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.azpy import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.dcc'

__all__ = ['blender',
           'houdini',
           'marmoset',
           'max',
           'maya',
           'substance']

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
from DccScriptingInterface import STR_CROSSBAR
_LOGGER.debug(STR_CROSSBAR)

# set up access to this DCC folder as a pkg
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH.as_posix()))

from DccScriptingInterface import PATH_DCCSIG
_LOGGER.debug('PATH_DCCSIG: {}'.format(PATH_DCCSIG))

from DccScriptingInterface.azpy import PATH_DCCSI_AZPY
_LOGGER.debug('PATH_DCCSI_AZPY: {}'.format(PATH_DCCSI_AZPY))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from DccScriptingInterface.globals import *

if DCCSI_TESTS:
    # If in dev mode this will test imports of __all__
    from DccScriptingInterface.azpy.shared.utils.init import test_imports

    _LOGGER.info(STR_CROSSBAR)

    _LOGGER.debug('Testing Imports from {0}'.format(_PACKAGENAME))
    test_imports(__all__, _pkg=_PACKAGENAME)

    _LOGGER.info(STR_CROSSBAR)
# -------------------------------------------------------------------------
