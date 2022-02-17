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
<DCCsi>//azpy/dcc/__init__.py

dcc is a sub-module of the azpy pure-python api.
"""
# -------------------------------------------------------------------------
# standard imports
import logging as _logging
# -------------------------------------------------------------------------
# global scope
_PACKAGENAME = 'azpy.dcc'

__all__ = ['blender',
           'maya']

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_TESTS

#  global space
_DCCSI_TESTS = env_bool(ENVAR_DCCSI_TESTS, False)

if _DCCSI_TESTS:
    # If in dev mode this will test imports of __all__
    from azpy import test_imports
    
    _LOGGER.debug('Testing Imports from {0}'.format(_PACKAGENAME))
    test_imports(__all__,
                 _pkg=_PACKAGENAME,
                 _logger=_LOGGER)
# -------------------------------------------------------------------------