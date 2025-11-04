#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""azpy.dev.ide.__init__"""

# standard imports
import logging as _logging

from DccScriptingInterface.azpy.dev import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.ide'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

from DccScriptingInterface.globals import *

__all__ = []
# -------------------------------------------------------------------------


try:
    import wingapi
    __all__ = init_wing(__all__)
except:
    pass


def init_wing(_all):
    """If the wingapi is required for a package/module to import,
    then it should be initialized and added here so general imports
    don't fail"""

    # Make sure we can import the native apis
    import wingapi # this will fail if we can't

    _all.append('wing')
    # add others

    # Importing additional local packages/modules
    return _all


def init_all(_all):
    """If the wingapi is required for a package/module to import,
    then it should be initialized and added here so general imports
    don't fail"""

    # Make sure we can import the native apis
    import wingapi # this will fail if we can't

    _all.append('wing')
    # add others

    # Importing additional local packages/modules
    return _all


if DCCSI_DEV_MODE:
    # If in dev mode this will test imports of __all__
    from DccScriptingInterface.azpy.shared.utils.init import test_imports
    _LOGGER.debug('Testing Imports from {0}'.format(_PACKAGENAME))
    test_imports(__all__,
                 _pkg=_PACKAGENAME,
                 _logger=_LOGGER)
