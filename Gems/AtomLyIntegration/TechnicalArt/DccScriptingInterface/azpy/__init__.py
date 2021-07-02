# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------
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
_ORG_TAG = 'Amazon_Lumberyard'
_APP_TAG = 'DCCsi'
_TOOL_TAG = 'azpy'
_TYPE_TAG = 'module'

_PACKAGENAME = _TOOL_TAG

__all__ = ['config_utils', 'render',
           'constants', 'return_stub', 'synthetic_env',
           'env_base', 'env_bool', 'test', 'dev',
           'lumberyard', 'marmoset']  # 'blender', 'maya', 'substance', 'houdini']
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# _ROOT_LOGGER = _logging.getLogger()  # only use this if debugging
# https://stackoverflow.com/questions/56733085/how-to-know-the-current-file-path-after-being-frozen-into-an-executable-using-cx/56748839
#os.chdir(os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))))
# -------------------------------------------------------------------------
#  global space
# we need to set up basic access to the DCCsi
_MODULE_PATH = os.path.realpath(__file__)  # To Do: what if frozen?
_DCCSIG_PATH = os.path.normpath(os.path.join(_MODULE_PATH, '../..'))
_DCCSIG_PATH = os.getenv('DCCSIG_PATH', _DCCSIG_PATH)
site.addsitedir(_DCCSIG_PATH)

# azpy
import azpy.return_stub as return_stub
import azpy.env_bool as env_bool
import azpy.constants as constants
import azpy.config_utils as config_utils

_G_DEBUG = env_bool.env_bool(constants.ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool.env_bool(constants.ENVAR_DCCSI_DEV_MODE, False)

# for py2.7 (Maya) we provide this, so we must assume some bootstrapping
# has occured, see DccScriptingInterface\\config.py (_DCCSI_PYTHON_LIB_PATH)

try:
    import pathlib
except:
    import pathlib2 as pathlib
from pathlib import Path
if _G_DEBUG:
    print('DCCsi debug breadcrumb, pathlib is: {}'.format(pathlib))

# to be continued...

# get/set the project name
_LY_DEV = os.getenv(constants.ENVAR_LY_DEV,
                    config_utils.get_stub_check_path(in_path=os.getcwd(),
                                                     check_stub='engine.json'))

# get/set the project name
_LY_PROJECT_NAME = os.getenv(constants.ENVAR_LY_PROJECT,
                            config_utils.get_current_project().name)

# project cache log dir path
_DCCSI_LOG_PATH = Path(os.getenv(constants.ENVAR_DCCSI_LOG_PATH,
                                 Path(_LY_DEV,
                                      _LY_PROJECT_NAME,
                                      'Cache',
                                     'pc', 'user', 'log', 'logs')))


for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)

# very basic root logger for early debugging, flip to while 1:
while 0:
    _logging.basicConfig(level=_logging.DEBUG,
                         format=constants.FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')

    _logging.debug('azpy.rootlogger> root logger set up for debugging')  # root logger
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
                      default_log_level=_logging.NOTSET):
    """Start a azpy logger"""
    _logger = _logging.getLogger(name)
    _logger.propagate = False
    if not _logger.handlers:

        _log_level = int(os.getenv('DCCSI_LOGLEVEL', default_log_level))
        if _G_DEBUG:
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
        try:
            # exist_ok, isn't available in py2.7 pathlib
            # because it doesn't exist for os.makedirs
            # pathlib2 backport used instead (see above)
            if sys.version_info.major >= 3:
                _DCCSI_LOG_PATH.mkdir(parents=True, exist_ok=True)
            else:
                makedirs(str(_DCCSI_LOG_PATH.resolve())) # py2.7
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
# set up logger with both console and file _logging
if _G_DEBUG:
    _LOGGER = initialize_logger(_PACKAGENAME, log_to_file=True)
else:
    _LOGGER = initialize_logger(_PACKAGENAME, log_to_file=False)

_LOGGER.debug('Invoking __init__.py for {0}.'.format({_PACKAGENAME}))

# some simple logger tests
# evoke the filehandlers and test writting to the log file
if _G_DEBUG:
    _LOGGER.info('Forced Info! for {0}.'.format({_PACKAGENAME}))
    _LOGGER.error('Forced ERROR! for {0}.'.format({_PACKAGENAME}))

# debug breadcrumbs to check this module and used paths
_LOGGER.debug('MODULE_PATH: {}'.format(_MODULE_PATH))
_LOGGER.debug('LY_DEV_PATH: {}'.format(_LY_DEV))
_LOGGER.debug('DCCSI_PATH: {}'.format(_DCCSIG_PATH))
_LOGGER.debug('LY_PROJECT_TAG: {}'.format(_LY_PROJECT_NAME))
_LOGGER.debug('DCCSI_LOG_PATH: {}'.format(_DCCSI_LOG_PATH))


# -------------------------------------------------------------------------
def test_imports(_all=__all__,
                 _pkg=_PACKAGENAME,
                 _logger=_LOGGER):
    # If in dev mode this will test imports of __all__
    _logger.debug("~   Import triggered from: {0}".format(_pkg))
    import importlib
    for pkgStr in _all:
        try:
            # this is py2.7 compatible
            # in py3.5+, we can use importlib.util instead
            importlib.import_module('.' + pkgStr, _pkg)
            _logger.debug("~       Imported module: {0}".format(pkgStr))
        except Exception as e:
            _logger.warning('~       {0}'.format(e))
            _logger.warning("~       {0} :: ImportFail".format(pkgStr))
            return False
    return True
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
if _DCCSI_DEV_MODE:
    # If in dev mode this will test imports of __all__
    _LOGGER.debug('Testing Imports: {0}'.format(_PACKAGENAME))
    test_imports(__all__)
# -------------------------------------------------------------------------

del _LOGGER

###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    _G_DEBUG = True
    _DCCSI_DEV_MODE = True

    if _G_DEBUG:
        print(_DCCSIG_PATH)
        test_imports()
