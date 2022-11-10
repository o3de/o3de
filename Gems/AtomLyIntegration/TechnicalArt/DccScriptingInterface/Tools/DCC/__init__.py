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
import site
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.Tools import _PACKAGENAME
_PACKAGENAME = f'{_PACKAGENAME}.DCC'

__all__ = ['Blender',
           'Maya',
           'Substance']  # to do: add others when they are set up
          #'3dsMax',
          #'Houdini',
          #'Marmoset',
          # 'Foo',

_LOGGER = _logging.getLogger(_PACKAGENAME)

# set up access to this DCC folder as a pkg
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?

from DccScriptingInterface import add_site_dir

from DccScriptingInterface import PATH_DCCSIG
from DccScriptingInterface.Tools import PATH_DCCSI_TOOLS
from DccScriptingInterface.globals import *

from DccScriptingInterface.constants import ENVAR_PATH_DCCSI_TOOLS_DCC

# the path to this < dccsi >/Tools pkg
PATH_DCCSI_TOOLS_DCC = Path(_MODULE_PATH.parent)
# this allows the Tool location to be overriden in the external env
PATH_DCCSI_TOOLS_DCC = Path(os.getenv(ENVAR_PATH_DCCSI_TOOLS_DCC,
                                      PATH_DCCSI_TOOLS_DCC.as_posix()))
add_site_dir(PATH_DCCSI_TOOLS_DCC.as_posix())
_LOGGER.debug(f'{ENVAR_PATH_DCCSI_TOOLS_DCC}: {PATH_DCCSI_TOOLS_DCC}')
_LOGGER.debug(STR_CROSSBAR)
