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
    < DCCsi >:: foundation.py

Running this module installs the DCCsi python requirements.txt for other python
interpreters (like Maya)

It installs based on the python version into a location (such as):
    '<o3de>/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/3rdParty/Python/Lib/3.x'

This is to ensure that we are not modifying the users DCC tools install directly.

For this script to function on windows you may need Administrator privileges.
^ You only have to start with Admin rights if you are running foundation.py
to install/update packages and other functions that write to disk.

Suggestion: we could move the package location and be more cross-platform:
from: '<o3de>/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/3rdParty/Python/Lib/3.x'
to (windows): '/Users/<Username>/AppData/Local/Programs/'

Open an admin elevated cmd prompt here:
    '<o3de>/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface'

The following would execute this script, the default behavior is to check
the o3de python and install the requirements.txt for that python version,

  >python.cmd foundation.py

Suggestion: document additional usage (how to install for Maya 2022 py3.7, etc.)
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
# os.environ['PYTHONINSPECT'] = 'True'
_START = timeit.default_timer()  # start tracking

# global scope
_MODULENAME = 'foundation'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Local access
_MODULE_PATH = Path(__file__)            # this script
_PATH_DCCSIG = Path(_MODULE_PATH.parent)  # dccsi
os.environ['PATH_DCCSIG'] = _PATH_DCCSIG.as_posix()
site.addsitedir(_PATH_DCCSIG.as_posix())  # python path
os.chdir(_PATH_DCCSIG.as_posix())        # cwd

# local imports from dccsi
import azpy.config_utils

# these are just defaults and are meant to be replaced by info for the target python.exe
_SYS_VER_MAJOR = sys.version_info.major
_SYS_VER_MINOR = sys.version_info.minor
# the default will be based on the python executable running this module
# this value should be replaced with the sys,version of the target python
# for example mayapy, or blenders python, etc.

_PATH_DCCSI_PYTHON_LIB = Path(f'{_PATH_DCCSIG}\\3rdParty\\Python\\Lib\\'
                              f'{_SYS_VER_MAJOR}.x\\'
                              f'{_SYS_VER_MAJOR}.{_SYS_VER_MINOR}.x\\site-packages').resolve()

_PATH_DCCSI_PYTHON_LIB.touch(exist_ok=True) # make sure it's there

# this is the shared default requirements.txt file to install for python 3.6.x+
_DCCSI_PYTHON_REQUIREMENTS = Path(_PATH_DCCSIG, 'requirements.txt')

# this will default to the python interpreter running this script (probably o3de)
# this should be replaced by the target interpreter python exe, like mayapy.exe
_PYTHON_EXE = Path(sys.executable)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def check_pip(python_exe=_PYTHON_EXE):
    """Check if pip is installed and log what version"""

    python_exe = Path(python_exe)

    if python_exe.exists():
        result = subprocess.call([python_exe.as_posix(), "-m", "pip", "--version"])
        _LOGGER.info(f'foundation.check_pip(), result: {result}')
        return result
    else:
        _LOGGER.error(f'python_exe does not exist: {python_exe}')
        return 1
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def ensurepip(python_exe=_PYTHON_EXE, upgrade=False):
    """Will use ensurepip method to ensure pip is installed"""

    # note: this doesn't work with python 3.10 which is the version o3de is on
    # luckily o3de comes with working pip
    # if this errors out with an exception and "ValueError: bad marshal data (unknown type code)"
    # you should try to install pip using foundation.install_pip() method

    result = 0

    python_exe = Path(python_exe)

    if python_exe.exists():

        if upgrade:
            result = subprocess.call([python_exe.as_posix(), "-m", "ensurepip", "--upgrade"])
            _LOGGER.info(f'foundation.ensurepip(python_exe, upgrade=True), result: {result}')
        else:
            result = subprocess.call([python_exe.as_posix(), "-m", "ensurepip"])
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

# temp dir to store in:
_PIP_DL_LOC = Path(_PATH_DCCSIG) / '__tmp__'
if not _PIP_DL_LOC.exists():
    try:
        _PIP_DL_LOC.mkdir(parents=True)
    except Exception as e:
        _LOGGER.error(f'error: {e}, could not .mkdir(): {PIP_DL_LOC.as_posix()}')

# default file location to store it:
_PIP_DL_LOC = _PIP_DL_LOC / 'get-pip.py'
try:
    _PIP_DL_LOC.touch(mode=0o666, exist_ok=True)
