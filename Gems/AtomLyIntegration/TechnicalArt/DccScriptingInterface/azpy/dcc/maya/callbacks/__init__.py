#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------
# The __init__.py files help guide import statements without automatically
# importing all of the modules
"""azpy.maya.callbacks.__init__"""

# standard imports
import logging as _logging
# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.azpy.dcc.maya import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.callbaks'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

from DccScriptingInterface.globals import *

__all__ = ['event_callback_handler',
           'node_message_callback_handler',
           'on_shader_rename']
#--------------------------------------------------------------------------
