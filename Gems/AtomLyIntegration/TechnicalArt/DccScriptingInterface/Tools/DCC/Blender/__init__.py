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
DCCsi/Tools/DCC/Blender/__init__.py

This init allows us to treat Blender setup as a DCCsi tools python package
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
_PACKAGENAME = 'Tools.DCC.Blender'

__all__ = ['config',
           'constants',
           'setup',
           'start']

_LOGGER = _logging.getLogger(_PACKAGENAME)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# set up access to this Blender folder as a pkg
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_DCCSI_TOOLS_BLENDER_PATH = Path(_MODULE_PATH.parent)

# we need to set up basic access to the DCCsi
_PATH_DCCSI_TOOLS_DCC = Path(_DCCSI_TOOLS_BLENDER_PATH.parent)
_PATH_DCCSI_TOOLS_DCC = Path(os.getenv('PATH_DCCSI_TOOLS_DCC', _PATH_DCCSI_TOOLS_DCC.as_posix()))
site.addsitedir(_PATH_DCCSI_TOOLS_DCC.as_posix())

# we need to set up basic access to the DCCsi
_PATH_DCCSI_TOOLS = Path(_PATH_DCCSI_TOOLS_DCC.parent)
_PATH_DCCSI_TOOLS = Path(os.getenv('PATH_DCCSI_TOOLS', _PATH_DCCSI_TOOLS.as_posix()))

# we need to set up basic access to the DCCsi
_PATH_DCCSIG = Path.joinpath(_DCCSI_TOOLS_BLENDER_PATH, '../../..').resolve()
_PATH_DCCSIG = Path(os.getenv('PATH_DCCSIG', _PATH_DCCSIG.as_posix()))
site.addsitedir(_PATH_DCCSIG.as_posix())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# now we have access to the DCCsi code and azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG

#  global space
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')

# default loglevel to info unless set
_DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))
if _DCCSI_GDEBUG:
    # override loglevel if runnign debug
    _DCCSI_LOGLEVEL = _logging.DEBUG
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                        format=FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')
    _LOGGER = _logging.getLogger(_PACKAGENAME)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def attach_debugger():
    """!
    This will attemp to attch the WING debugger
    To Do: other IDEs for debugging not yet implemented.
    This should be replaced with a plugin based dev package."""
    _DCCSI_GDEBUG = True
    os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)
    
    _DCCSI_DEV_MODE = True
    os.environ["DYNACONF_DCCSI_DEV_MODE"] = str(_DCCSI_DEV_MODE)
    
    from azpy.test.entry_test import connect_wing
    _debugger = connect_wing()
    
    return _debugger

if _DCCSI_DEV_MODE:
    attach_debugger()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# message collection
_LOGGER.debug(f'Initializing: {_PACKAGENAME}')
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH}')
_LOGGER.debug(f'PATH_DCCSIG: {_PATH_DCCSIG}')
_LOGGER.debug(f'PATH_DCCSI_TOOLS: {_PATH_DCCSI_TOOLS}')
_LOGGER.debug(f'PATH_DCCSI_TOOLS_DCC: {_PATH_DCCSI_TOOLS_DCC}')
_LOGGER.debug(f'DCCSI_TOOLS_BLENDER_PATH: {_DCCSI_TOOLS_BLENDER_PATH}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def test_imports(_all=__all__,_pkg=_PACKAGENAME,_logger=_LOGGER):
    # If in dev mode this will test imports of __all__
    _logger.debug(f"~   Import triggered from: {_pkg}")
    import importlib
    for mod_str in _all:
        try:
            # this is py2.7 compatible
            # in py3.5+, we can use importlib.util instead
            importlib.import_module(f'.{mod_str}', _pkg)
            _logger.debug(f"~       Imported module: {_pkg}.{mod_str}")
        except Exception as e:
            _logger.warning(f'~       {e}')
            _logger.warning(f"~       {_pkg}.{mod_str} :: ImportFail")
            return False
    return True
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
if _DCCSI_DEV_MODE:
    # If in dev mode this will test imports of __all__
    _LOGGER.debug(f'Testing Imports from {_PACKAGENAME}')
    test_imports(__all__)
# -------------------------------------------------------------------------



###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run as main, perform debug and tests"""
    pass
    