except Exception as e:
    _LOGGER.error(f'error: {e}, could not .touch(): {PIP_DL_LOC.as_posix()}')

def download_getpip(url=DL_URL, file_store=_PIP_DL_LOC):
    """Attempts to download the get - pip.py script"""
    import requests

    # ensure what is passed in is a Path object
    file_store = Path(file_store)
    file_store = Path.joinpath(file_store)

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
        return None
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def install_pip(python_exe=_PYTHON_EXE, download=True, upgrade=True, getpip=_PIP_DL_LOC):
    """Installs pip via get - pip.py"""
    result = 0

    if download:
        getpip = download_getpip()
        if not getpip:
            return result

    python_exe = Path(python_exe)

    if python_exe.exists():
        python_exe = python_exe.as_posix()
        result = subprocess.call([python_exe, "-m", getpip])
        _LOGGER.info(f'result: {result}')
    else:
        _LOGGER.error(f'python_exe does not exist: {python_exe}')
        return 0

    if upgrade:
        python_exe = python_exe.as_posix()
        result = subprocess.call([python_exe, "-m", "pip", "install", "--upgrade", "pip"])
        _LOGGER.info(f'result: {result}')
        return result

    return result
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# version of requirements.txt to install
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
    """Installs the DCCsi requirements.txt"""

    python_exe = Path(python_exe)
    requirements = Path(requirements)
    target_loc = Path(target_loc)

    if python_exe.exists():
        # install required packages
        inst_cmd = [python_exe.as_posix(), "-m", "pip", "install",
                    "-r", requirements.as_posix(), "-t", target_loc.as_posix()]
        result = subprocess.call(inst_cmd)
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
        inst_cmd = [python_exe.as_posix(), "-m", "pip", "install", pkg_name.as_posix(),
                    "-t", target_loc.as_posix()]
        result = subprocess.call(inst_cmd)
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


