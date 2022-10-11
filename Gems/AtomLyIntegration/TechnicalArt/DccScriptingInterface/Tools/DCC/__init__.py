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
<DCCsi>/Tools/DCC/__init__.py

This init allows us to treat the DCCsi Tools DCC folder as a package.
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
_PACKAGENAME = 'Tools.DCC'

__all__ = ['Blender',
           'Maya',
           'Substance']  # to do: add others when they are set up
          #'3dsMax',
          #'Houdini',
          #'Marmoset',
          # 'Foo',

_LOGGER = _logging.getLogger(_PACKAGENAME)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# set up access to this DCC folder as a pkg
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?

from Tools import PATH_DCCSIG

# suggest to remove a lot of this boilerplate imports and debug config
# from __init__'s in the future, there were put in an earlier version when
# some of the scaffolding wasn't fully in place. These settings can now be
# set in the env (*.bat files) and/or the settings.local.json to active
# and propogate.

# now we have access to the DCCsi code and azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG
from azpy.config_utils import attach_debugger

from Tools import PATH_DCCSI_TOOLS

PATH_DCCSI_TOOLS_DCC = Path(_MODULE_PATH.parent)
PATH_DCCSI_TOOLS_DCC = Path(os.getenv('PATH_DCCSI_TOOLS_DCC',
                                       PATH_DCCSI_TOOLS_DCC.as_posix()))
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
    # override loglevel if runnign debug
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
_LOGGER.debug(f'PATH_DCCSIG: {PATH_DCCSIG}')
_LOGGER.debug(f'PATH_DCCSI_TOOLS: {PATH_DCCSI_TOOLS}')
_LOGGER.debug(f'PATH_DCCSI_TOOLS_DCC: {PATH_DCCSI_TOOLS_DCC}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# dev mode will enable nested import tests
if _DCCSI_DEV_MODE:
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
