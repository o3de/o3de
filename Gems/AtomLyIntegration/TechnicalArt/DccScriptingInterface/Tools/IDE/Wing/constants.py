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
"""Common constants for the dccsi wing pro ide integration

:file: DccScriptingInterface\\Tools\\IDE\\Wing\\constants.py
:Status: Prototype
:Version: 0.0.1
"""
# -------------------------------------------------------------------------
# standard imports
import logging as _logging
# -------------------------------------------------------------------------
_MODULENAME = 'DCCsi.Tools.IDE.Wing.constants'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')
# -------------------------------------------------------------------------
# dccsi imports here
from DccScriptingInterface.Tools.IDE.Wing import USER_HOME
# wing is a x86 program, this pulls the constant for that (not a Path object)
from DccScriptingInterface.Tools.IDE.Wing import PATH_PROGRAMFILES_X86
from DccScriptingInterface.Tools.IDE.Wing import PATH_DCCSI_TOOLS_IDE_WING
# -------------------------------------------------------------------------
# wing pro ide constants here
# a setting to know the wing config exists
ENVAR_DCCSI_CONFIG_IDE_WING = 'DCCSI_CONFIG_IDE_WING'

# enavr to get/set the wing pro version
ENVAR_DCCSI_WING_VERSION_MAJOR = 'DCCSI_WING_VERSION_MAJOR'

# the default supported version of wing pro is 8
from DccScriptingInterface.Tools.IDE.Wing import SLUG_DCCSI_WING_VERSION_MAJOR

# str slug for the default wing type
# in the future, add support for wing personal and maybe wing 101 versions
from DccScriptingInterface.Tools.IDE.Wing import SLUG_DCCSI_WING_TYPE

# path string constructor, dccsi default WINGHOME location
from DccScriptingInterface.Tools.IDE.Wing import PATH_WINGHOME

# wing native hook for it's home location, used in wingstub.py
ENVAR_WINGHOME = 'WINGHOME'

ENVAR_WINGHOME_BIN = 'WINGHOME_BIN'

# path string constructor, wing home bin folder
PATH_WINGHOME_BIN = f'{PATH_WINGHOME}\\bin'

# envar to get/set the wing pro exe
ENVAR_WING_EXE = 'WING_EXE'

# path string constructor, wing home bin folder
PATH_WINGHOME_BIN_EXE = f'{PATH_WINGHOME_BIN}\\wing.exe'

# this is already available PATH_DCCSI_TOOLS_IDE_WING, imported above
# path string constructor, dccsi wing sub-dir
# PATH_DCCSI_TOOLS_IDE_WING = f'{PATH_DCCSIG.as_posix()}\\Tools\\IDE\\Wing'

# envar to get/set the solution project file for wing
ENVAR_WING_PROJ = 'WING_PROJ'

# path string constructor, the dccsi wing solution file
PATH_DCCSI_TOOLS_IDE_WING_PROJ = (f'{PATH_DCCSI_TOOLS_IDE_WING}' +
                                  f'\\.solutions' +
                                  f'\\DCCsi_{SLUG_DCCSI_WING_VERSION_MAJOR}x.wpr')

# envar to get/set the userhome wing appdata folder
ENVAR_WING_APPDATA = 'WING_APPDATA'

# path string constructor, userhome where wingstubdb.py can live
from DccScriptingInterface.Tools.IDE.Wing import PATH_WING_APPDATA
# --- END -----------------------------------------------------------------