# -------------------------------------------------------------------------
def set_version(ver_major=sys.version_info.major,
                ver_minor=sys.version_info.minor):
    global _SYS_VER_MAJOR
    global _SYS_VER_MINOR
    global _PATH_DCCSI_PYTHON_LIB

    _SYS_VER_MAJOR = ver_major
    _SYS_VER_MINOR = ver_minor
    _PATH_DCCSI_PYTHON_LIB = Path(STR_PATH_DCCSI_PYTHON_LIB.format(_PATH_DCCSIG,
                                                                   _SYS_VER_MAJOR,
                                                                   _SYS_VER_MINOR))
    return _PATH_DCCSI_PYTHON_LIB
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def get_version(_PYTHON_EXE):
    _PYTHON_EXE = Path(_PYTHON_EXE)
    if _PYTHON_EXE.exists():
        # this will switch to run the specified dcc tools python exe and determine version
        _COMMAND = [_PYTHON_EXE.as_posix(), "--version"]
        _process = subprocess.Popen(_COMMAND, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        _out, _err = _process.communicate()
        _out = _out.decode("utf-8")  # decodes byte string to string
        _out = _out.replace("\r\n", "")  # clean
        _LOGGER.info(f'Python Version is: {_out}')
        _ver = _out.split(" ")[-1]  # split by space, take version
        _ver = _ver.split('.')  # split by . to list
        return _ver
    else:
        _LOGGER.error(f'Python exe does not exist: {_PYTHON_EXE.as_posix()}')
        return None
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main(external command line)"""

    STR_CROSSBAR = f"{'-' * 74}"

    _DCCSI_GDEBUG = False
    _DCCSI_DEV_MODE = False

    # default loglevel to info unless set
    _DCCSI_LOGLEVEL = _logging.INFO

    if _DCCSI_GDEBUG:
        # override loglevel if running debug
        _DCCSI_LOGLEVEL = _logging.DEBUG

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

    import argparse

    parser = argparse.ArgumentParser(
        description='O3DE DCCsi Setup (aka Foundation). Will install DCCsi python package dependencies, for various DCC tools.',
        epilog="It is suggested to use '-py' or '--python_exe' to pass in the python exe for the target dcc tool.")

    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        help='Enables global debug flag.')

    parser.add_argument('-dm', '--developer-mode',
                        type=bool,
                        required=False,
                        help='Enables dev mode for early auto attaching debugger.')

    parser.add_argument('-sd', '--set-debugger',
                        type=str,
                        required=False,
                        default='WING',
                        help='(NOT IMPLEMENTED) Default debugger: WING, others: PYCHARM and VSCODE.')

    parser.add_argument('-py', '--python_exe',
                        type=str,
                        required=False,
                        help='The python interpreter you want to run in the subprocess')

    parser.add_argument('-cp', '--check_pip',
                        required=False,
                        default=True,
                        help='Checks for pip')

    parser.add_argument('-ep', '--ensurepip',
                        required=False,
                        help='Uses ensurepip, to make sure pip is installed')

    parser.add_argument('-ip', '--install_pip',
                        required=False,
                        help='Attempts install pip via download of get-pip.py')

    parser.add_argument('-ir', '--install_requirements',
                        required=False,
                        default=True,
                        help='Exits python')

    parser.add_argument('-ex', '--exit',
                        type=bool,
                        required=False,
                        help='Exits python. Do not exit if you want to be in interactive interpreter after config')

    args = parser.parse_args()

    from azpy.shared.utils.arg_bool import arg_bool

    # easy overrides
    if arg_bool(args.global_debug, desc="args.global_debug"):
        from azpy.constants import ENVAR_DCCSI_GDEBUG
        _DCCSI_GDEBUG = True
        _LOGGER.setLevel(_logging.DEBUG)
        _LOGGER.info(f'Global debug is set, {ENVAR_DCCSI_GDEBUG}={_DCCSI_GDEBUG}')

    if arg_bool(args.developer_mode, desc="args.developer_mode"):
        _DCCSI_DEV_MODE = True
        azpy.config_utils.attach_debugger()  # attempts to start debugger

    if not args.python_exe:
        _LOGGER.warning("It is suggested to use arg '-py' or '--python_exe' to pass in the python exe for the target dcc tool.")

    if args.python_exe:
        _PYTHON_EXE = Path(args.python_exe)
        _LOGGER.info(f'Target py exe is: {_PYTHON_EXE}')

        if _PYTHON_EXE.exists():
            _py_version = get_version(_PYTHON_EXE)

            # then we can change the version dependant target folder for pkg install
            _PATH_DCCSI_PYTHON_LIB = set_version(_py_version[0], _py_version[1])

            if _PATH_DCCSI_PYTHON_LIB.exists():
                _LOGGER.info(f'Requirements, install target: {_PATH_DCCSI_PYTHON_LIB}')
            else:
                _PATH_DCCSI_PYTHON_LIB.touch()
                _LOGGER.info(f'.touch(): {_PATH_DCCSI_PYTHON_LIB}')
        else:
            _LOGGER.error(f'This py exe does not exist:{_PYTHON_EXE}')
            _LOGGER.info(f'Make sure to wrap your path to the exe in quotes, like:')
            _LOGGER.info(f'.\python foundation.py -py="C:\Program Files\Autodesk\Maya2023\bin\mayapy.exe"')
            sys.exit()

    # this will verify pip is installed for the target python interpreter/env
    if arg_bool(args.check_pip, desc='args.check_pip'):
        _LOGGER.info(f'calling foundation.check_pip()')
        result = check_pip(_PYTHON_EXE)

        if result != 0:
            _LOGGER.warning(f'check_pip(), Invalid result: { result }')

    if arg_bool(args.ensurepip, desc='args.ensurepip'):
        _LOGGER.info(f'calling foundation.ensurepip()')
        ensurepip(_PYTHON_EXE)

    if arg_bool(args.install_pip, desc='args.install_pip'):
        _LOGGER.info(f'calling foundation.install_pip()')
        install_pip(_PYTHON_EXE)

    # installing the requirements.txt is enabled by default
    if arg_bool(args.install_requirements, desc='args.check_pip'):
        _LOGGER.info(f'calling foundation.install_requirements( {_PYTHON_EXE}, target_loc = {_PATH_DCCSI_PYTHON_LIB.as_posix()} )')
        install_requirements(_PYTHON_EXE, target_loc = _PATH_DCCSI_PYTHON_LIB.as_posix())

    # -- DONE ----
    _LOGGER.info(STR_CROSSBAR)

    _LOGGER.info('O3DE DCCsi {0}.py took: {1} sec'.format(_MODULENAME, timeit.default_timer() - _START))

    if args.exit:
        import sys
        # return
        sys.exit()
    else:
        # custom prompt
        sys.ps1 = "[{}]>>".format(_MODULENAME)
# --- END -----------------------------------------------------------------
