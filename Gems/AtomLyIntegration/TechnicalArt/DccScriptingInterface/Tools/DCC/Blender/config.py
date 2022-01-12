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
# standard imports
import sys
import os
import site
import re
import importlib.util
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_MODULENAME = 'O3DE.DCCsi.Tools.DCC.Blender.config'

# configure basic logger
FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
_logging.basicConfig(level=_logging.DEBUG,
                     format=FRMT_LOG_LONG,
                    datefmt='%m-%d %H:%M')
_LOGGER = _logging.getLogger(_MODULENAME)

_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# we don't use dynaconf setting here as we might not yet have access
# we need to set up basic access to the DCCsi
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?
_PATH_DCCSIG = Path(_MODULE_PATH, '../../../..')
_LOGGER.debug(f'PATH_DCCSIG: {_PATH_DCCSIG.resolve()}')
site.addsitedir(_PATH_DCCSIG.resolve())

# set envar so DCCsi synthetic env bootstraps with it (config.py)
from azpy.constants import ENVAR_PATH_DCCSIG
os.environ[ENVAR_PATH_DCCSIG] = str(_PATH_DCCSIG.resolve())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def attach_debugger():
    _DCCSI_GDEBUG = True
    os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)

    _DCCSI_DEV_MODE = True
    os.environ["DYNACONF_DCCSI_DEV_MODE"] = str(_DCCSI_DEV_MODE)

    from azpy.test.entry_test import connect_wing
    _debugger = connect_wing()

    return _debugger
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# _settings.setenv()  # doing this will add the additional DYNACONF_ envars
def get_dccsi_config(PATH_DCCSIG=_PATH_DCCSIG.resolve()):
    """Convenience method to set and retreive settings directly from module."""

    attach_debugger()

    try:
        Path(PATH_DCCSIG).exists()
    except FileNotFoundError as e:
        _LOGGER.debug(f"File does not exist: {PATH_DCCSIG}")
        return None

    # we can go ahead and just make sure the the DCCsi env is set
    # _config is SO generic this ensures we are importing a specific one
    _spec_dccsi_config = importlib.util.spec_from_file_location("dccsi._config",
                                                                Path(PATH_DCCSIG,
                                                                     "config.py"))
    _dccsi_config = importlib.util.module_from_spec(_spec_dccsi_config)
    _spec_dccsi_config.loader.exec_module(_dccsi_config)

    return _dccsi_config
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_CONFIG = get_dccsi_config()
settings = _CONFIG.get_config_settings(enable_o3de_python=False,
                                        enable_o3de_pyside2=True)
# we don't init the O3DE python env settings!
# that will cause conflicts with python enabled DCC tools!!!
# we are enabling the O3DE PySide2 (aka QtForPython) access

_DCCSI_PATH_BLENDER = Path(sys.prefix)
os.environ["DYNACONF_DCCSI_PATH_BLENDER"] = _DCCSI_PATH_BLENDER.resolve()
_LOGGER.debug(f"Blender Install: {_DCCSI_PATH_BLENDER}")

from dynaconf import settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_dccsi_blender_settings(settings):
    return settings
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main (external commandline for testing)"""
    pass