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
"""Treat Adobe Substance3D Deisgner, DCC app integration as a package

    < DCCsi >/Tools/DCC/Substance/__init__.py

:Status: Prototype
:Version: 0.0.2
:Substance3D Version: Adobe ver 12.x+

Note: other versions may work but have not been tested.
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
_PACKAGENAME = 'Tools.DCC.Substance'

__all__ = ['config',
           'constants',
           'start']

_LOGGER = _logging.getLogger(_PACKAGENAME)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# This boilerplate code is here as a developer convenience, simply so I
# can run this module as an entrypoint, perform some local tests, and/or
# propogate values easily into sub-modules with less boilerplate.

# set up access to this Substance folder as a pkg
_MODULE_PATH = Path(__file__)
_DCCSI_TOOLS_SUBSTANCE_PATH = Path(_MODULE_PATH.parent)

from Tools import PATH_DCCSIG

# now we have access to the DCCsi code and azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG
from azpy.config_utils import attach_debugger

from Tools import PATH_DCCSI_TOOLS
from Tools.DCC import PATH_DCCSI_TOOLS_DCC

_DCCSI_TOOLS_SUBSTANCE = Path(_MODULE_PATH.parent)
_DCCSI_TOOLS_SUBSTANCE = Path(os.getenv('DCCSI_TOOLS_SUBSTANCE',
                                        _DCCSI_TOOLS_SUBSTANCE.as_posix()))
site.addsitedir(PATH_DCCSI_TOOLS_DCC.as_posix())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# reconfigure for debug and dev mode
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')

# default loglevel to info unless set
_DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))
if _DCCSI_GDEBUG:
    # override loglevel if running debug
    _DCCSI_LOGLEVEL = _logging.DEBUG
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                        format=FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')
    _LOGGER = _logging.getLogger(_PACKAGENAME)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# message collection
_LOGGER.debug(f'Initializing: {_PACKAGENAME}')
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')
_LOGGER.debug(f'DCCSI_TOOLS_SUBSTANCE_PATH: {_DCCSI_TOOLS_SUBSTANCE_PATH}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
if _DCCSI_DEV_MODE:
    attach_debugger(debugger_type=_DCCSI_GDEBUGGER)

    from azpy.shared.utils.init import test_imports
    # If in dev mode this will test imports of __all__
    _LOGGER.debug(f'Testing Imports from {_PACKAGENAME}')
    test_imports(_all=__all__,_pkg=_PACKAGENAME,_logger=_LOGGER)
# -------------------------------------------------------------------------



###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run as main, perform debug and tests"""
    pass
