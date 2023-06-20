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
from DccScriptingInterface.azpy.shared import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.noodely'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

from DccScriptingInterface.globals import *

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


###########################################################################
# --call block-------------------------------------------------------------
if __name__ == "__main__":
    from DccScriptingInterface import STR_CROSSBAR

    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('~ noodely.prime ... Running script as __main__')
    _LOGGER.info(STR_CROSSBAR)

    # run simple tests
    tests()
