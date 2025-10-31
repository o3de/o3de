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
<DCCsi>/azpy/__init__.py

This is the shared pure-python api.
"""

# -------------------------------------------------------------------------
# standard imports
import sys
import os
import site
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface import _PACKAGENAME, STR_CROSSBAR
_PACKAGENAME = f'{_PACKAGENAME}.azpy'
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
_MODULE_PATH = Path(__file__) # thos module should not be used as an entry

__all__ = ['core',
           'dcc',
           'dev',
           'o3de',
           'shared',
           'test',
           'config_class',
           'config_utils',
           'constants',
           'env_bool',
           'general_utils',
           'return_stub',
           'logger']

# future deprection of these modules
# env_base and synthetic_env were precursors to using dynaconf

from DccScriptingInterface import PATH_DCCSIG

from DccScriptingInterface.globals import *

PATH_DCCSI_AZPY = Path(_MODULE_PATH.parent)

# debug breadcrumbs to check this module and used paths
_LOGGER.debug(f'This MODULE_PATH: {_MODULE_PATH}')
_LOGGER.debug(f'Default {ENVAR_PATH_DCCSIG}: {PATH_DCCSIG}')
_LOGGER.debug(f'Default {ENVAR_O3DE_DEV}: {O3DE_DEV}')
_LOGGER.debug(STR_CROSSBAR)
# -------------------------------------------------------------------------


##########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run in debug perform local tests from IDE or CLI"""

    # This cli is no longer needed, it pre-existed config.py
    # use config.py cli as the entrypoint
