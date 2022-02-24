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
<DCCsi>/Tools/__init__.py

This init allows us to treat the Tools folder as a package.
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
_PACKAGENAME = 'Tools'

__all__ = ['DCC',
           'Python']  # to do: add others when they are set up
          #'Foo',
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# we need to set up basic access to the DCCsi
_MODULE_PATH = Path(__file__)  # To Do: what if frozen?

_PATH_DCCSI_TOOLS = Path(_MODULE_PATH.parent)
_PATH_DCCSI_TOOLS = Path(os.getenv('PATH_DCCSI_TOOLS',
                                       _PATH_DCCSI_TOOLS.as_posix()))
site.addsitedir(_PATH_DCCSI_TOOLS.as_posix())

_PATH_DCCSIG = Path(_PATH_DCCSI_TOOLS.parent)
_PATH_DCCSIG = Path(os.getenv('PATH_DCCSIG', _PATH_DCCSIG.as_posix()))
site.addsitedir(_PATH_DCCSIG.as_posix())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_TESTS
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG
from azpy.constants import STR_CROSSBAR

_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_TESTS = env_bool(ENVAR_DCCSI_TESTS, False)
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

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug(STR_CROSSBAR)
_LOGGER.debug('Initializing: {}'.format(_PACKAGENAME))
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH.as_posix()))
_LOGGER.debug('PATH_DCCSI_TOOLS: {}'.format(_PATH_DCCSI_TOOLS.as_posix()))
_LOGGER.debug('PATH_DCCSIG: {}'.format(_PATH_DCCSIG.as_posix()))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# To Do: import and use a shared method
def attach_debugger(debugge_typer=_DCCSI_GDEBUGGER):
    """!
    This will attemp to attch the WING debugger
    To Do: other IDEs for debugging not yet implemented.
    This should be replaced with a plugin based dev package."""
    _DCCSI_GDEBUG = True
    os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)
    
    _DCCSI_DEV_MODE = True
    os.environ["DYNACONF_DCCSI_DEV_MODE"] = str(_DCCSI_DEV_MODE)
    
    if debugge_typer == 'WING':
        from azpy.test.entry_test import connect_wing
        _debugger = connect_wing()
    else:
        _LOGGER.warning('Debugger type: {}, is Not Implemented!'.format(debugge_typer))
    
    return _debugger
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run in debug perform local tests from IDE or CLI"""
    
    # happy print
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('~ {}.py ... Running script as __main__'.format(_PACKAGENAME))
    _LOGGER.info(STR_CROSSBAR)
    
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
    
    # re-configure basic logger for debug
    _LOGGER = _logging.getLogger(_PACKAGENAME)
        
    import argparse
    parser = argparse.ArgumentParser(
        description='O3DE DCCsi Tools CLI',
        epilog="Allows for some light testing of the DCCsi/Tools structure from CLI") 
    
    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables global debug flag.')
    
    parser.add_argument('-rt', '--run-tests',
                        type=bool,
                        required=False,
                        default=False,
                        help='Runs local import and other tests.')
    
    parser.add_argument('-sd', '--set-debugger',
                        type=str,
                        required=False,
                        default='WING',
                        help='Default debugger: WING, (not implemented) others: PYCHARM and VSCODE.')
    
    parser.add_argument('-dm', '--developer-mode',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables dev mode for early auto attaching debugger.')  
    
    parser.add_argument('-ex', '--exit',
                        type=bool,
                        required=False,
                        default=False,
                        help='Exits python. Do not exit if you want to be in interactive interpretter after config')
    
    args = parser.parse_args()

    # easy overrides
    if args.global_debug:
        _DCCSI_GDEBUG = True
        os.environ["DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)

    if args.set_debugger:
        _LOGGER.info('Setting and switching debugger type not implemented yet (default=WING)')
        # To Do: implement debugger plugin pattern
        
    if args.developer_mode:
        _DCCSI_DEV_MODE = True
        os.environ["DCCSI_DEV_MODE"] = str(_DCCSI_DEV_MODE)
        attach_debugger()  # attempts to start debugger     
    
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('_MODULE_PATH: {}'.format(_MODULE_PATH.as_posix()))
    _LOGGER.info('PATH_DCCSI_TOOLS: {}'.format(_PATH_DCCSI_TOOLS.as_posix()))
    _LOGGER.info('PATH_DCCSIG: {}'.format(_PATH_DCCSIG.as_posix()))
    _LOGGER.info('_DCCSI_GDEBUG: {}'.format(_DCCSI_GDEBUG))
    _LOGGER.info('_DCCSI_TESTS: {}'.format(_DCCSI_TESTS))
    _LOGGER.info('_DCCSI_DEV_MODE: {}'.format(_DCCSI_DEV_MODE))
    _LOGGER.info('_DCCSI_LOGLEVEL: {}'.format(_DCCSI_LOGLEVEL))
    
    if args.run_tests:
        _DCCSI_TESTS = True
        os.environ["DCCSI_TESTS"] = str(_DCCSI_TESTS)
        
        _LOGGER.info('TESTS ENABLED, _DCCSI_TESTS: True')
        _LOGGER.info('Tests are recursive package/module imports and will be verbose!')
        
        # If in dev mode this will test imports of __all__
        from azpy import test_imports
        
        _LOGGER.info(STR_CROSSBAR)
        
        _LOGGER.info('Testing Imports from {0}'.format(_PACKAGENAME))
        test_imports(__all__,
                     _pkg=_PACKAGENAME)
        
    # -- DONE ----
    _LOGGER.info(STR_CROSSBAR)

    if args.exit:
        import sys
        # return
        sys.exit()
# -------------------------------------------------------------------------