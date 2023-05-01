#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------
"""azpy.3dsmax.__init__"""

# standard imports
import logging as _logging

from DccScriptingInterface.azpy.dcc import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.3dsmax'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

from DccScriptingInterface.globals import *


# These are explicit imports for now
__all__ = []
# To Do: procedurally discover dcc access and extend __all__
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def init():
    """If the 3dsmax api is required for a package/module to import,
    then it should be initialized and added here so general imports
    don't fail"""

    # Make sure we can import the native apis
    import pymxs
    import MaxPlus

    # extend all with submodules
    #__all__.append('foo', 'bar')

    # Importing local packages/modules
    pass


if DCCSI_DEV_MODE:
    # If in dev mode this will test imports of __all__
    from DccScriptingInterface.azpy.shared.utils.init import test_imports
    _LOGGER.debug('Testing Imports from {0}'.format(_PACKAGENAME))
    test_imports(__all__,
                 _pkg=_PACKAGENAME,
                 _logger=_LOGGER)
