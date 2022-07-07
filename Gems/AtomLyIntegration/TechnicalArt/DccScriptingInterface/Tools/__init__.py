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
import site
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_PACKAGENAME = 'Tools'
_LOGGER = _logging.getLogger(_PACKAGENAME)

__all__ = ['DCC',
           'Python']  # to do: add others when they are set up
          #'Foo',
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# we need to set up basic access to the DCCsi
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?

_PATH_DCCSI_TOOLS = Path(_MODULE_PATH.parent)
_PATH_DCCSI_TOOLS = Path(os.getenv('PATH_DCCSI_TOOLS',
                                       _PATH_DCCSI_TOOLS.as_posix()))
site.addsitedir(_PATH_DCCSI_TOOLS.as_posix())

_PATH_DCCSIG = Path(_PATH_DCCSI_TOOLS.parent)
_PATH_DCCSIG = Path(os.getenv('PATH_DCCSIG', _PATH_DCCSIG.as_posix()))
site.addsitedir(_PATH_DCCSIG.as_posix())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# now we have access to the DCCsi code and azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG

#  global space
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

from azpy.config_utils import attach_debugger

# -------------------------------------------------------------------------
# message collection
_LOGGER.debug(f'Initializing: {_PACKAGENAME}')
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')
_LOGGER.debug(f'PATH_DCCSIG: {_PATH_DCCSIG}')
_LOGGER.debug(f'PATH_DCCSI_TOOLS: {_PATH_DCCSI_TOOLS}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
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
