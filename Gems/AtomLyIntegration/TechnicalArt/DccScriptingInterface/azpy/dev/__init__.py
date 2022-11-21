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
# standard imports
import logging as _logging

# define api package for each IDE supported

# global scope
from DccScriptingInterface.azpy import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.dev'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

from DccScriptingInterface.globals import *

__all__ = ['ide', 'utils']
# -------------------------------------------------------------------------
