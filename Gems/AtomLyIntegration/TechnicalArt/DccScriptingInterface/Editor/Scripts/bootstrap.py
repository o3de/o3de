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

# test bootstrap execution time
import time
_START = time.process_time() # start tracking

# standard imports
import sys
import os
import site
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_MODULENAME = 'O3DE.DCCsi.bootstrap'

# we don't use dynaconf setting here as we might not yet have access
# we need to set up basic access to the DCCsi
_MODULE_PATH = os.path.realpath(__file__)  # To Do: what if frozen?
_PATH_DCCSIG = Path(os.path.join(_MODULE_PATH, '../../..'))
site.addsitedir(_PATH_DCCSIG)

# set envar so DCCsi synthetic env bootstraps with it (config.py)
from azpy.constants import ENVAR_PATH_DCCSIG
os.environ[ENVAR_PATH_DCCSIG] = str(_PATH_DCCSIG.resolve())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# now we have azpy api access
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG

from azpy.env_bool import env_bool
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')

# default loglevel to info unless set
_DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))
if _DCCSI_GDEBUG:
    # override loglevel if runnign debug
    _DCCSI_LOGLEVEL = _logging.DEBUG
    
# set up module logging
#for handler in _logging.root.handlers[:]:
    #_logging.root.removeHandler(handler)
    
