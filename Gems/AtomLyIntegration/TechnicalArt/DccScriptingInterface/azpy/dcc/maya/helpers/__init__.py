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
# The __init__.py files help guide import statements without automatically
# importing all of the modules
"""azpy.dcc.maya.helpers.__init__"""

import logging as _logging

import azpy.env_bool as env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import FRMT_LOG_LONG

_DCCSI_GDEBUG = env_bool.env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool.env_bool(ENVAR_DCCSI_DEV_MODE, False)

_PACKAGENAME = __name__
if _PACKAGENAME == '__main__':
    _PACKAGENAME = 'azpy.dcc.maya.helpers'

# set up module logging
for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)
_LOGGER = _logging.getLogger(_PACKAGENAME)
_logging.basicConfig(format=FRMT_LOG_LONG)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

__all__ = ['undo_context', 'utils']

del _LOGGER
# --------------------------------------------------------------------------
