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
If a sub-module requires an import from a maya api like maya.cmds,
Then it must be placed into this API so it is gated!
"""
# -------------------------------------------------------------------------
# standard imports
import os
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------
# global scope
_PKG_DCC_NAME = 'maya'
_PACKAGENAME = 'azpy.dcc.{}'.format(_PKG_DCC_NAME)

__all__ = ['stub'] # only add here, if that sub-module does NOT require bpy!

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
from azpy.constants import STR_CROSSBAR
_LOGGER.debug(STR_CROSSBAR)

# set up access to this DCC folder as a pkg
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH.as_posix()))

from azpy import _PATH_DCCSIG
_LOGGER.debug('PATH_DCCSIG: {}'.format(_PATH_DCCSIG))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init(_all=list()):
    """If the {} or other apis are required for a package/module to
    import, then it should be initialized and added here so general imports
    don't fail""".format(_PKG_DCC_NAME)
    
    _add_all_ = ('callbacks',
                 'helpers',
                 'toolbits',
                 'utils') # populate modules here
    
    # ^ as moldules are created, add them to the list above
    # like _add_all_ = list('foo', 'bar')

    for each in _add_all_:
        # extend all with submodules
        _all.append(each) 
    
    # Importing local packages/modules
    return _all
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Make sure we can import the native blender apis
try:
    import maya.cmds
    __all__ = init(__all__) # if we can import maya, we can load sub-modules
except:
    pass # no changes to __all__
    
_LOGGER.debug('_MODULE_PATH.__all__: {}'.format(__all__))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_TESTS

#  global space
_DCCSI_TESTS = env_bool(ENVAR_DCCSI_TESTS, False)

if _DCCSI_TESTS:
    # If in dev mode this will test imports of __all__
    from azpy import test_imports
    
    _LOGGER.info(STR_CROSSBAR)
    
    _LOGGER.debug('Testing Imports from {0}'.format(_PACKAGENAME))
    test_imports(__all__,
                 _pkg=_PACKAGENAME)
    
    _LOGGER.info(STR_CROSSBAR)
# -------------------------------------------------------------------------