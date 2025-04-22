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
from DccScriptingInterface.azpy.dcc.blender import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.helpers'
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

__all__ = ['blender_materials_conversion',
           'convert_bsdf_material']
