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

"""azpy.shared.__init__"""

import logging as _logging

import azpy.env_bool as env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import FRMT_LOG_LONG

#  global space
_DCCSI_GDEBUG = env_bool.env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool.env_bool(ENVAR_DCCSI_DEV_MODE, False)

_PACKAGENAME = __name__
if _PACKAGENAME is '__main__':
    _PACKAGENAME = 'azpy.dcc'

# set up module logging
for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)
_LOGGER = _logging.getLogger(_PACKAGENAME)
_logging.basicConfig(format=FRMT_LOG_LONG)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

# -------------------------------------------------------------------------
# These are explicit imports for now
__all__ = []
# To Do: procedurally discover dcc access and extend __all__
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
if _DCCSI_DEV_MODE:
    # If in dev mode this will test imports of __all__
    from azpy import test_imports
    _LOGGER.debug('Testing Imports from {0}'.format(_PACKAGENAME))
    test_imports(__all__,
                 _pkg=_PACKAGENAME,
                 _logger=_LOGGER)
# -------------------------------------------------------------------------

del _LOGGER
