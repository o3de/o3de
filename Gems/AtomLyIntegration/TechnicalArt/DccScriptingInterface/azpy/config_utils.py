# coding:utf-8
#!/usr/bin/python
#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# -- This line is 75 characters -------------------------------------------
import sys
import os
import re
import site
import logging as _logging

from pathlib import Path  # note: we provide this in py2.7
# so using it here suggests some boostrapping has occured before using azpy
# --------------------------------------------------------------------------
_PACKAGENAME = 'azpy.config_utils'

FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
_logging.basicConfig(level=_logging.INFO,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

__all__ = ['get_os', 'return_stub', 'get_stub_check_path',
           'get_dccsi_config']

# note: this module should reamin py2.7 compatible (Maya) so no f'strings
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_os():
    """returns lumberyard dir names used in python path"""
    if sys.platform.startswith('win'):
        os_folder = "windows"
    elif sys.platform.startswith('darwin'):
        os_folder = "mac"
    elif sys.platform.startswith('linux'):
        os_folder = "linux_x64"
    else:
        message = str("DCCsi.azpy.config_utils.py: "
                      "Unexpectedly executing on operating system '{}'"
                      "".format(sys.platform))
        
        raise RuntimeError(message)
    return os_folder
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def return_stub_dir(stub_file='dccsi_stub'):
    _dir_to_last_file = None
    '''Take a file name (stub_file) and returns the directory of the file (stub_file)'''
    # To Do: refactor to use pathlib object oriented Path
    if _dir_to_last_file is None:
        path = os.path.abspath(__file__)
        while 1:
            path, tail = os.path.split(path)
            if (os.path.isfile(os.path.join(path, stub_file))):
                break
            if (len(tail) == 0):
                path = ""
                _LOGGER.debug('I was not able to find the path to that file '
                              '({}) in a walk-up from currnet path'
                              ''.format(stub_file))
                break
            
        _dir_to_last_file = path

    return _dir_to_last_file
# --------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_stub_check_path(in_path=os.getcwd(), check_stub='engineroot.txt'):
    '''
    Returns the branch root directory of the dev\\'engineroot.txt'
    (... or you can pass it another known stub)

    so we can safely build relative filepaths within that branch.

    If the stub is not found, it returns None
    '''
    path = Path(in_path).absolute()

    while 1:
        test_path = Path(path, check_stub)

        if test_path.is_file():
            return Path(test_path).parent

        else:
            path, tail = (path.parent, path.name)

            if (len(tail) == 0):
                return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# settings.setenv()  # doing this will add the additional DYNACONF_ envars
def get_dccsi_config(dccsi_dirpath=return_stub_dir()):
    """Convenience method to set and retreive settings directly from module."""
    
    # we can go ahead and just make sure the the DCCsi env is set
    # config is SO generic this ensures we are importing a specific one
    _module_tag = "dccsi.config"
    _dccsi_path = Path(dccsi_dirpath, "config.py")
    if _dccsi_path.exists():
        if sys.version_info.major >= 3:
            import importlib  # note: py2.7 doesn't have importlib.util
            import importlib.util
            #from importlib import util
            _spec_dccsi_config = importlib.util.spec_from_file_location(_module_tag,
                                                                        str(_dccsi_path.resolve()))
            _dccsi_config = importlib.util.module_from_spec(_spec_dccsi_config)
            _spec_dccsi_config.loader.exec_module(_dccsi_config)
        
            _LOGGER.debug('Executed config: {}'.format(_spec_dccsi_config))
        else:  # py2.x
            import imp
            _dccsi_config = imp.load_source(_module_tag, str(_dccsi_path.resolve()))
            _LOGGER.debug('Imported config: {}'.format(_spec_dccsi_config))
        return _dccsi_config
            
    else:
        return None
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
def bootstrap_dccsi_py_libs(dccsi_dirpath=return_stub_dir()):
    """Builds and adds local site dir libs based on py version"""

    from azpy.constants import STR_DCCSI_PYTHON_LIB_PATH  # a path string constructor
    _DCCSI_PYTHON_LIB_PATH = STR_DCCSI_PYTHON_LIB_PATH.format(dccsi_dirpath,
                                                              sys.version_info[0],
                                                              sys.version_info[1])

    if os.path.exists(_DCCSI_PYTHON_LIB_PATH):
        _LOGGER.debug('Performed site.addsitedir({})'.format(_DCCSI_PYTHON_LIB_PATH))
        site.addsitedir(_DCCSI_PYTHON_LIB_PATH)  # PYTHONPATH
        return _DCCSI_PYTHON_LIB_PATH
    else:
        message = "Doesn't exist: {}".format(_DCCSI_PYTHON_LIB_PATH)
        _LOGGER.error(message)
        raise NotADirectoryError(message)
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':

    # happy print
    _LOGGER.info("# {0} #".format('-' * 72))
    _LOGGER.info('~ config_utils.py ... Running script as __main__')
    _LOGGER.info("# {0} #".format('-' * 72))

    _LOGGER.info('Current Work dir: {0}'.format(os.getcwd()))
    
    _LOGGER.info('OS: {}'.format(get_os()))

    _LOGGER.info('DCCSIG_PATH: {}'.format(return_stub_dir('dccsi_stub')))
    
    _config = get_dccsi_config()
    _LOGGER.info('DCCSI_CONFIG_PATH: {}'.format(_config))

    _LOGGER.info('LY_DEV: {}'.format(get_stub_check_path('engineroot.txt')))

    _LOGGER.info('DCCSI_PYTHON_LIB_PATH: {}'.format(bootstrap_dccsi_py_libs(return_stub_dir('dccsi_stub'))))

    # custom prompt
    sys.ps1 = "[azpy]>>"