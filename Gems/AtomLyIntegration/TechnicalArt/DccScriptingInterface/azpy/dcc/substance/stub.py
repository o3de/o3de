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
<DCCsi>/azpy/dcc/substance/stub.py

stub is a sub-module placeholder, for the azpy.dcc.substance api.
"""
# -------------------------------------------------------------------------
# standard imports
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from azpy.dcc.substance import _PKG_DCC_NAME

_MODULENAME = 'tools.dcc.{}.stub'.format(_PKG_DCC_NAME)

_LOGGER = _logging.getLogger(_MODULENAME)

_LOGGER.info('This stub is an api placeholder: {}'.format(_MODULENAME))