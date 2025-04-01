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
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface import _PACKAGENAME, STR_CROSSBAR
_PACKAGENAME = f'{_PACKAGENAME}.Tools'

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
from DccScriptingInterface import add_site_dir

# this makes sure nothing asserts higher up
from DccScriptingInterface.globals import *

from DccScriptingInterface.constants import ENVAR_PATH_DCCSI_TOOLS

# the path to this < dccsi >/Tools pkg
PATH_DCCSI_TOOLS = Path(_MODULE_PATH.parent)
# this allows the Tool location to be overriden in the external env
PATH_DCCSI_TOOLS = Path(os.getenv(ENVAR_PATH_DCCSI_TOOLS,
                                   PATH_DCCSI_TOOLS.as_posix()))

add_site_dir(PATH_DCCSI_TOOLS.as_posix())
_LOGGER.debug(f'{ENVAR_PATH_DCCSI_TOOLS}: {PATH_DCCSI_TOOLS}')
_LOGGER.debug(STR_CROSSBAR)
# -------------------------------------------------------------------------
