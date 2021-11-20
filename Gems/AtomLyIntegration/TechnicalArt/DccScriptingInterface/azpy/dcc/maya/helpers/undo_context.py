# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------

# -------------------------------------------------------------------------
# -------------------------------------------------------------------------
# <DCCsi>\\azpy\\maya\\\callbacks\\node_message_callback_handler.py
# Maya node message callback handler
# Reference: Rob Galanakis, Tech-artists.org
# -------------------------------------------------------------------------
# -------------------------------------------------------------------------


"""
This module creates a simple Class object for managing Maya Undo Chunking.
"""

# -------------------------------------------------------------------------
# -- Standard Python modules
import os
from functools import wraps

# -- External Python modules

# -- Lumberyard Extension Modules
from azpy import initialize_logger
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

# -- Maya Modules --
import maya.cmds as mc
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# -- Misc Global Space Definitions
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_PACKAGENAME = __name__
if _PACKAGENAME is '__main__':
    _PACKAGENAME = 'azpy.dcc.maya.helpers.undo_context'

_LOGGER = initialize_logger(_PACKAGENAME, default_log_level=int(20))
_LOGGER.debug('Invoking:: {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# =========================================================================
# First Class
# =========================================================================
class UndoContext(object):
    """
    This Class creates a undo context chunk
    """
    def __enter__(self):
        mc.undoInfo(openChunk=True)
    def __exit__(self, *exc_info):
        mc.undoInfo(closeChunk=True)
# -------------------------------------------------------------------------


# =========================================================================
# Undo Decorator ... makes a whole function call undoable
# =========================================================================
def undo(func, autoUndo=False):
    """
    Puts the wrapped `func` into a single Maya Undo action,
    then undoes it when the function enters the finally: block
    """
    @wraps(func)
    def _undofunc(*args, **kwargs):
        try:
            # start an undo chunk
            mc.undoInfo( openChunk = True )
            return func( *args, **kwargs )
        finally:
            # after calling the func, end the undo chunk and undo
            mc.undoInfo( closeChunk = True )
            if autoUndo:
                mc.undo()

    return _undofunc
# -------------------------------------------------------------------------


# =========================================================================
# Public Functions
# =========================================================================
# --First Function---------------------------------------------------------
def test():
    """test() example undo context """

    ## This is how you call to the UndoContext()
    with UndoContext():
        # Do a couple things, in a block
        # undo should step backwards clearing all of them at once
        mc.polySphere(sx=10, sy=15, r=20)
        mc.move( 1, 1, 1 )
        mc.move( 5, y=True )
# -------------------------------------------------------------------------


#==========================================================================
# Module Tests
#==========================================================================
if __name__ == "__main__" :
    test()
