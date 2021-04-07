"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
# -------------------------------------------------------------------------

# The __init__.py files help guide import statements without automatically
# importing all of the modules
"""azpy.shared.ui.__init__"""

import os
import logging
import logging.config

#  global space debug flag
_G_DEBUG = os.getenv('DCCSI_GDEBUG', False)

#  global space debug flag
_DCCSI_DEV_MODE = os.getenv('DCCSI_DEV_MODE', False)

if _DCCSI_DEV_MODE:
    _PACKAGENAME = __name__
    if _PACKAGENAME is '__main__':
        _PACKAGENAME = 'noodely'

_PKG_PARENT_PATH = str('azpy.shared')
_PKG_PATH = str('{0}.{1}'.format(_PKG_PARENT_PATH, _PACKAGENAME))
_LOGGER = logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Invoking __init__.py for {0}.'.format({_PKG_PATH}))

# -------------------------------------------------------------------------
#
__all__ = ['config', 'find_arg', 'master', 'node', 'synth',
           'synth_arg_kwarg', 'test_foo']
#
# -------------------------------------------------------------------------

del _LOGGER
