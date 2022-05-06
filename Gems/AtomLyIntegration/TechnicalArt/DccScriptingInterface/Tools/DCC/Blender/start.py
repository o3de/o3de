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
    < DCCsi >:: Tools//DCC//Blender//start.py

This module is used to start blender with O3DE bootstrapping
"""
# -------------------------------------------------------------------------
# standard imports
import sys
import os
import site
import timeit
import inspect
import subprocess
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
#os.environ['PYTHONINSPECT'] = 'True'
_START = timeit.default_timer() # start tracking

# global scope
_MODULENAME = 'Tools.DCC.Blender.start'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))

#os.environ['PYTHONINSPECT'] = 'True'
_MODULE_PATH = os.path.abspath(__file__)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Maya is frozen
#_MODULE_PATH = Path(__file__)
# https://tinyurl.com/y49t3zzn
# module path when frozen
_MODULE_PATH = Path(os.path.abspath(inspect.getfile(inspect.currentframe())))
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH.as_posix()))

_PATH_DCCSI_TOOLS_BLENDER = Path(_MODULE_PATH.parent)
_PATH_DCCSI_TOOLS_BLENDER = Path(os.getenv('PATH_DCCSI_TOOLS_BLENDER',
                                           _PATH_DCCSI_TOOLS_BLENDER.as_posix()))

# we don't have access yet to the DCCsi Lib\site-packages
# (1) this will give us import access to dccsi.azpy (always?)
# (2) this allows it to be set as ENVAR
_PATH_DCCSIG = Path(_PATH_DCCSI_TOOLS_BLENDER.parent.parent.parent)
_PATH_DCCSIG = Path(os.getenv('PATH_DCCSIG', _PATH_DCCSIG.as_posix()))
site.addsitedir(_PATH_DCCSIG.as_posix())

from Tools.DCC.Blender.constants import STR_PATH_DCCSI_PYTHON_LIB

# override based on current executable
_PATH_DCCSI_PYTHON_LIB = STR_PATH_DCCSI_PYTHON_LIB.format(_PATH_DCCSIG,
                                                         sys.version_info.major,
                                                         sys.version_info.minor)
_PATH_DCCSI_PYTHON_LIB = Path(_PATH_DCCSI_PYTHON_LIB)
site.addsitedir(_PATH_DCCSI_PYTHON_LIB.as_posix())

# import others
from Tools.DCC.Blender.constants import DCCSI_BLENDER_EXE
from Tools.DCC.Blender.constants import DCCSI_BLENDER_LAUNCHER
from Tools.DCC.Blender.constants import DCCSI_BLENDER_PY_EXE
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------


# --- END -----------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main (external commandline)"""
    
    #os.environ['PYTHONINSPECT'] = 'True'
    
    STR_CROSSBAR = f"{'-' * 74}"
    
    _DCCSI_GDEBUG = False
    _DCCSI_DEV_MODE = False
    
    # default loglevel to info unless set
    _DCCSI_LOGLEVEL = _logging.INFO
    
    if _DCCSI_GDEBUG:
        # override loglevel if runnign debug
        _DCCSI_LOGLEVEL = _logging.DEBUG
        
    # set up module logging
    #for handler in _logging.root.handlers[:]:
        #_logging.root.removeHandler(handler)
        
    FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
    
    # configure basic logger
    # note: not using a common logger to reduce cyclical imports
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                         format=FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')
    
    _LOGGER = _logging.getLogger(_MODULENAME)

    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
    _LOGGER.debug('_DCCSI_GDEBUG: {}'.format(_DCCSI_GDEBUG))
    _LOGGER.debug('_DCCSI_DEV_MODE: {}'.format(_DCCSI_DEV_MODE))
    _LOGGER.debug('_DCCSI_LOGLEVEL: {}'.format(_DCCSI_LOGLEVEL))
    
    # get the settings for the environment
    from dynaconf import settings
    
    # commandline interface
    import argparse
    parser = argparse.ArgumentParser(
        description='O3DE DCCsi.Tools.DCC.Blender.start',
        epilog="Attempts to start Blender with the DCCsi and O3DE bootstrapping")
    
    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables global debug flag.')
    
    parser.add_argument('-dm', '--developer-mode',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables dev mode for early auto attaching debugger.')
    
    parser.add_argument('-sd', '--set-debugger',
                        type=str,
                        required=False,
                        default='WING',
                        help='Default debugger: WING, (not implemented) others: PYCHARM and VSCODE.')
    
    parser.add_argument('-be', '--blender-executable',
                        type=str,
                        required=False,
                        default='Blender', 
                        help="Name of exe to start, options are: 'Blender', 'Launcher' (aka Blender-Launcher.exe),'Python' (blenders python) ")
    
    parser.add_argument('-ex', '--exit',
                        type=bool,
                        required=False,
                        default=False,
                        help='Exits python. Do not exit if you want to be in interactive interpretter after config')
    
    args = parser.parse_args()

    # easy overrides
    if args.global_debug:
        _DCCSI_GDEBUG = True
        os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)
        
    if args.developer_mode:
        _DCCSI_DEV_MODE = True
        attach_debugger()  # attempts to start debugger

    if args.set_debugger:
        _LOGGER.info('Setting and switching debugger type not implemented (default=WING)')
        # To Do: implement debugger plugin pattern
    
    if args.blender_executable:
        if args.blender_executable == 'Blender':
            subprocess.Popen(DCCSI_BLENDER_EXE, env=os.environ.copy(), shell=True)
            
        elif args.blender_executable == 'Launcher':
            _LOGGER.warn(f'Not Implemented Yet')
            
        elif args.blender_executable == 'Python':
            _LOGGER.warn(f'Not Implemented Yet')    
        
        else:
            _LOGGER.error(f'Specified option {args.blender_executable}, is not supported!')
        
    # -- DONE ----
    _LOGGER.info(STR_CROSSBAR)
    
    _LOGGER.debug('{0} took: {1} sec'.format(_MODULENAME, timeit.default_timer() - _START))

    if args.exit:
        import sys
        # return
        sys.exit()
    else:
        # custom prompt
        sys.ps1 = "[{}]>>".format(_MODULENAME)
# --- END -----------------------------------------------------------------