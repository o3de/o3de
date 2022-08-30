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
"""! O3DE DCCsi Tools package

:file: < DCCsi >/Tools/__init__.py
:Status: Prototype
:Version: 0.0.1

This init allows us to treat the Tools folder as a package.
"""
# -------------------------------------------------------------------------
# standard imports
import os
import site
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_PACKAGENAME = 'DCCsi.Tools'

__all__ = ['DCC',
           'IDE',
           'Python']  # to do: add others when they are set up
          #'Foo',

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# we need to set up basic access to the DCCsi
_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')

# turtles all the way down, the paths from there to here
from DccScriptingInterface import PATH_O3DE_TECHART_GEMS
from DccScriptingInterface import PATH_DCCSIG
# this makes sure nothing asserts higher up
from DccScriptingInterface.globals import *

from DccScriptingInterface.constants import ENVAR_PATH_DCCSI_TOOLS

# the path to this < dccsi >/Tools pkg
PATH_DCCSI_TOOLS = Path(_MODULE_PATH.parent)
# this allows the Tool location to be overriden in the external env
PATH_DCCSI_TOOLS = Path(os.getenv(ENVAR_PATH_DCCSI_TOOLS,
                                   PATH_DCCSI_TOOLS.as_posix()))
site.addsitedir(PATH_DCCSI_TOOLS.as_posix())
_LOGGER.debug(f'{ENVAR_PATH_DCCSI_TOOLS}: {PATH_DCCSI_TOOLS}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.config_utils import attach_debugger
from azpy import test_imports

# suggestion would be to turn this into a method to reduce boilerplate
# but where to put it that makes sense?
if DCCSI_DEV_MODE:
    # if dev mode, this will attemp to auto-attach the debugger
    # at the earliest possible point in this module
    attach_debugger(debugger_type=DCCSI_GDEBUGGER)

    _LOGGER.debug(f'Testing Imports from {_PACKAGENAME}')

    # If in dev mode and test is flagged this will force imports of __all__
    # although slower and verbose, this can help detect cyclical import
    # failure and other issues

    # the DCCSI_TESTS flag needs to be properly added in .bat env
    if DCCSI_TESTS:
        test_imports(_all=__all__,
                     _pkg=_PACKAGENAME,
                     _logger=_LOGGER)
# -------------------------------------------------------------------------