# configure basic logger
# note: not using a common logger to reduce cyclical imports
_logging.basicConfig(level=_DCCSI_LOGLEVEL,
                    format=FRMT_LOG_LONG,
                    datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def attach_debugger():
    from azpy.test.entry_test import connect_wing
    _debugger = connect_wing()
    return _debugger
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# _settings.setenv()  # doing this will add the additional DYNACONF_ envars
import importlib.util
def get_dccsi_config(PATH_DCCSIG=_PATH_DCCSIG):
    """Convenience method to set and retreive settings directly from module."""
    
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
# bootstrap in AssetProcessor
def bootstrap_Editor(test_config=_DCCSI_GDEBUG,
                     test_pyside2=_DCCSI_GDEBUG):
    '''Put boostrapping code here to execute in O3DE Editor.exe'''
    
    _settings = None

    if test_config:
        # set and retreive the base env context/_settings on import
        _config = get_dccsi_config()
        _settings = _config.get_config_settings(enable_o3de_python=True,
                                               enable_o3de_pyside2=True)
        # note: this can impact start up times so currently we are only
        # running it to test.
        # To Do: slim down start up times so we can init _settings
    
    if test_pyside2:
        _config.test_pyside2()
    
    return _settings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# bootstrap in AssetProcessor
def bootstrap_MaterialEditor():
    '''Put boostrapping code here to execute in O3DE MaterialEdito.exe'''
    pass
    return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# bootstrap in AssetProcessor
def bootstrap_AssetProcessor():
    '''Put boostrapping code here to execute in O3DE AssetProcessor.exe'''
    pass
    return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# bootstrap in AssetProcessor
def bootstrap_AssetBuilder():
    '''Put boostrapping code here to execute in O3DE AssetBuilder.exe'''
    pass
    return None
# -------------------------------------------------------------------------

        
# -------------------------------------------------------------------------
if _DCCSI_DEV_MODE:
    foo = attach_debugger()  # attempts to start debugger

# set and retreive the *basic* env context/_settings on import
# What application is executing the bootstrap?
# Python is being run from:
#    editor.exe
#    materialeditor.exe
#    assetprocessor.exe
#    assetbuilder.exe, or the Python executable.
# Exclude the .exe so it works on other platforms

_O3DE_Editor = Path(sys.executable)
_LOGGER.debug(f'The sys.executable is: {_O3DE_Editor}')

if _O3DE_Editor.stem.lower() == "editor":
    # if _DCCSI_GDEBUG then run the pyside2 test
    _settings = bootstrap_Editor(_DCCSI_GDEBUG)
    
elif _O3DE_Editor.stem.lower() == "materialeditor":
    _settings = bootstrap_MaterialEditor()
    
elif _O3DE_Editor.stem.lower() == "assetprocessor":
    _settings = bootstrap_AssetProcessor()
    
elif _O3DE_Editor.stem.lower() == "assetbuilder":
    _settings= bootstrap_AssetBuilder()
    
elif _O3DE_Editor.stem.lower() == "python":
    # in this case, we can re-use the editor settings
    # which will init python and pyside2 access externally
    _settings= bootstrap_Editor(_DCCSI_GDEBUG)
    
else:
    _LOGGER.warning(f'No bootstrapping code for: {_O3DE_Editor}')
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main (external commandline for testing)"""
    # ---------------------------------------------------------------------
    # force enable debug settings manually
    _DCCSI_GDEBUG = False # enable here to force temporarily
    _DCCSI_DEV_MODE = False
    if _DCCSI_GDEBUG:
        # override loglevel if runnign debug
        _DCCSI_LOGLEVEL = _logging.DEBUG
    # ---------------------------------------------------------------------
    
    
    # ---------------------------------------------------------------------
    _O3DE_RUNNING=None    
    try:
        import azlmbr
        _O3DE_RUNNING=True
    except:
        _O3DE_RUNNING=False
    # ---------------------------------------------------------------------


    # ---------------------------------------------------------------------
    # this is a simple commandline interface for running and testing externally
    if not _O3DE_RUNNING: # external tool or commandline
        
        # parse the command line args
        import argparse
        parser = argparse.ArgumentParser(
            description='O3DE DCCsi Boostrap (Test)',
            epilog="Will externally test the DCCsi boostrap")

        parser.add_argument('-gd', '--global-debug',
                            type=bool,
                            required=False,
                            help='Enables global debug flag.')
    
        parser.add_argument('-sd', '--set-debugger',
                            type=str,
                            required=False,
                            help='Default debugger: WING, others: PYCHARM, VSCODE (not yet implemented).')
        
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
            _DCCSI_LOGLEVEL = _logging.DEBUG
            _LOGGER.setLevel(_DCCSI_LOGLEVEL)

        if args.set_debugger:
            _LOGGER.info('Setting and switching debugger type not implemented (default=WING)')
            # To Do: implement debugger plugin pattern
    
        if args.developer_mode or _DCCSI_DEV_MODE:
            _DCCSI_DEV_MODE = True
            foo = attach_debugger()  # attempts to start debugger
            
        # happy print
        from azpy.constants import STR_CROSSBAR
        _LOGGER.info(STR_CROSSBAR)
        _LOGGER.info('~ DCCsi: bootstrap.py ... Running script as __main__')
        _LOGGER.info(STR_CROSSBAR)
        
        _TEST_PYSIDE2 = False
        if args.test_pyside2:
            _TEST_PYSIDE2 = True
        
        _settings= bootstrap_Editor(_DCCSI_GDEBUG, _TEST_PYSIDE2)
        
        if _DCCSI_GDEBUG:
            _LOGGER.info(f'PATH_DCCSIG: {_settings.PATH_DCCSIG}')
            _LOGGER.info(f'DCCSI_G_DEBUG: {_settings.DCCSI_GDEBUG}')
            _LOGGER.info(f'DCCSI_DEV_MODE: {_settings.DCCSI_DEV_MODE}')
        
            _LOGGER.info(f'DCCSI_OS_FOLDER: {_settings.DCCSI_OS_FOLDER}')
            _LOGGER.info(f'O3DE_PROJECT: {_settings.O3DE_PROJECT}')
            _LOGGER.info(f'PATH_O3DE_PROJECT: {_settings.PATH_O3DE_PROJECT}')
            _LOGGER.info(f'O3DE_DEV: {_settings.O3DE_DEV}')
            _LOGGER.info(f'PATH_O3DE_BUILD: {_settings.PATH_O3DE_BUILD}')
            _LOGGER.info(f'PATH_O3DE_BIN: {_settings.PATH_O3DE_BIN}')
            
            _LOGGER.info(f'PATH_DCCSIG: {_settings.PATH_DCCSIG}')
            _LOGGER.info(f'PATH_DCCSI_PYTHON_LIB: {_settings.PATH_DCCSI_PYTHON_LIB}')
            _LOGGER.info(f'DCCSI_PY_BASE: {_settings.DCCSI_PY_BASE}')

        if args.test_pyside2:
            _LOGGER.info(f'PySide2: {PySide2}')
            _LOGGER.info(f'PATH_O3DE_BIN: {_settings.PATH_O3DE_BIN}')
            _LOGGER.info(f'QT_PLUGIN_PATH: {_settings.QT_PLUGIN_PATH}')
            _LOGGER.info(f'QT_QPA_PLATFORM_PLUGIN_PATH: {_settings.QT_QPA_PLATFORM_PLUGIN_PATH}')
    # ---------------------------------------------------------------------
    
    # custom prompt
    sys.ps1 = "[azpy]>>"
    
_END_TIME = time.process_time() - _START
_LOGGER.debug('~ DCCsi: bootstrap.py took: {_END_TIME} sec') 
# --- END -----------------------------------------------------------------
