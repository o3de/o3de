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
_MODULENAME = 'foundation'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Local access
_MODULE_PATH = Path(__file__)            # this script
_PATH_DCCSIG = Path(_MODULE_PATH.parent) # dcsi/tools/dcc/blender
os.environ['PATH_DCCSIG'] = _PATH_DCCSIG.as_posix()
site.addsitedir(_PATH_DCCSIG.as_posix()) # python path

# the path we want to install packages into
STR_PATH_DCCSI_PYTHON_LIB = str('{0}\\3rdParty\\Python\\Lib\\{1}.x\\{1}.{2}.x\\site-packages')

# the default will be based on the python executable running this module    
# this value should be replaced with the sys,version of the target python
# for example mayapy, or blenders python, etc.
_PATH_DCCSI_PYTHON_LIB = Path(STR_PATH_DCCSI_PYTHON_LIB.format(_PATH_DCCSIG,
                                                               sys.version_info.major,
                                                               sys.version_info.minor))

# this is the shared default requirements.txt file to install for python 3.6.x+
_DCCSI_PYTHON_REQUIREMENTS = Path(_PATH_DCCSIG, 'requirements.txt')

# this will default to the python interpretter running this script (probably o3de)
# this should be relaced by the target interpretter python exe, like mayapy.exe
_PYTHON_EXE = Path(sys.executable)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def check_pip(python_exe=_PYTHON_EXE):
    """Check if pip is installed and log what version"""
    
    python_exe = Path(python_exe)
    
    if python_exe.exists():
        result = subprocess.call( [python_exe.as_posix(), "-m", "pip", "--version"] )
        _LOGGER.info(f'foundation.check_pip(), result: {result}')
        return result
    else:
        _LOGGER.error(f'python_exe does not exist: {python_exe}')
        return 0
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def ensurepip(python_exe=_PYTHON_EXE, upgrade=False):
    """Will use ensurepip method to ensure pip is installed"""
    
    #note: this doesn't work with python 3.7 which is the version o3de is on
    #luckily o3de comes with working pip
    #if this errors out with an exception and "ValueError: bad marshal data (unknown type code)"
    #you should try to install pip using dfoundation.install_pip() method
    
    result = 0
    
    python_exe = Path(python_exe)
    
    if python_exe.exists():
        
        if upgrade:
            result = subprocess.call( [python_exe.as_posix(), "-m", "ensurepip", "--upgrade"] )
            _LOGGER.info(f'foundation.ensurepip(python_exe, upgrade=True), result: {result}')    
        else:
            result = subprocess.call( [python_exe.as_posix(), "-m", "ensurepip"] )
            _LOGGER.info(f'foundation.ensurepip(python_exe), result: {result}')
    else:
        _LOGGER.error(f'python_exe does not exist: {python_exe}')
        return 0
    
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
    
    # ensure what is passed in is a Path object
    file_store = Path(file_store)
    
    try:
        file_store.exists()
    except FileExistsError as e:
        try:
            file_store.touch()
        except FileExistsError as e:
            _LOGGER.error(f'Could not make file: {file_store}')
    
    try:
        _get_pip = requests.get(url)
    except Exception as e:
        _LOGGER.error(f'could not request: {url}')
    
    try:
        file = open(file_store.as_posix(), 'wb').write(_get_pip.content)
        return file
    except IOError as e:
        _LOGGER.error(f'could not write: {file_store.as_posix()}')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def install_pip(python_exe=_PYTHON_EXE, download=True, upgrade=True, getpip=PIP_DL_LOC):
    """Installs pip via get-pip.py"""
    result = 0
    
    if download:
        getpip = download_getpip()
    
    python_exe = Path(python_exe)
    
    if python_exe.exists():
        python_exe = python_exe.as_posix()
        result = subprocess.call( [python_exe, "-m", getpip] )
        _LOGGER.info(f'result: {result}')
    else:
        _LOGGER.error(f'python_exe does not exist: {python_exe}')
        return 0
    
    if upgrade:
        python_exe = python_exe.as_posix()
        result = subprocess.call( [python_exe, "-m", "pip", "install", "--upgrade", "pip"] )
        _LOGGER.info(f'result: {result}')
        return result
    return result
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# version of requirements.txt to installa
if sys.version_info.major >= 3 and sys.version_info.minor >= 7:
    _REQUIREMENTS = _DCCSI_PYTHON_REQUIREMENTS
elif sys.version_info.major == 2 and sys.version_info.minor >= 7:
    _LOGGER.warning('Python 2.7 is end of life, we recommend using tools that operate py3.7 or higher')
    _REQUIREMENTS = Path(_PATH_DCCSIG,
                         'Tools',
                         'Resources',
                         'py27',
                         'requirements.txt').as_posix()
else:
    _REQUIREMENTS = None
    _LOGGER.error(f'Unsupported version: {sys.version_info}')

