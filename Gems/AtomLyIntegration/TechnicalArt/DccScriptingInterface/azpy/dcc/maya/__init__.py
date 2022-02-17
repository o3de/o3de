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
<DCCsi>/azpy/dcc/maya/__init__.py

maya is a sub-module of the azpy.dcc pure-python api.
"""
# -------------------------------------------------------------------------
# standard imports
import logging as _logging
# -------------------------------------------------------------------------
# global scope
_PACKAGENAME = 'azpy.dcc.maya'

__all__ = [] # only add here, if that sub-module does NOT require bpy!

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

# Make sure we can import the native blender apis
try:
    import maya.cmds
    __all__ = init(__all__) # if we can import maya, we can load sub-modules
except:
    __all__ = []
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init(_all=__all__):
    """If the maya.cmds or other apis are required for a package/module to
    import, then it should be initialized and added here so general imports
    don't fail"""

    # extend all with submodules
    _all.append('stub')
    
    # ^ as moldules are created, add them to the list above
    # like _all.append('stub', 'foo', 'bar')
    
    # Importing local packages/modules
    return _all
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_TESTS

#  global space
_DCCSI_TESTS = env_bool(ENVAR_DCCSI_TESTS, False)

if _DCCSI_TESTS:
    # If in dev mode this will test imports of __all__
    from azpy import test_imports
    
    _LOGGER.debug('Testing Imports from {0}'.format(_PACKAGENAME))
    test_imports(__all__,
                 _pkg=_PACKAGENAME,
                 _logger=_LOGGER)
# -------------------------------------------------------------------------