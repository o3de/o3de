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
_PACKAGENAME = 'Tools.DCC.Maya'

__all__ = ['config',
           'constants',
           'setup',
           'start']

_LOGGER = _logging.getLogger(_PACKAGENAME)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Maya is frozen
#_MODULE_PATH = Path(__file__)
# https://tinyurl.com/y49t3zzn
# module path when frozen
_MODULE_PATH = Path(os.path.abspath(inspect.getfile(inspect.currentframe())))
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH))
_PATH_DCCSI_TOOLS_MAYA = Path(_MODULE_PATH.parent)

# we need to set up basic access to the DCCsi
_PATH_DCCSI_TOOLS_DCC = Path(_PATH_DCCSI_TOOLS_MAYA.parent)
_PATH_DCCSI_TOOLS_DCC = Path(os.getenv('PATH_DCCSI_TOOLS_DCC', _PATH_DCCSI_TOOLS_DCC.as_posix()))
site.addsitedir(_PATH_DCCSI_TOOLS_DCC.as_posix())

# we need to set up basic access to the DCCsi
_PATH_DCCSI_TOOLS = Path(_PATH_DCCSI_TOOLS_DCC.parent)
_PATH_DCCSI_TOOLS = Path(os.getenv('PATH_DCCSI_TOOLS', _PATH_DCCSI_TOOLS.as_posix()))

# we need to set up basic access to the DCCsi
_PATH_DCCSIG = Path(_PATH_DCCSI_TOOLS.parent)
_PATH_DCCSIG = Path(os.getenv('PATH_DCCSIG', _PATH_DCCSIG.as_posix()))
site.addsitedir(_PATH_DCCSIG.as_posix())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# now we have access to the DCCsi code and azpy
import azpy.test.entry_test
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

from azpy.constants import STR_CROSSBAR
_LOGGER.info(STR_CROSSBAR)
_LOGGER.info('Initializing: {0}.'.format({_PACKAGENAME}))

if _DCCSI_DEV_MODE:
    azpy.test.entry_test.connect_wing()

# message collection
_LOGGER.debug(f'Initializing: {_PACKAGENAME}')
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')
_LOGGER.debug(f'PATH_DCCSI_TOOLS_MAYA: {_PATH_DCCSI_TOOLS_MAYA}')
_LOGGER.debug(f'PATH_DCCSIG: {_PATH_DCCSIG}')
_LOGGER.debug(f'PATH_DCCSI_TOOLS_DCC: {_PATH_DCCSI_TOOLS_DCC}')
_LOGGER.debug(f'PATH_DCCSI_TOOLS: {_PATH_DCCSI_TOOLS}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_TESTS

#  global space
_DCCSI_TESTS = env_bool(ENVAR_DCCSI_TESTS, False)

if _DCCSI_TESTS:
    # If in dev mode this will test imports of __all__
    from azpy import test_imports
    
    _LOGGER.info(STR_CROSSBAR)
    
    _LOGGER.debug('Testing Imports from {0}'.format(_PACKAGENAME))
    test_imports(__all__,
                 _pkg=_PACKAGENAME)
    
    _LOGGER.info(STR_CROSSBAR)
# -------------------------------------------------------------------------



###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run as main, perform debug and tests"""
    pass
    