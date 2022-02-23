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
    < DCCsi >:: Tools/DCC/Maya/setup.py
    
Running this module installs the DCCsi python requirements.txt for Maya.

It installs based on the python version into a location (such as):
<o3de>/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/3rdParty/Python/Lib/3.x

This is to ensure that we are not modifying the users Maya install directly.

For this script to function on windows you will need Administrator privledges.
^ You only have to start with Admin rights if you are running setup.py or otherwise updating packages

To Do: document usage (how to install for Maya 2020 py2, Maya 2022 py2.7, Maya 2022 py3.7)
"""
# -------------------------------------------------------------------------
# standard imports
import subprocess
import sys
import os
import site
import timeit
import inspect
import traceback
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
#os.environ['PYTHONINSPECT'] = 'True'
_START = timeit.default_timer() # start tracking

# global scope
_MODULENAME = 'Tools.DCC.Maya.setup'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Maya is frozen
#_MODULE_PATH = Path(__file__)
# https://tinyurl.com/y49t3zzn
# module path when frozen
_MODULE_PATH = Path(os.path.abspath(inspect.getfile(inspect.currentframe())))
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH.as_posix()))

_PATH_DCCSI_TOOLS_MAYA = Path(_MODULE_PATH.parent)
_PATH_DCCSI_TOOLS_MAYA = Path(os.getenv('PATH_DCCSI_TOOLS_MAYA',
                                        _PATH_DCCSI_TOOLS_MAYA.as_posix()))

# we need to set up basic access to the DCCsi
_PATH_DCCSIG = Path(_PATH_DCCSI_TOOLS_MAYA.parent.parent.parent)
_PATH_DCCSIG = Path(os.getenv('PATH_DCCSIG', _PATH_DCCSIG.as_posix()))
site.addsitedir(_PATH_DCCSIG.as_posix())

# now we have azpy api access
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG
from azpy.constants import STR_CROSSBAR

# bootstraps the DCCsi
import Tools.DCC.Maya.constants
from Tools.DCC.Maya.constants import DCCSI_PY_MAYA
from Tools.DCC.Maya.constants import DCCSI_PYTHON_REQUIREMENTS
from Tools.DCC.Maya.constants import MAYA_BIN_PATH
from Tools.DCC.Maya.constants import STR_PATH_DCCSI_PYTHON_LIB
    
# override based on current executable
PATH_DCCSI_PYTHON_LIB = STR_PATH_DCCSI_PYTHON_LIB.format(_PATH_DCCSIG,
                                                         sys.version_info.major,
                                                         sys.version_info.minor)
PATH_DCCSI_PYTHON_LIB = Path(PATH_DCCSI_PYTHON_LIB).as_posix()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def check_pip(python_exe=DCCSI_PY_MAYA):
    """Check if pip is installed and what version"""
    
    python_exe = Path(python_exe)
    
    if python_exe.exists():
        python_exe = python_exe.as_posix()
        result = subprocess.call( [python_exe, "-m", "pip", "--version"] )
        _LOGGER.debug('check_pip(), result: {}'.format(result))
        return result
    else:
        _LOGGER.error('python_exe does not exist: {}'.format(python_exe))
        return 0
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def ensurepip(python_exe=DCCSI_PY_MAYA, upgrade=True):
    """Will use ensurepip to ensure pip is installed"""
    
    result = 0
    
    python_exe = Path(python_exe)
    
    if python_exe.exists():
        python_exe = python_exe.as_posix()
        result = subprocess.call( [python_exe, "-m", "ensurepip"] )
        _LOGGER.debug('ensurepip(), result: {}'.format(result))
    else:
        _LOGGER.error('python_exe does not exist: {}'.format(python_exe))
        return 0
    
    if upgrade:
        python_exe = python_exe
        result = subprocess.call( [python_exe, "-m", "ensurepip", "--upgrade"] )
        _LOGGER.debug('ensurepip(upgrade=True), result: {}'.format(result))
        return result
    return result
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_GET_PIP_PY37_URL = "https://bootstrap.pypa.io/get-pip.py"
_GET_PIP_PY27_URL = "https://bootstrap.pypa.io/pip/2.7/get-pip.py"

# version to download (DL)
if sys.version_info.major >= 3 and sys.version_info.minor >= 7:
    DL_URL = _GET_PIP_PY37_URL
elif sys.version_info.major < 3:
    DL_URL = _GET_PIP_PY27_URL
    
# default location to store it:
PIP_DL_LOC = Path(_PATH_DCCSIG, '.tmp', 'get-pip.py').as_posix()

def download_getpip(url=DL_URL, file_store=PIP_DL_LOC):
    """Attempts to download the get-pip.py script"""
    import requests
    try:
        _get_pip = requests.get(url)
    except Exception as e:
        _LOGGER.error('could not request: {}'.format(url))
    
    try:
        file = open(file_store.as_posix(), 'wb').write(_get_pip.content)
        return file
    except IOError as e:
        _LOGGER.error('could not write: {}'.format(file_store.as_posix()))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def install_pip(python_exe=DCCSI_PY_MAYA, download=True, upgrade=True, getpip=PIP_DL_LOC):
    """Installs pip via get-pip.py"""
    result = 0
    
    if download:
        getpip = download_getpip()
    
    python_exe = Path(python_exe)
    
    if python_exe.exists():
        python_exe = python_exe.as_posix()
        result = subprocess.call( [python_exe, "-m", getpip] )
        _LOGGER.debug('result: {}'.format(result))
    else:
        _LOGGER.error('python_exe does not exist: {}'.format(python_exe))
        return 0
    
    if upgrade:
        python_exe = python_exe.as_posix()
        result = subprocess.call( [python_exe, "-m", "pip", "install", "--upgrade", "pip"] )
        _LOGGER.debug('result: {}'.format(result))
        return result
    return result
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# version of requirements.txt to install
if sys.version_info.major >= 3 and sys.version_info.minor >= 7:
    _REQUIREMENTS = DCCSI_PYTHON_REQUIREMENTS
elif sys.version_info.major < 3:
    _REQUIREMENTS = Path(_PATH_DCCSI_TOOLS_MAYA, 'requirements.txt').as_posix()

def install_requirements(python_exe=DCCSI_PY_MAYA, requirements=_REQUIREMENTS, target_loc=PATH_DCCSI_PYTHON_LIB):
    """Installs the DCCsi requirments.txt"""
    
    python_exe = Path(python_exe)
    
    if python_exe.exists():
        python_exe = python_exe.as_posix()
        ## install required packages
        inst_cmd = [python_exe, "-m", "pip", "install", "-r", requirements, "-t", target_loc]
        result = subprocess.call( inst_cmd )
        return result
    else:
        _LOGGER.error('python_exe does not exist: {}'.format(python_exe))
        return 0
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def install_pkg(python_exe=DCCSI_PY_MAYA, pkg_name='pathlib', target_loc=PATH_DCCSI_PYTHON_LIB):
    """Installs a pkg for DCCsi"""
    
    python_exe = Path(python_exe)
    
    if python_exe.exists():
        python_exe = python_exe.as_posix()
        inst_cmd = [python_exe, "-m", "pip", "install", pkg_name, "-t", target_loc]
        result = subprocess.call( inst_cmd )
        return result
    else:
        _LOGGER.error('python_exe does not exist: {}'.format(python_exe))
        return 0
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main (external commandline for testing)"""
    
    #os.environ['PYTHONINSPECT'] = 'True'
    
    _DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
    _DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
    _DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')
    
    # default loglevel to info unless set
    _DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))
    if _DCCSI_GDEBUG:
        # override loglevel if runnign debug
        _DCCSI_LOGLEVEL = _logging.DEBUG
    
    # configure basic logger
    # note: not using a common logger to reduce cyclical imports
    _logging.basicConfig(level=_DCCSI_LOGLEVEL,
                         format=FRMT_LOG_LONG,
                        datefmt='%m-%d %H:%M')
    
    _LOGGER = _logging.getLogger(_MODULENAME)
    _LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
    _LOGGER.debug('_DCCSI_GDEBUG: {}'.format(_DCCSI_GDEBUG))
    _LOGGER.debug('_DCCSI_DEV_MODE: {}'.format(_DCCSI_DEV_MODE))
    _LOGGER.debug('_DCCSI_LOGLEVEL: {}'.format(_DCCSI_LOGLEVEL))
    
    import argparse

    parser = argparse.ArgumentParser(
        description='O3DE DCCsi Blender Setup',
            epilog="Will install python package dependancies")

    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        help='Enables global debug flag.')
    
    parser.add_argument('-cp', '--check_pip',
                        type=bool,
                        required=False,
                        default=True,
                        help='Checks for pip')
    
    parser.add_argument('-ep', '--ensurepip',
                        type=bool,
                        required=False,
                        default=True,
                        help='Uses ensurepip, to make sure pip is installed')
    
    parser.add_argument('-ir', '--install_requirements',
                        type=bool,
                        required=False,
                        default=True,
                        help='Exits python')
    
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
    
    if args.check_pip:    
        check_pip()
    
    if args.ensurepip:    
        ensurepip()
    
    if args.install_requirements:    
        install_requirements()
        
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