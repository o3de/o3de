# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# --------------------------------------------------------------------------
"""! @brief
<DCCsi>/azpy/__init__.py

This is the shared pure-python api.
"""
__copyright__ = "Copyright 2021, Amazon"
__credits__ = ["Jonny Galloway", "Ben Black"]
__license__ = "EULA"
__version__ = "0.0.1"
__status__ = "Prototype"
# --------------------------------------------------------------------------
# standard imports
import sys
import errno
import os
import os.path
import site
import re
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_PACKAGENAME = 'azpy'

__all__ = ['config_utils',
           'constants',
           'env_bool',
           'return_stub',
           'logger',
           'core',
           'dcc',
           'dev',
           'o3de',
           'shared',
           'test']
           #'foo' is WIP

# we need to set up basic access to the DCCsi
_MODULE_PATH = os.path.realpath(__file__)  # To Do: what if frozen?
_PATH_DCCSIG = os.path.normpath(os.path.join(_MODULE_PATH, '../..'))
_PATH_DCCSIG = os.getenv('PATH_DCCSIG', _PATH_DCCSIG)
site.addsitedir(_PATH_DCCSIG)

_PATH_DCCSI_AZPY = os.path.dirname(_MODULE_PATH)

# azpy
import azpy.return_stub as return_stub
import azpy.env_bool as env_bool
import azpy.constants as constants
import azpy.config_utils as config_utils

