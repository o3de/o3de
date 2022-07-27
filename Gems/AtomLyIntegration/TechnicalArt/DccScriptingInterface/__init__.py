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

Allow DccScriptingInterface Gem to be a python pkg
"""
import os
import site
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------
# global scope
_PACKAGENAME = 'DCCsi'

__all__ = ['config',
           'constants',
           'foundation',
           'return_sys_version']

# we need to set up basic access to the DCCsi
_MODULE_PATH = Path(__file__)

# add gems parent
_PATH_O3DE_TECHART_GEMS = _MODULE_PATH.parents[1].resolve()
site.addsitedir(_PATH_O3DE_TECHART_GEMS)

_PATH_DCCSIG = _MODULE_PATH.parents[0].resolve()
# allows env to override the path externally
_PATH_DCCSIG = os.getenv('PATH_DCCSIG', _PATH_DCCSIG)
site.addsitedir(_PATH_DCCSIG)

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
