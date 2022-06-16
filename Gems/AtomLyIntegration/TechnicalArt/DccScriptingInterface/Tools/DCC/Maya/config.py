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
Module Documentation:
    < DCCsi >:: Tools/DCC/Maya/config.py

This module manages the dynamic config and settings for boostrapping Blender
"""
# -------------------------------------------------------------------------
# standard imports
import sys
import os
import re
import site
import inspect
import traceback
import importlib.util
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'Tools.DCC.Maya.config'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Maya is frozen
# module path when frozen
_MODULE_PATH = Path(os.path.abspath(inspect.getfile(inspect.currentframe())))
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH))

_PATH_DCCSI_TOOLS_MAYA = Path(_MODULE_PATH.parent)
_PATH_DCCSI_TOOLS_MAYA = Path(os.getenv('PATH_DCCSI_TOOLS_MAYA', _PATH_DCCSI_TOOLS_MAYA.as_posix()))
site.addsitedir(_PATH_DCCSI_TOOLS_MAYA.as_posix())

_PATH_DCCSI_TOOLS_DCC = Path(_PATH_DCCSI_TOOLS_MAYA.parent)
_PATH_DCCSI_TOOLS_DCC = Path(os.getenv('PATH_DCCSI_TOOLS_DCC', _PATH_DCCSI_TOOLS_DCC.as_posix()))
site.addsitedir(_PATH_DCCSI_TOOLS_DCC.as_posix())

_PATH_DCCSI_TOOLS = Path(_PATH_DCCSI_TOOLS_DCC.parent)
_PATH_DCCSI_TOOLS = Path(os.getenv('PATH_DCCSI_TOOLS', _PATH_DCCSI_TOOLS.as_posix()))

_PATH_DCCSIG = Path(_PATH_DCCSI_TOOLS.parent)
_PATH_DCCSIG = Path(os.getenv('PATH_DCCSIG', _PATH_DCCSIG.as_posix()))
site.addsitedir(_PATH_DCCSIG.as_posix())

# now we have azpy api access
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# _settings.setenv()  # doing this will add the additional DYNACONF_ envars
def get_dccsi_config(PATH_DCCSIG=_PATH_DCCSIG.as_posix()):
    """Convenience method to set and retrieve settings directly from module."""
    
    _PATH_DCCSIG = Path(PATH_DCCSIG)
    _PATH_DCCSI_CONFIG = Path(PATH_DCCSIG, "config.py")

    try:
        _PATH_DCCSIG.exists()
    except FileNotFoundError as e:
        _LOGGER.debug(f"File does not exist: {PATH_DCCSIG}")
        return None

    # we can go ahead and just make sure the the DCCsi env is set
    # _config is SO generic this ensures we are importing a specific one
    _spec_dccsi_config = importlib.util.spec_from_file_location("dccsi._config",
                                                                _PATH_DCCSI_CONFIG.as_posix())
    _dccsi_config = importlib.util.module_from_spec(_spec_dccsi_config)
    _spec_dccsi_config.loader.exec_module(_dccsi_config)

    return _dccsi_config
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_CONFIG = get_dccsi_config()
settings = _CONFIG.get_config_settings(enable_o3de_python=False,
                                        enable_o3de_pyside2=True)
# we don't init the O3DE python env settings!
# that will cause conflicts with the DCC tools python!!!
# we are enabling the O3DE PySide2 (aka QtForPython) access

#_DCCSI_PATH_MAYA = Path(sys.prefix)
#os.environ["DYNACONF_DCCSI_PATH_MAYA"] = _DCCSI_PATH_MAYA.resolve()
#_LOGGER.debug(f"Maya Install: {_DCCSI_PATH_MAYA}")

from dynaconf import settings
settings.setenv()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_dccsi_maya_settings(settings):
    return settings
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main (external commandline for testing)"""
    pass