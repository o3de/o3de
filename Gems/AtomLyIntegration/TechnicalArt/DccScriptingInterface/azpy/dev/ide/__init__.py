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
"""azpy.dev.ide.__init__"""

from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

#  global space
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_PACKAGENAME = 'azpy.dev.ide'

from azpy import initialize_logger
_LOGGER = initialize_logger(_PACKAGENAME)
_LOGGER.debug('Invoking __init__.py for {0}.'.format({_PACKAGENAME}))

# -------------------------------------------------------------------------

__all__ = []

try:
    import wingapi
    __all__ = init_wing(__all__)
except:
    pass

# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
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
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
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
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def import_all(_all=__all__):
    """this will test imports of __all__
    can be run before or after init() to test"""
    from azpy import test_imports
    _LOGGER.debug('Testing Imports from {0}'.format(_PACKAGENAME))
    test_imports(_all,
                 _pkg=_PACKAGENAME,
                 _logger=_LOGGER)
    return _all
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
if _DCCSI_DEV_MODE:
    # If in dev mode this will test imports of __all__
    import_all(__all__)
# -------------------------------------------------------------------------
