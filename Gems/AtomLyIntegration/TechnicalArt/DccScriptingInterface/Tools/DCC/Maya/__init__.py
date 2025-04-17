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
"""! TThis init allows us to treat Maya as a DCCsi tools python package

:file: DccScriptingInterface\\Tools\\DCC\\Maya\\__init__.py
:Status: Prototype
:Version: 0.0.1
:Notice:
"""
# -------------------------------------------------------------------------
# standard imports
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
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# set up access to this DCC folder as a pkg
_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')

from DccScriptingInterface import add_site_dir

from DccScriptingInterface import STR_CROSSBAR
from DccScriptingInterface import SETTINGS_FILE_SLUG
from DccScriptingInterface import LOCAL_SETTINGS_FILE_SLUG

# last two parents
from DccScriptingInterface.Tools import PATH_DCCSI_TOOLS
from DccScriptingInterface.Tools.DCC import PATH_DCCSI_TOOLS_DCC

from DccScriptingInterface.globals import *

# our dccsi location for <DCCsi>\Tools\DCC\Maya
ENVAR_PATH_DCCSI_TOOLS_DCC_MAYA = "PATH_DCCSI_TOOLS_DCC_MAYA"

# the path to this < dccsi >/Tools/DCC pkg
PATH_DCCSI_TOOLS_DCC_MAYA = Path(_MODULE_PATH.parent)
PATH_DCCSI_TOOLS_DCC_MAYA = Path(os.getenv(ENVAR_PATH_DCCSI_TOOLS_DCC_MAYA,
                                           PATH_DCCSI_TOOLS_DCC_MAYA.as_posix()))
add_site_dir(PATH_DCCSI_TOOLS_DCC_MAYA.as_posix())
_LOGGER.debug(f'{ENVAR_PATH_DCCSI_TOOLS_DCC_MAYA}: {PATH_DCCSI_TOOLS_DCC_MAYA}')
_LOGGER.debug(STR_CROSSBAR)
# -------------------------------------------------------------------------
