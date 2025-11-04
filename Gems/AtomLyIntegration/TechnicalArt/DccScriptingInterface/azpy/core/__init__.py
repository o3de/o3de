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
<DCCsi>/core/azpy/__init__.py

core is a sub-module of the azpy pure-python api.
"""
# -------------------------------------------------------------------------
import sys
import logging as _logging
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.azpy import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.core'

__all__ = []

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

if sys.version_info >= (3, 6):
    from azpy.core.py3.utils import get_datadir
elif sys.version_info >= (2, 6) and sys.version_info < (3, 6):
    _LOGGER.warning('Python vesion < 3 will be deprecated in the future')
    from azpy.core.py2.utils import get_datadir
else:
    _LOGGER.warning('Python vesion < 2.6 not recommended')
    from azpy.core.py2.utils import get_datadir

__all__ = ['get_datadir'] # you should hope it works
# -------------------------------------------------------------------------
