#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
# standard imports
import logging as _logging

from DccScriptingInterface.Tools.DCC.Maya import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.Scripts'
_LOGGER = _logging.getLogger(_PACKAGENAME)

__all__ = ['set_callbacks',
           'set_defaults',
           'set_menu',
           'set_shelf',
           'userSetup']
