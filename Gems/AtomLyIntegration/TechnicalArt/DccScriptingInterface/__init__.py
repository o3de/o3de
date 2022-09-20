# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# --------------------------------------------------------------------------
"""! @brief

<DCCsi>/__init__.py

Allow DccScriptingInterface Gem to be the parent python pkg
"""
dccsi_info = {
    'name': 'O3DE_DCCSI_GEM',
    "description": "DccScriptingInterface",
    'status': 'prototype',
    'version': (0, 0, 1),
    'platforms': ({
        'include': ('win'),   # windows
        'exclude': ('darwin', # mac
                    'linux')  # linux, linux_x64
    }),
    "doc_url": "https://github.com/o3de/o3de/blob/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/readme.md"
}
# -------------------------------------------------------------------------
# standard imports
import os
import sys
import site
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------
# global scope
_PACKAGENAME = 'DCCsi'

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

__all__ = ['globals', # global state module
           'config', # dccsi core config.py
           'constants', # global dccsi constants
           'foundation', # set up dependancy pkgs for DCC tools
           'return_sys_version', # util
           'azpy', # shared pure python api
           'Editor', # O3DE editor scripts
           'Tools' # DCC and IDE tool integrations
           ]

# we need to set up basic access to the DCCsi
_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')

# add gems parent, dccsi lives under:
#  < o3de >\Gems\AtomLyIntegration\TechnicalArt
PATH_O3DE_TECHART_GEMS = _MODULE_PATH.parents[1].resolve()
sys.path.append(PATH_O3DE_TECHART_GEMS)
site.addsitedir(PATH_O3DE_TECHART_GEMS)

# there may be other TechArt gems in the future
# or the dccsi maybe split up

from DccScriptingInterface.constants import ENVAR_PATH_DCCSIG

#  < o3de >\Gems\AtomLyIntegration\TechnicalArt\< dccsi >
PATH_DCCSIG = _MODULE_PATH.parents[0].resolve()
# this allows the dccsi gem location to be overridden in the external env
PATH_DCCSIG = Path(os.getenv(ENVAR_PATH_DCCSIG,
                              PATH_DCCSIG.as_posix()))
site.addsitedir(PATH_DCCSIG.as_posix())
_LOGGER.debug(f'{ENVAR_PATH_DCCSIG}: {PATH_DCCSIG}')

# pulling from this __init__.py module, may cause cyclical imports
# -------------------------------------------------------------------------
from DccScriptingInterface.globals import *
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
