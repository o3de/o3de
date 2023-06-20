
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
<DCCsi>/Editor/Scripts/__init__.py
"""
import os
from pathlib import Path
import logging as _logging

# -------------------------------------------------------------------------
# global scope
_DCCSI_SLUG = 'DccScriptingInterface'
_PACKAGENAME = 'DCCsi.Editor.Scripts'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
_MODULE_PATH = Path(__file__) # thos module should not be used as an entry

__ALL__ = ['about', 'ui']