def install_requirements(python_exe=_PYTHON_EXE,
                         requirements=_REQUIREMENTS,
                         target_loc=_PATH_DCCSI_PYTHON_LIB.as_posix()):
    """Installs the DCCsi requirments.txt"""
    
    python_exe = Path(python_exe)
    requirements = Path(requirements)
    target_loc = Path(target_loc)
    
    if python_exe.exists():
        ## install required packages
        inst_cmd = [python_exe.as_posix(), "-m", "pip", "install", "-r", requirements.as_posix(), "-t", target_loc.as_posix()]
        result = subprocess.call( inst_cmd )
        return result
    else:
        _LOGGER.error(f'python_exe does not exist: {python_exe}')
        return 0
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def install_pkg(python_exe=_PYTHON_EXE,
                pkg_name='pathlib',
                target_loc=_PATH_DCCSI_PYTHON_LIB.as_posix()):
    """Installs a pkg for DCCsi"""
    
    python_exe = Path(python_exe)
    pkg_name = Path(pkg_name)
    target_loc = Path(target_loc)
    
    if python_exe.exists():
        python_exe = python_exe.as_posix()
        inst_cmd = [python_exe, "-m", "pip", "install", pkg_name.as_posix(), "-t", target_loc.as_posix()]
        result = subprocess.call( inst_cmd )
        return result
    else:
        _LOGGER.error(f'python_exe does not exist: {python_exe}')
        return 0
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def run_command() -> 'subprocess.CompletedProcess[str]':
    """Run some subprocess that captures output as ``str``"""
    return subprocess.CompletedProcess(args=[], returncode=0, stdout='')
# -------------------------------------------------------------------------



###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main (external commandline for testing)"""
    
    #os.environ['PYTHONINSPECT'] = 'True'
    
    STR_CROSSBAR = f"{'-' * 74}"
    _LOGGER.info(STR_CROSSBAR)
    
    _DCCSI_GDEBUG = False
    _DCCSI_DEV_MODE = False
    
    # default loglevel to info unless set
    _DCCSI_LOGLEVEL = _logging.INFO
    
    if _DCCSI_GDEBUG:
        # override loglevel if runnign debug
        _DCCSI_LOGLEVEL = _logging.DEBUG
        
    FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
    
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
        description='O3DE DCCsi Setup (aka Foundation). Will install DCCsi python package dependancies, for various DCC tools.',
            epilog="It is suggested to use '-py' or '--python_exe' to pass in the python exe for the target dcc tool.")

    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        help='Enables global debug flag.')
    
    parser.add_argument('-py', '--python_exe',
                        type=str,
                        required=False,
                        help='The python interpretter you want to run in the subprocess')
    
    parser.add_argument('-cp', '--check_pip',
                        type=bool,
                        required=False,
                        default=True,
                        help='Checks for pip')
    
    parser.add_argument('-ep', '--ensurepip',
                        type=bool,
                        required=False,
                        default=False,
                        help='Uses ensurepip, to make sure pip is installed')
    
    parser.add_argument('-ip', '--install_pip',
                        type=bool,
                        required=False,
                        default=False,
                        help='Attempts install pip via download of get-pip.py')
    
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
        
    if not args.python_exe:
        _LOGGER.info("It is suggested to use '-py' or '--python_exe' to pass in the python exe for the target dcc tool.")
        
    if args.python_exe:
        _PYTHON_EXE = Path(args.python_exe)
        
        # script to run
        _SYS_VER_SCRIPT = Path(_PATH_DCCSIG, 'return_sys_version.py')
        
        _SYS_VER = subprocess.CompletedProcess(args=[_PYTHON_EXE.as_posix(), "-m", _SYS_VER_SCRIPT.as_posix()], returncode=0, stdout='')
        
        _SYS_VER = _SYS_VER.split('.')
        
        STR_PATH_DCCSI_PYTHON_LIB = str('{0}\\3rdParty\\Python\\Lib\\{1}.x\\{1}.{2}.x\\site-packages')

        # need to update the package install location target based on python version
        _PATH_DCCSI_PYTHON_LIB = Path(STR_PATH_DCCSI_PYTHON_LIB.format(_PATH_DCCSIG, _SYS_VER[0], _SYS_VER[1]))        
    
    # this will verify pip is installed for the target python interpretter/env
    if args.check_pip:    
        _LOGGER.info(f'calling foundation.check_pip()')
        check_pip(_PYTHON_EXE)
    
    if args.ensurepip:    
        _LOGGER.info(f'calling foundation.ensurepip()')
        ensurepip(_PYTHON_EXE)
    
    if args.install_pip:    
        _LOGGER.info(f'calling foundation.install_pip()')
        install_pip(_PYTHON_EXE)
    
    if args.install_requirements:    
        _LOGGER.info(f'calling foundation.install_requirements()')
        install_requirements(_PYTHON_EXE, target_loc = _PATH_DCCSI_PYTHON_LIB)
        
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