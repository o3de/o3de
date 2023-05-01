#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! @brief
<DCCsi>/azpy/dcc/o3de/__init__.py

dcc/o3de is a sub-module of the azpy.dcc pure-python api.
If a sub-module requires an import from a o3de api like azlmbr,
Then it must be placed into this API so it is gated!
"""
# -------------------------------------------------------------------------
# standard imports
import os
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.azpy.dcc.o3de import _PACKAGENAME
_PKG_DCC_NAME = 'atom'
_PACKAGENAME = f'{_PACKAGENAME}.{_PKG_DCC_NAME}'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

from DccScriptingInterface.globals import *

__all__ = ['stub'] # only add here, if that sub-module does NOT require bpy!
# -------------------------------------------------------------------------


def init(_all=list()):
    """If the o3de or other apis are required for a package/module to
    import, then it should be initialized and added here so general imports
    don't fail"""

    _add_all_ = (None) # populate modules here

    # ^ as moldules are created, add them to the list above
    # like _add_all_ = list('foo', 'bar')

    for each in _add_all_:
        # extend all with submodules
        _all.append(each)

    # Importing local packages/modules
    return _all


# Make sure we can import the native blender apis
try:
    import azlmbr
    __all__ = init(__all__) # if we can import Blender, we can load sub-modules
except:
    pass # no changes to __all__


if DCCSI_DEV_MODE:
    # If in dev mode this will test imports of __all__
    from DccScriptingInterface.azpy.shared.utils.init import test_imports
    _LOGGER.debug('Testing Imports from {0}'.format(_PACKAGENAME))
    test_imports(__all__,
                 _pkg=_PACKAGENAME,
                 _logger=_LOGGER)
