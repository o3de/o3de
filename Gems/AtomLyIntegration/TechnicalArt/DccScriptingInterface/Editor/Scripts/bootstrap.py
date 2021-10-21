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
"""This module is for use in boostrapping the DccScriptingInterface Gem
with O3DE. Note: this boostrap is only designed fo be py3 compatible.
If you need DCCsi access in py27 (Autodesk Maya for instance) you may need
to implement your own boostrapper module. Currently this is boostrapped
from add_dccsi.py, as a temporty measure related to this Jira:
SPEC-2581"""
# standard imports
import sys
import os
import site
import importlib.util
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_O3DE_RUNNING=None    
try:
    import azlmbr
    _O3DE_RUNNING=True
except:
    _O3DE_RUNNING=False
# ------------------------------------------------------------------------- 


# -------------------------------------------------------------------------
# we don't use dynaconf setting here as we might not yet have access
# to that site-dir.

_MODULENAME = __name__
if _MODULENAME is '__main__':
    _MODULENAME = 'O3DE.DCCsi.bootstrap'

# set up module logging
for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)
_LOGGER = _logging.getLogger(_MODULENAME)

# we need to set up basic access to the DCCsi
_MODULE_PATH = os.path.realpath(__file__)  # To Do: what if frozen?
_DCCSI_PATH = os.path.normpath(os.path.join(_MODULE_PATH, '../../..'))
_DCCSI_PATH = os.getenv('DCCSI_PATH', _DCCSI_PATH)
site.addsitedir(_DCCSI_PATH)

# now we have azpy api access
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import FRMT_LOG_LONG

# set up global space, logging etc.
# set these true if you want them set globally for debugging
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, int(20)))
if _DCCSI_GDEBUG:
    _DCCSI_LOGLEVEL = int(10)

_logging.basicConfig(format=FRMT_LOG_LONG, level=_DCCSI_LOGLEVEL)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# _settings.setenv()  # doing this will add the additional DYNACONF_ envars
def get_dccsi_config(DCCSI_PATH=_DCCSI_PATH):
    """Convenience method to set and retreive settings directly from module."""
    
    # we can go ahead and just make sure the the DCCsi env is set
    # _config is SO generic this ensures we are importing a specific one
    _spec_dccsi_config = importlib.util.spec_from_file_location("dccsi._config",
                                                                Path(DCCSI_PATH,
                                                                     "config.py"))
    _dccsi_config = importlib.util.module_from_spec(_spec_dccsi_config)
    _spec_dccsi_config.loader.exec_module(_dccsi_config)

    return _dccsi_config
# -------------------------------------------------------------------------

# set and retreive the base env context/_settings on import
_config = get_dccsi_config()
_settings = _config.get_config_settings()

if _DCCSI_DEV_MODE:
    _config.attach_debugger()  # attempts to start debugger
# done with basic setup
# --- END -----------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""
    
    # -------------------------------------------------------------------------
    _O3DE_RUNNING=None    
    try:
        import azlmbr
        _O3DE_RUNNING=True
    except:
        _O3DE_RUNNING=False
    # -------------------------------------------------------------------------     

    _MODULENAME = __name__
    if _MODULENAME is '__main__':
        _MODULENAME = 'O3DE.DCCsi.bootstrap'
    
    from azpy.constants import STR_CROSSBAR

    # module internal debugging flags
    while 0: # temp internal debug flag
        _DCCSI_GDEBUG = True
        break
    
    # overide logger for standalone to be more verbose and log to file
    import azpy
    _LOGGER = azpy.initialize_logger(_MODULENAME,
                                     log_to_file=_DCCSI_GDEBUG,
                                     default_log_level=_DCCSI_LOGLEVEL)
    # happy print
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('~ constants.py ... Running script as __main__')
    _LOGGER.info(STR_CROSSBAR)
    
    # parse the command line args
    import argparse
    parser = argparse.ArgumentParser(
        description='O3DE DCCsi Boostrap (Test)',
        epilog="Will externally test the DCCsi boostrap")
    
    _config = get_dccsi_config()
    _settings = _config.get_config_settings(enable_o3de_python=True,
                                           enable_o3de_pyside2=True)
    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        help='Enables global debug flag.')
    parser.add_argument('-dm', '--developer-mode',
                        type=bool,
                        required=False,
                        help='Enables dev mode for early auto attaching debugger.')
    parser.add_argument('-tp', '--test-pyside2',
                        type=bool,
                        required=False,
                        help='Runs Qt/PySide2 tests and reports.')
    args = parser.parse_args()
    
    # easy overrides
    if args.global_debug:
        _DCCSI_GDEBUG = True
    if args.developer_mode:
        _DCCSI_DEV_MODE = True
        _config.attach_debugger()  # attempts to start debugger
    
    if _DCCSI_GDEBUG:
        _LOGGER.info(f'DCCSI_PATH: {_settings.DCCSI_PATH}')
        _LOGGER.info(f'DCCSI_G_DEBUG: {_settings.DCCSI_GDEBUG}')
        _LOGGER.info(f'DCCSI_DEV_MODE: {_settings.DCCSI_DEV_MODE}')
    
        _LOGGER.info(f'DCCSI_OS_FOLDER: {_settings.DCCSI_OS_FOLDER}')
        _LOGGER.info(f'O3DE_PROJECT: {_settings.O3DE_PROJECT}')
        _LOGGER.info(f'O3DE_PROJECT_PATH: {_settings.O3DE_PROJECT_PATH}')
        _LOGGER.info(f'O3DE_DEV: {_settings.O3DE_DEV}')
        _LOGGER.info(f'O3DE_BUILD_PATH: {_settings.O3DE_BUILD_PATH}')
        _LOGGER.info(f'O3DE_BIN_PATH: {_settings.O3DE_BIN_PATH}')
        
        _LOGGER.info(f'DCCSI_PATH: {_settings.DCCSI_PATH}')
        _LOGGER.info(f'DCCSI_PYTHON_LIB_PATH: {_settings.DCCSI_PYTHON_LIB_PATH}')
        _LOGGER.info(f'DCCSI_PY_BASE: {_settings.DCCSI_PY_BASE}')

    if _DCCSI_GDEBUG or args.test_pyside2:
        try:
            import PySide2
        except:
            # set up Qt/PySide2 access and test
            _settings = _config.get_config_settings(enable_o3de_pyside2=True)
            import PySide2

        _LOGGER.info(f'PySide2: {PySide2}')
        _LOGGER.info(f'O3DE_BIN_PATH: {_settings.O3DE_BIN_PATH}')
        _LOGGER.info(f'QT_PLUGIN_PATH: {_settings.QT_PLUGIN_PATH}')
        _LOGGER.info(f'QT_QPA_PLATFORM_PLUGIN_PATH: {_settings.QT_QPA_PLATFORM_PLUGIN_PATH}')
        
        _config.test_pyside2()
       
    if not _O3DE_RUNNING: 
        # return
        sys.exit()        
# --- END -----------------------------------------------------------------
