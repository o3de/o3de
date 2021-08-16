# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# standard imports
import sys
import os
import pathlib
import site
from pathlib import Path
import logging as _logging
from env_bool import env_bool

__all__ = ['start']

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.initialize'

# set these true if you want them set globally for debugging
_DCCSI_GDEBUG = env_bool('DCCSI_GDEBUG', False)
_DCCSI_DEV_MODE = env_bool('DCCSI_DEV_MODE', False)
_DCCSI_GDEBUGGER = env_bool('DCCSI_GDEBUGGER', False)
_DCCSI_LOGLEVEL = env_bool('DCCSI_LOGLEVEL', int(20))

if _DCCSI_GDEBUG:
    _DCCSI_LOGLEVEL = int(10)

FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
_logging.basicConfig(level=_DCCSI_LOGLEVEL,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

# connect to the debugger
if _DCCSI_DEV_MODE:
    APP_DATA_WING = Path('C:/Users/gallowj/AppData/Roaming/Wing Pro 7')
    APP_DATA_WING.resolve()
    site.addsitedir(pathlib.PureWindowsPath(APP_DATA_WING).as_posix())
    import wingdbstub as debugger
    try:
        debugger.Ensure()
        _LOGGER.info("Wing debugger attached")
    except Exception as e:
        _LOGGER.debug('Can not attach Wing debugger (running in IDE already?)')
# ------------------------------------------------------------------------


# ------------------------------------------------------------------------
def start():
    
    # ------------------------------------------------------------------------
    # set up access to OpenImageIO, within o3de or without
    try:
        # running in o3de
        import azlmbr
        
        # this one is less optional and if not set bootstrapping azpy will fail
        _O3DE_DEV = Path(os.getenv('O3DE_DEV', Path(azlmbr.paths.engroot)))  # assume usually not set
        os.environ['O3DE_DEV'] = pathlib.PureWindowsPath(_O3DE_DEV).as_posix()
        _LOGGER.debug(_O3DE_DEV)
    
        _O3DE_BIN_PATH = Path(str(_O3DE_DEV),Path(azlmbr.paths.executableFolder))
    
        # this one is less optional and if not set bootstrapping azpy will fail
        _O3DE_BIN = Path(os.getenv('O3DE_BIN', _O3DE_BIN_PATH.resolve()))  # assume usually not set
        os.environ['O3DE_BIN'] = pathlib.PureWindowsPath(_O3DE_BIN).as_posix()
    
        _LOGGER.debug(_O3DE_BIN)
    
        site.addsitedir(_O3DE_BIN)
    
    except Exception as e:
        # running external, start this module from:
        # Gems\AtomLyIntegration\CommonFeatures\Editor\Scripts\ColorGrading\cmdline\O3DE_py_cmd.bat
        pass
    
        try:
            _O3DE_DEV = Path(os.getenv('O3DE_DEV'))
            _O3DE_DEV = _O3DE_DEV.resolve()
            os.environ['O3DE_DEV'] = pathlib.PureWindowsPath(_O3DE_DEV).as_posix()
            _LOGGER.debug(f'O3DE_DEV is: {_O3DE_DEV}')
        except EnvironmentError as e:
            _LOGGER.error('O3DE engineroot not set or found')
            raise e
    
        try:
            _TAG_LY_BUILD_PATH = os.getenv('TAG_LY_BUILD_PATH', 'build')
            _DEFAULT_BIN_PATH = Path(str(_O3DE_DEV), _TAG_LY_BUILD_PATH, 'bin', 'profile')
            _O3DE_BIN_PATH = Path(os.getenv('O3DE_BIN_PATH', _DEFAULT_BIN_PATH))
            _O3DE_BIN_PATH = _O3DE_BIN_PATH.resolve()
            os.environ['O3DE_BIN_PATH'] = pathlib.PureWindowsPath(_O3DE_BIN_PATH).as_posix()
            _LOGGER.debug(f'O3DE_BIN_PATH is: {_O3DE_BIN_PATH}')
            site.addsitedir(pathlib.PureWindowsPath(_O3DE_BIN_PATH).as_posix())
        except EnvironmentError as e:
            _LOGGER.error('O3DE bin folder not set or found')
            raise e
# ------------------------------------------------------------------------


# ------------------------------------------------------------------------
try:
    import OpenImageIO as OpenImageIO
except ImportError as e:
    _LOGGER.error(f"invalid import: {e}")
    sys.exit(1)
# ------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""

    start()