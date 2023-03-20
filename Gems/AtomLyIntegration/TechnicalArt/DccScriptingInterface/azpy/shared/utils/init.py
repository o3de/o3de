# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# -------------------------------------------------------------------------
"""! @brief
<DCCsi>/azpy/shared/utils/__init__.py

This module moves some utility methods out of __init__ files so we can reuse
and reduce boiler plate code copied across modules.
"""
# --------------------------------------------------------------------------
# standard imports
import sys
import errno
import os
import site
import os.path
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------

# There are many __init__ files throughout the dccsi that need to be refactored
# and updated to share this module for common utils.

# -------------------------------------------------------------------------
# Global Scope
_MODULENAME = 'azpy.shared.utils.init'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))

__all__ = ['makedirs',
           'FileExistsError',
           'initialize_logger',
           'test_imports']

# to refactor things into this module, some of this was brought along
# and this could be streamlined in a future refactor pass to slim down
# we need to set up basic access to the DCCsi
_MODULE_PATH = os.path.realpath(__file__)  # To Do: what if frozen?
_PATH_DCCSIG = os.path.normpath(os.path.join(_MODULE_PATH, '../..'))
_PATH_DCCSIG = os.getenv('PATH_DCCSIG', _PATH_DCCSIG)
site.addsitedir(_PATH_DCCSIG)

import azpy.constants as constants

# For now, use dccsi as the default project location6
# This could be improved by deriving the project from o3de and/or managed settings
_PATH_O3DE_PROJECT = Path(os.getenv(constants.ENVAR_PATH_O3DE_PROJECT, _PATH_DCCSIG))
_LOGGER.debug('Default PATH_O3DE_PROJECT" {}'.format(_PATH_O3DE_PROJECT.resolve()))

from azpy.config_utils import ENVAR_DCCSI_GDEBUG
from azpy.env_bool import env_bool
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)

# project cache log dir path
from azpy.constants import TAG_DCCSI_NICKNAME
from azpy.constants import PATH_DCCSI_LOG_PATH
_DCCSI_LOG_PATH = Path(PATH_DCCSI_LOG_PATH.format(PATH_O3DE_PROJECT=_PATH_O3DE_PROJECT.resolve(),
                                                  TAG_DCCSI_NICKNAME=TAG_DCCSI_NICKNAME))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# This method could be updated for py3+ to use pathlib
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
# This method we intend to replace with a logging class/module in the future
def initialize_logger(name,
                      log_to_file=False,
                      default_log_level=_logging.NOTSET,
                      propagate=False):
    """Start a azpy logger"""
    _logger = _logging.getLogger(name)
    _logger.propagate = propagate
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
def test_imports(_all=__all__, _pkg=_MODULENAME, _logger=_LOGGER):

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
