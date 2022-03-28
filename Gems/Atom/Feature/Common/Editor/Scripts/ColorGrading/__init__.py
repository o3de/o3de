# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""O3DE/Atom Color Grading Python Package"""
# -------------------------------------------------------------------------
import sys
import os
import site
import inspect
import logging as _logging
import pathlib
from pathlib import Path
import math

# -------------------------------------------------------------------------
__copyright__ = "Copyright 2021, Amazon"
__credits__ = ["Jonny Galloway", "Ben Black"]
__license__ = "EULA"
__version__ = "0.0.1"
__status__ = "Prototype"

__all__ = ['from_3dl_to_azasset',
           'capture_displaymapperpassthrough',
           'exr_to_3dl_azasset',
           'initialize',
           'lut_compositor',
           'lut_helper',
           'env_bool',
           'makedirs',
           'initialize_logger'
           'inv_shaper_transform',
           'shaper_transform',
           'get_uv_coord',
           'clamp',
           'log2',
           'is_power_of_two']
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_MODULE_PATH = Path(os.path.abspath(__file__))
site.addsitedir(_MODULE_PATH.parent.absolute())

_PACKAGENAME = 'ColorGrading'
_MODULEENAME = 'ColorGrading.init'

# project cache log dir path
DCCSI_LOG_PATH = Path(_MODULE_PATH.parent.absolute(), '.tmp', 'logs')
# -------------------------------------------------------------------------


# -- envar util ----------------------------------------------------------
def env_bool(envar, default=False):
    """cast a env bool to a python bool"""
    envar_test = os.getenv(envar, default)
    # check 'False', 'false', and '0' since all are non-empty
    # env comes back as string and normally coerced to True.
    if envar_test in ('True', 'true', '1'):
        return True
    elif envar_test in ('False', 'false', '0'):
        return False
    else:
        return envar_test
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# set these true if you want them set globally for debugging
DCCSI_GDEBUG = env_bool('DCCSI_GDEBUG', False)
DCCSI_DEV_MODE = env_bool('DCCSI_DEV_MODE', False)
DCCSI_GDEBUGGER = env_bool('DCCSI_GDEBUGGER', False)
DCCSI_LOGLEVEL = env_bool('DCCSI_LOGLEVEL', int(20))
DCCSI_WING_VERSION_MAJOR = env_bool('DCCSI_WING_VERSION_MAJOR', '7')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"

def initialize_logger(name,
                      log_to_file=False,
                      default_log_level=_logging.NOTSET):
    """Start a azpy logger"""
    _logger = _logging.getLogger(name)
    _logger.propagate = False
    if not _logger.handlers:

        # default log level
        _log_level = int(os.getenv('DCCSI_LOGLEVEL', default_log_level))
        
        if DCCSI_GDEBUG:
            _log_level = int(10)  # force when debugging

        if _log_level:
            ch = _logging.StreamHandler(sys.stdout)
            ch.setLevel(_log_level)
            formatter = _logging.Formatter(FRMT_LOG_LONG)
            ch.setFormatter(formatter)
            _logger.addHandler(ch)
            _logger.setLevel(_log_level)
        else:
            _logger.addHandler(_logging.NullHandler())

        _logger.debug('_log_level: {}'.format(_log_level))

    # optionally add the log file handler (off by default)
    if log_to_file:
        _logger.info('DCCSI_LOG_PATH: {}'.format(DCCSI_LOG_PATH))
        try:
            # exist_ok, isn't available in py2.7 pathlib
            # because it doesn't exist for os.makedirs
            # pathlib2 backport used instead (see above)
            if sys.version_info.major >= 3:
                DCCSI_LOG_PATH.mkdir(parents=True, exist_ok=True)
            else:
                makedirs(str(DCCSI_LOG_PATH.resolve())) # py2.7
        except FileExistsError:
            # except FileExistsError: doesn't exist in py2.7
            _logger.debug("Folder is already there")
        else:
            _logger.debug("Folder was created")

        _log_filepath = Path(DCCSI_LOG_PATH, '{}.log'.format(name))
        try:
            _log_filepath.touch(mode=0o666, exist_ok=True)
        except FileExistsError:
            _logger.debug("Log file is already there: {}".format(_log_filepath))
        else:
            _logger.debug("Log file was created: {}".format(_log_filepath))

        if _log_filepath.exists():
            file_formatter = _logging.Formatter(FRMT_LOG_LONG)
            file_handler = _logging.FileHandler(str(_log_filepath))
            file_handler.setLevel(_logging.DEBUG)
            file_handler.setFormatter(file_formatter)
            _logger.addHandler(file_handler)

    return _logger
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# _ROOT_LOGGER = _logging.getLogger()  # only use this if debugging
# https://stackoverflow.com/questions/56733085/how-to-know-the-current-file-path-after-being-frozen-into-an-executable-using-cx/56748839
#os.chdir(os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))))
# -------------------------------------------------------------------------
if DCCSI_GDEBUG:
    DCCSI_LOGLEVEL = int(10)

# set up logger with both console and file _logging
if DCCSI_GDEBUG:
    _LOGGER = initialize_logger(_MODULEENAME, log_to_file=True)
else:
    _LOGGER = initialize_logger(_MODULEENAME, log_to_file=False)

_LOGGER.debug('Invoking __init__.py for {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_datadir() -> pathlib.Path:
    """
    persistent application data.
    # linux: ~/.local/share
    # macOS: ~/Library/Application Support
    # windows: C:/Users/<USER>/AppData/Roaming
    """

    home = pathlib.Path.home()

    if sys.platform == "win32":
        return home / "AppData/Roaming"
    elif sys.platform == "linux":
        return home / ".local/share"
    elif sys.platform == "darwin":
        return home / "Library/Application Support"
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


# ------------------------------------------------------------------------
# commonly reused utilities
AZASSET_LUT = """\
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "LookupTableAsset",
    "ClassData": {
        "Name": "LookupTable",
        "Intervals": [%s],
        "Values": [%s]
    }
}"""

FLOAT_EPSILON = sys.float_info.epsilon

# pow(f, e) won't work if f is negative, or may cause inf / NAN.
def no_nan_pow(base, power):
    return pow(max(abs(base), (FLOAT_EPSILON, FLOAT_EPSILON, FLOAT_EPSILON)), power)

# Transform from high dynamic range to normalized
def inv_shaper_transform(bias, scale, v):
    return pow(2.0, (v - bias) / scale)

# Transform from normalized range to high dynamic range
def shaper_transform(bias, scale, v, clip_threshold=FLOAT_EPSILON):
    if v > 0.0:
        return math.log(v, 2.0) * scale + bias

    # this is probably not correct, clamps, avoids a math domain error
    elif v <= 0.0:
        _LOGGER.debug(f'Clipping a value: bias={bias}, scale={scale}, v={v}')
        return math.log(clip_threshold, 2.0) * scale + bias

def get_uv_coord(size, r, g, b):
    u = g * size + r
    v = b
    return (u, v)

def clamp(v):
    return max(0.0, min(1.0, v))

def log2(x):
    return (math.log10(x) /
            math.log10(2))

def is_power_of_two(n):
    return (math.ceil(log2(n)) == math.floor(log2(n)))
# ------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':

    del _LOGGER