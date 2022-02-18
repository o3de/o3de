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
<DCCsi>/Tools/DCC/__init__.py

This init allows us to treat the DCCsi Tools DCC folder as a package.
"""
# -------------------------------------------------------------------------
# standard imports
import os
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
# global scope
_PACKAGENAME = 'Tools.DCC'

__all__ = ['Blender',
           'Maya']  # to do: add others when they are set up
          #'3dsMax',
          #'Houdini',
          #'Marmoset',
          #'Substance',
          #'Foo',

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {}'.format(_PACKAGENAME))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from Tools import STR_CROSSBAR
_LOGGER.debug(STR_CROSSBAR)

# set up access to this DCC folder as a pkg
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH.as_posix()))

from Tools import _PATH_DCCSIG
_LOGGER.debug('PATH_DCCSIG: {}'.format(_PATH_DCCSIG))

from Tools import _PATH_DCCSI_TOOLS
_LOGGER.debug('PATH_DCCSI_TOOLS: {}'.format(_PATH_DCCSI_TOOLS))

_PATH_DCCSI_TOOLS_DCC = Path(_MODULE_PATH.parent)
_PATH_DCCSI_TOOLS_DCC = Path(os.getenv('ATH_DCCSI_TOOLS_DCC',
                                       _PATH_DCCSI_TOOLS_DCC.as_posix()))

_LOGGER.debug('PATH_DCCSI_TOOLS_DCC: {}'.format(_PATH_DCCSI_TOOLS_DCC))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from Tools import _DCCSI_TESTS
    
if _DCCSI_TESTS:
    from azpy import test_imports
    
    _LOGGER.info(STR_CROSSBAR)
    
    _LOGGER.info('Testing Imports from {0}'.format(_PACKAGENAME))
    test_imports(__all__,
                 _pkg=_PACKAGENAME,
                 _logger=_LOGGER)
    
    _LOGGER.info(STR_CROSSBAR)    
# -------------------------------------------------------------------------