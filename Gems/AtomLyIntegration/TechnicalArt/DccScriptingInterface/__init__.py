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
DCCSI_DOCS_URL = "https://github.com/o3de/o3de/blob/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/readme.md"

DCCSI_INFO = {
    'name': 'O3DE_DCCSI_GEM',
    "description": "DccScriptingInterface",
    'status': 'prototype',
    'version': (0, 0, 1),
    'platforms': ({
        'include': ('win'),   # windows
        'exclude': ('darwin', # mac
                    'linux')  # linux, linux_x64
    }),
    "doc_url": DCCSI_DOCS_URL
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

STR_CROSSBAR = f"{('-' * 74)}"
FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"

# default loglevel to info unless set
ENVAR_DCCSI_LOGLEVEL = 'DCCSI_LOGLEVEL'
DCCSI_LOGLEVEL = int(os.getenv(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))

# configure basic logger since this is the top-level module
# note: not using a common logger to reduce cyclical imports
_logging.basicConfig(level=DCCSI_LOGLEVEL,
                    format=FRMT_LOG_LONG,
                    datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.info(STR_CROSSBAR)
_LOGGER.info(f'Initializing: {_PACKAGENAME}')

__all__ = ['globals', # global state module
           'config', # dccsi core config.py
           'constants', # global dccsi constants
           'foundation', # set up dependancy pkgs for DCC tools
           'return_sys_version', # util
           'azpy', # shared pure python api
           'Editor', # O3DE editor scripts
           'Tools' # DCC and IDE tool integrations
           ]

# be careful when pulling from this __init__.py module
# avoid cyclical imports, this module should not import from sub-modules

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
ENVAR_PATH_DCCSIG = 'PATH_DCCSIG'

# < o3de >\Gems\AtomLyIntegration\TechnicalArt\< dccsi >
# the default location
PATH_DCCSIG = _MODULE_PATH.parents[0].resolve()

# this allows the dccsi gem location to be overridden in the external env
PATH_DCCSIG = Path(os.getenv(ENVAR_PATH_DCCSIG,
                              PATH_DCCSIG.as_posix()))
try: # validate (if envar is bad)
    PATH_DCCSIG.resolve(strict=True)
    _LOGGER.info(f'{ENVAR_PATH_DCCSIG}: {PATH_DCCSIG}')
except NotADirectoryError as e:
    _LOGGER.error(f'{ENVAR_PATH_DCCSIG} failed: {PATH_DCCSIG}')
    raise e

# < dccsi >\3rdParty bootstraooing area for installed site-packages
# dcc tools on a different version of python will import from here.
# path string constructor
PATH_DCCSI_PYTHON_LIB = (f'{PATH_DCCSIG.as_posix()}' +
                         f'\\3rdParty\\Python\\Lib' +
                         f'\\{sys.version_info[0]}.x' +
                         f'\\{sys.version_info[0]}.{sys.version_info[1]}.x' +
                         f'\\site-packages')
PATH_DCCSI_PYTHON_LIB = Path(PATH_DCCSI_PYTHON_LIB).resolve()

# default o3de engine location
# a suggestion would be for us to refactor from _O3DE_DEV to _O3DE_ROOT
# dev is a legacy Lumberyard concept, as the engine snadbox was /dev
ENVAR_O3DE_DEV = 'O3DE_DEV'
O3DE_DEV = None
try: # check if we are running in o3de Editor
    import azlmbr.paths
    O3DE_DEV = Path(azlmbr.paths.engroot).resolve()
    _LOGGER.info(f'{ENVAR_O3DE_DEV} is: {O3DE_DEV.as_posix()}')
except ImportError as e:
    _LOGGER.warning(f'Not running o3do, no: azlmbr.paths.engroot')
    _LOGGER.warning(f'Using a fallback')
    pass

if not O3DE_DEV: # fallback 1, check if a dev set it in env
    if os.getenv(ENVAR_O3DE_DEV):
        O3DE_DEV = os.getenv(ENVAR_O3DE_DEV)
        try:
            O3DE_DEV = Path(O3DE_DEV).resolve(strict=True)
            _LOGGER.info(f'{ENVAR_O3DE_DEV} is: {O3DE_DEV.as_posix()}')
        except NotADirectoryError as e:
            O3DE_DEV = None
            _LOGGER.warning(f'{ENVAR_O3DE_DEV} does not exist: {O3DE_DEV.as_posix()}')

if not O3DE_DEV: # fallback 2, assume root from dccsi
    O3DE_DEV = PATH_DCCSIG.parents[3]
    try:
        O3DE_DEV.resolve(strict=True)
        _LOGGER.info(f'{ENVAR_O3DE_DEV} is: {O3DE_DEV.as_posix()}')
    except NotADirectoryError as e:
        O3DE_DEV = None
        _LOGGER.error(f'{ENVAR_O3DE_DEV} does not exist: {O3DE_DEV.as_posix()}')
        raise e

# should we implement a safe way to manage adding paths
# to make sure we are not adding paths already there?
sys.path.append(O3DE_DEV.as_posix())

# use the dccsi as the default fallback project during development
# these can be replaced in the config.py, the defaults are fine for this stage
ENVAR_PATH_O3DE_PROJECT = 'PATH_O3DE_PROJECT' # project path
ENVAR_O3DE_PROJECT = 'O3DE_PROJECT' # project name

# use dccsi as the default project location
PATH_O3DE_PROJECT = Path(os.getenv(ENVAR_PATH_O3DE_PROJECT,
                                   PATH_DCCSIG)).resolve()
_LOGGER.info(f'Default {ENVAR_PATH_O3DE_PROJECT}: {PATH_O3DE_PROJECT.as_posix()}')

# use dccsi as the default project name
O3DE_PROJECT = str(os.getenv(ENVAR_O3DE_PROJECT,
                             PATH_O3DE_PROJECT.name))
_LOGGER.info(f'Default {ENVAR_O3DE_PROJECT}: {O3DE_PROJECT}')
_LOGGER.info(STR_CROSSBAR)

# END

# -------------------------------------------------------------------------
# will probably deprecate this whole block to avoid cyclical imports
# this doesn't need to be an entrypoint or debugged from cli

# from DccScriptingInterface.globals import *
# from azpy.config_utils import attach_debugger
# from azpy import test_imports

# # suggestion would be to turn this into a method to reduce boilerplate
# # but where to put it that makes sense?
# if DCCSI_DEV_MODE:
#     # if dev mode, this will attemp to auto-attach the debugger
#     # at the earliest possible point in this module
#     attach_debugger(debugger_type=DCCSI_GDEBUGGER)
#
#     _LOGGER.debug(f'Testing Imports from {_PACKAGENAME}')
#
#     # If in dev mode and test is flagged this will force imports of __all__
#     # although slower and verbose, this can help detect cyclical import
#     # failure and other issues
#
#     # the DCCSI_TESTS flag needs to be properly added in .bat env
#     if DCCSI_TESTS:
#         test_imports(_all=__all__,
#                      _pkg=_PACKAGENAME,
#                      _logger=_LOGGER)
# -------------------------------------------------------------------------