_DCCSI_GDEBUG = env_bool.env_bool(constants.ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool.env_bool(constants.ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_TESTS = env_bool.env_bool(constants.ENVAR_DCCSI_TESTS, False)
_DCCSI_GDEBUGGER = env_bool.env_bool(constants.ENVAR_DCCSI_GDEBUGGER, 'WING')

# default loglevel to info unless set
_DCCSI_LOGLEVEL = int(env_bool.env_bool(constants.ENVAR_DCCSI_LOGLEVEL,
                                        _logging.INFO))
if _DCCSI_GDEBUG:
    # override loglevel if runnign debug
    _DCCSI_LOGLEVEL = _logging.DEBUG

## set up module logging
#for handler in _logging.root.handlers[:]:
    #_logging.root.removeHandler(handler)
    
_logging.basicConfig(level=_DCCSI_LOGLEVEL,
                     format=constants.FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
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


# -------------------------------------------------------------------------
# for py2.7 (Maya) we provide this, so we must assume some bootstrapping
# has occured, see DccScriptingInterface\\config.py (_PATH_DCCSI_PYTHON_LIB)

try:
    import pathlib
except:
    import pathlib2 as pathlib
from pathlib import Path
if _DCCSI_GDEBUG:
    _LOGGER.debug('[DCCsi][AZPY] DEBUG BREADCRUMB, pathlib is: {}'.format(pathlib))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# default o3de engin location
_O3DE_DEV = os.path.normpath(os.path.join(_PATH_DCCSIG, '../../../..'))
_O3DE_DEV = os.getenv(constants.ENVAR_O3DE_DEV, _O3DE_DEV)
_O3DE_DEV = Path(_O3DE_DEV)
_LOGGER.debug('_O3DE_DEV" {}'.format(_O3DE_DEV.resolve()))

# use dccsi as the default project location
_PATH_O3DE_PROJECT = Path(os.getenv(constants.ENVAR_PATH_O3DE_PROJECT, _PATH_DCCSIG))
_LOGGER.debug('Default PATH_O3DE_PROJECT" {}'.format(_PATH_O3DE_PROJECT.resolve()))

# use dccsi as the default project name
_O3DE_PROJECT = str(os.getenv(constants.ENVAR_O3DE_PROJECT, _PATH_O3DE_PROJECT.name))

# project cache log dir path
from azpy.constants import TAG_DCCSI_NICKNAME
from azpy.constants import PATH_DCCSI_LOG_PATH
_DCCSI_LOG_PATH = Path(PATH_DCCSI_LOG_PATH.format(PATH_O3DE_PROJECT=_PATH_O3DE_PROJECT.resolve(),
                                                  TAG_DCCSI_NICKNAME=TAG_DCCSI_NICKNAME))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def makedirs(folder, *args, **kwargs):
    """a makedirs for py2.7 support"""
    try:
        return os.makedirs(folder, exist_ok=True, *args, **kwargs)
    except TypeError: 
        # Unexpected arguments encountered 
        pass

    try:
        # Should work is TypeError was caused by exist_ok, eg., Py2
        return os.makedirs(folder, *args, **kwargs)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise

        if os.path.isfile(folder):
            # folder is a file, raise OSError just like os.makedirs() in Py3
            raise
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class FileExistsError(Exception):
    """Implements a stand-in Exception for py2.7"""
    def __init__(self, message, errors):

        # Call the base class constructor with the parameters it needs
        super(FileExistsError, self).__init__(message)

        # Now for your custom code...
        self.errors = errors

if sys.version_info.major < 3:
    FileExistsError = FileExistsError
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def initialize_logger(name,
                      log_to_file=False,
                      default_log_level=_logging.NOTSET,
                      propogate=False):
    """Start a azpy logger"""
    _logger = _logging.getLogger(name)
    _logger.propagate = propogate
    if not _logger.handlers:

        _log_level = int(os.getenv('DCCSI_LOGLEVEL', default_log_level))
        if _DCCSI_GDEBUG:
            _log_level = int(10)  # force when debugging
            print('_log_level: {}'.format(_log_level))

        if _log_level:
            ch = _logging.StreamHandler(sys.stdout)
            ch.setLevel(_log_level)
            formatter = _logging.Formatter(constants.FRMT_LOG_LONG)
            ch.setFormatter(formatter)
            _logger.addHandler(ch)
            _logger.setLevel(_log_level)
        else:
            _logger.addHandler(_logging.NullHandler())

    # optionally add the log file handler (off by default)
    if log_to_file:
        _logger.info('DCCSI_LOG_PATH: {}'.format(_DCCSI_LOG_PATH))
        if not _DCCSI_LOG_PATH.exists():  
            try:
                os.makedirs(_DCCSI_LOG_PATH.as_posix())
            except FileExistsError:
                # except FileExistsError: doesn't exist in py2.7
                _logger.debug("Folder is already there")
            else:
                _logger.debug("Folder was created")

        _log_filepath = Path(_DCCSI_LOG_PATH, '{}.log'.format(name))
        
        try:
            _log_filepath.touch(mode=0o666, exist_ok=True)
        except FileExistsError:
            _logger.debug("Log file is already there: {}".format(_log_filepath))
        else:
            _logger.debug("Log file was created: {}".format(_log_filepath))

        if _log_filepath.exists():
            file_formatter = _logging.Formatter(constants.FRMT_LOG_LONG)
            file_handler = _logging.FileHandler(str(_log_filepath))
            file_handler.setLevel(_logging.DEBUG)
            file_handler.setFormatter(file_formatter)
            _logger.addHandler(file_handler)

    return _logger
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# some simple logger tests
# evoke the filehandlers and test writting to the log file
if _DCCSI_GDEBUG:
    _LOGGER.info('Forced Info! for {0}.'.format({_PACKAGENAME}))
    _LOGGER.error('Forced ERROR! for {0}.'.format({_PACKAGENAME}))

# debug breadcrumbs to check this module and used paths
_LOGGER.debug('MODULE_PATH: {}'.format(_MODULE_PATH))
_LOGGER.debug('O3DE_DEV_PATH: {}'.format(_O3DE_DEV))
_LOGGER.debug('PATH_DCCSIG: {}'.format(_PATH_DCCSIG))
_LOGGER.debug('O3DE_PROJECT_TAG: {}'.format(_O3DE_PROJECT))
_LOGGER.debug('DCCSI_LOG_PATH: {}'.format(_DCCSI_LOG_PATH))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def test_imports(_all=__all__, _pkg=_PACKAGENAME, _logger=_LOGGER):
    
    # If in dev mode this will test imports of __all__
    _logger.debug("~   Import triggered from: {}".format(_pkg))
    import importlib
    for mod_str in _all:
        try:
            # this is py2.7 compatible
            # in py3.5+, we can use importlib.util instead
            importlib.import_module('.{}'.format(mod_str), _pkg)
            _logger.info("~       Imported module: {0}.{1}".format(_pkg, mod_str))
        except Exception as e:
            _logger.warning('~       {}'.format(e))
            _logger.warning("~       {0}.{1} :: ImportFail".format(_pkg, mod_str))
            return False
    return True
# -------------------------------------------------------------------------


##########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run in debug perform local tests from IDE or CLI"""
    
    # happy print
    _LOGGER.info(constants.STR_CROSSBAR)
    _LOGGER.info('~ {}.py ... Running script as __main__'.format(_PACKAGENAME))
    _LOGGER.info(constants.STR_CROSSBAR)
    
    # default loglevel to info unless set
    _DCCSI_LOGLEVEL = int(env_bool.env_bool(constants.ENVAR_DCCSI_LOGLEVEL,
                                            _logging.INFO))
    if _DCCSI_GDEBUG:
        # override loglevel if runnign debug
        _DCCSI_LOGLEVEL = _logging.DEBUG
        
    # set up module logging
    #for handler in _logging.root.handlers[:]:
        #_logging.root.removeHandler(handler)
        
    # configure basic logger
    # note: not using a common logger to reduce cyclical imports
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                        format=constants.FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')
    
    # re-configure basic logger for debug
    _LOGGER = _logging.getLogger(_PACKAGENAME)
        
    import argparse
    parser = argparse.ArgumentParser(
        description='O3DE DCCsi azpy API CLI',
        epilog="Allows for some light testing of the API structure from CLI")  
    
    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables global debug flag.')
    
    parser.add_argument('-rt', '--run-tests',
                        type=bool,
                        required=False,
                        default=True,
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
                        default=True,
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
    
    _LOGGER.info(constants.STR_CROSSBAR)
    _LOGGER.info('_MODULE_PATH: {}'.format(_MODULE_PATH))
    _LOGGER.info('_PATH_DCCSIG: {}'.format(_PATH_DCCSIG))
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
        
        _LOGGER.info(constants.STR_CROSSBAR)
        
        _LOGGER.info('Testing Imports from {0}'.format(_PACKAGENAME))
        test_imports(__all__,
                     _pkg=_PACKAGENAME,
                     _logger=_LOGGER)
        
    # -- DONE ----
    _LOGGER.info(constants.STR_CROSSBAR)

    if args.exit:
        import sys
        # return
        sys.exit()
# -------------------------------------------------------------------------
