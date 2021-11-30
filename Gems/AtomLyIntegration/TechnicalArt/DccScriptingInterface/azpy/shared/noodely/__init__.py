"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

# The __init__.py files help guide import statements without automatically
# importing all of the modules
"""azpy.shared.ui.__init__"""

import os
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
#  global space
import azpy.env_bool as env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import FRMT_LOG_LONG

_DCCSI_GDEBUG = env_bool.env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool.env_bool(ENVAR_DCCSI_DEV_MODE, False)

_PACKAGENAME = __name__
if _PACKAGENAME is '__main__':
    _PACKAGENAME = 'azpy.shared.noodely'

# set up module logging
for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)
_LOGGER = _logging.getLogger(_PACKAGENAME)
_logging.basicConfig(format=FRMT_LOG_LONG)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

__all__ = ['find_arg',
           'helpers',
           'node',
           'synth',
           'synth_arg_kwarg',
           'test_foo']
# -------------------------------------------------------------------------


# -- Project Globals ------------------------------------------------------
while 1:
    # But if we want to change the prject root, we can set a new one here.
    def set_G_DEFAULT_PROJECT_DIR(value):
        global _G_DEFAULT_PROJECT_DIR
        """Sets and returns the _G_DEFAULT_PROJECT_DIR"""
        _G_DEFAULT_PROJECT_DIR = Path(value).resolve()
        return Path(_G_DEFAULT_PROJECT_DIR)
    
    def get_G_DEFAULT_PROJECT_DIR():
        global _G_DEFAULT_PROJECT_DIR
        """Returns the _G_DEFAULT_PROJECT_DIR"""
        return Path(_G_DEFAULT_PROJECT_DIR)
    
    # if we are initializing everythin properly, we can assume the project
    # variables are properly set
    global _G_DEFAULT_PROJECT_DIR
    _G_DEFAULT_PROJECT_DIR = Path(os.getcwd()).resolve()
    
    break
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
while 1:
    global _G_PRIME_ROOT_NODE
    _G_PRIME_ROOT_NODE = None  # <-- needs to be set up!
    # But if we want to change the prject root, we can set a new one here.
    
    def set_G_PRIME_ROOT_NODE(node):
        """Sets and returns the _G_PRIME_ROOT_NODE"""
        global _G_PRIME_ROOT_NODE
        if not isinstance(node, ProjectRootNode):
            message = '_G_PRIME_ROOT_NODE: {}\r'.format(type(node))
            message += 'A project root node needs to be properly set\r'
            raise TypeError(message)
        _G_PRIME_ROOT_NODE = node
        return _G_PRIME_ROOT_NODE
    
    def get_G_PRIME_ROOT_NODE():
        """Returns the _G_PRIME_ROOT_NODE"""
        global _G_PRIME_ROOT_NODE
        return _G_PRIME_ROOT_NODE
    
    break
# -------------------------------------------------------------------------


###########################################################################
# tests(), code block for testing module
# -------------------------------------------------------------------------
def tests():
    _G_DEFAULT_PROJECT_DIR = set_G_DEFAULT_PROJECT_DIR(os.getcwd())
    _LOGGER.info(_G_DEFAULT_PROJECT_DIR)
    _LOGGER.info(_G_DEFAULT_PROJECT_DIR.parent)
    _LOGGER.info(_G_DEFAULT_PROJECT_DIR.parts)

    # NOT implemented yet
    # _G_PRIME_ROOT_NODE

    return
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


###########################################################################
# --call block-------------------------------------------------------------
if __name__ == "__main__":
    _LOGGER.info("# {0} #".format('-' * 72))
    _LOGGER.info('~ noodely.prime ... Running script as __main__')
    _LOGGER.info("# {0} #".format('-' * 72))

    # run simple tests
    tests()
    
del _LOGGER