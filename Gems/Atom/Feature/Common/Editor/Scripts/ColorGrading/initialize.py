# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

# local imports
from ColorGrading import initialize_logger
from ColorGrading import DCCSI_GDEBUG
from ColorGrading import DCCSI_DEV_MODE
from ColorGrading import DCCSI_LOGLEVEL

__all__ = ['start']

# ------------------------------------------------------------------------
_PACKAGENAME = 'ColorGrading'
_MODULENAME = 'ColorGrading.initialize'

if DCCSI_GDEBUG:
    DCCSI_LOGLEVEL = int(10)

# set up logger with both console and file _logging
_LOGGER = initialize_logger(_PACKAGENAME, log_to_file=DCCSI_GDEBUG, default_log_level=DCCSI_LOGLEVEL)
        
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

# connect to the debugger
if DCCSI_DEV_MODE:
    from ColorGrading import DCCSI_WING_VERSION_MAJOR
    from ColorGrading import get_datadir
    APPDATA = get_datadir()  # os APPDATA
    APPDATA_WING = Path(APPDATA, f"Wing Pro {DCCSI_WING_VERSION_MAJOR}").resolve()
    if APPDATA_WING.exists():
        site.addsitedir(APPDATA_WING.resolve())
        import wingdbstub as debugger
        try:
            debugger.Ensure()
            _LOGGER.info("Wing debugger attached")
        except Exception as e:
            _LOGGER.debug('Can not attach Wing debugger (running in IDE already?)')
    else:
        _LOGGER.warning("Path envar doesn't exist: APPDATA_WING")
        _LOGGER.info(f"Pattern: {APPDATA_WING}")
# ------------------------------------------------------------------------


# ------------------------------------------------------------------------
def start():
    """set up access to OpenImageIO, within o3de or without"""
    # ------------------------------------------------------------------------
    running_editor = None
    try:
        # running in o3de
        import azlmbr
        running_editor = True
    
    except Exception as e:
        # running external, start this module from:
        # "C:\Depot\o3de-engine\Gems\Atom\Feature\Common\Tools\ColorGrading\cmdline\CMD_ColorGradingTools.bat"

        try:
            _O3DE_DEV = Path(os.getenv('O3DE_DEV'))
            os.environ['O3DE_DEV'] = _O3DE_DEV.as_posix()
            _LOGGER.debug(f'O3DE_DEV is: {_O3DE_DEV}')
        except EnvironmentError as e:
            _LOGGER.error('O3DE engineroot not set or found')
            raise e
    
        try:
            _TAG_LY_BUILD_PATH = os.getenv('TAG_LY_BUILD_PATH', 'build')
            _DEFAULT_BIN_PATH = Path(str(_O3DE_DEV), _TAG_LY_BUILD_PATH, 'bin', 'profile')
            _O3DE_BIN_PATH = Path(os.getenv('O3DE_BIN_PATH', _DEFAULT_BIN_PATH))
            os.environ['O3DE_BIN_PATH'] = _O3DE_BIN_PATH.as_posix()
            _LOGGER.debug(f'O3DE_BIN_PATH is: {_O3DE_BIN_PATH}')
            site.addsitedir(_O3DE_BIN_PATH.resolve())
        except EnvironmentError as e:
            _LOGGER.error('O3DE bin folder not set or found')
            raise e
        
        if running_editor:
            _O3DE_DEV = Path(os.getenv('O3DE_DEV', Path(azlmbr.paths.engroot)))
            os.environ['O3DE_DEV'] = _O3DE_DEV.as_posix()
            _LOGGER.debug(_O3DE_DEV)
        
            _O3DE_BIN_PATH = Path(str(_O3DE_DEV),Path(azlmbr.paths.executableFolder))
        
            _O3DE_BIN = Path(os.getenv('O3DE_BIN', _O3DE_BIN_PATH.resolve()))
            os.environ['O3DE_BIN'] = _O3DE_BIN_PATH.as_posix()
        
            _LOGGER.debug(_O3DE_BIN)
        
            site.addsitedir(_O3DE_BIN)

    # test access to oiio
    if os.name == 'nt':
        try:
            import OpenImageIO as oiio
            return True
        except ImportError as e:
            _LOGGER.error(f"invalid import: {e}")
            pass
    else:
        _LOGGER.info("Non-Windows platforms not yet supported...")
        return False
# ------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""

    oiio_exists = start()
    _LOGGER.debug(f"Import OpenImageIO performed: {oiio_exists}")