
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
import logging as _logging
from DccScriptingInterface.azpy.o3de import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.renderer'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

__all__ = ['materials']
