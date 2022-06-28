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
Module Documentation:
    < DCCsi >:: Tools//DCC//Substance//config.py

This module manages the dynamic config and settings for boostrapping
Adobe Substance Designer.
"""
# -------------------------------------------------------------------------
# standard imports
import sys
import os
import site
import re
import importlib.util
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
#os.environ['PYTHONINSPECT'] = 'True'
# global scope
_MODULENAME = 'Tools.DCC.Substance.config'

_START = timeit.default_timer() # start tracking

# we need to set up basic access to the DCCsi
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_PATH_DCCSIG = Path(_MODULE_PATH, '../../../..').resolve()

# set envar so DCCsi synthetic env bootstraps with it (config.py)
from azpy.constants import ENVAR_PATH_DCCSIG
os.environ[ENVAR_PATH_DCCSIG] = str(_PATH_DCCSIG.as_posix())
site.addsitedir(_PATH_DCCSIG.as_posix())

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()')
_LOGGER.debug(f'PATH_DCCSIG: {_PATH_DCCSIG.as_posix()')
# -------------------------------------------------------------------------
