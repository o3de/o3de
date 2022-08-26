#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

"""! This is a ui pkg for O3DE, DccScriptingInterface Gem

:file: DccScriptingInterface\\azpy\\shared\\ui\\__init__.py
"""

import logging as _logging

_PACKAGENAME = 'azpy.shared.ui'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

__all__ = ['samples']

# # -------------------------------------------------------------------------
# if _DCCSI_DEV_MODE:
#     # If in dev mode this will test imports of __all__
#     from azpy import test_imports
#     _LOGGER.debug('Testing Imports from {0}'.format(_PACKAGENAME))
#     test_imports(__all__,
#                  _pkg=_PACKAGENAME,
#                  _logger=_LOGGER)
# # -------------------------------------------------------------------------

del _LOGGER
