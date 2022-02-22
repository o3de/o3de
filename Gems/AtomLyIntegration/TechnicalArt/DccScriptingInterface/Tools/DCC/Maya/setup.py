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
import inspect
import traceback
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
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
_LOGGER.debug('_MODULE_PATH: {}'.format(_MODULE_PATH))

# we need to set up basic access to the DCCsi
_DCCSI_TOOLS_MAYA_PATH = Path(_MODULE_PATH.parent)
_DCCSI_TOOLS_MAYA_PATH = Path(os.getenv('DCCSI_TOOLS_MAYA_PATH', _DCCSI_TOOLS_MAYA_PATH.as_posix()))
site.addsitedir(_DCCSI_TOOLS_MAYA_PATH.as_posix())

# we need to set up basic access to the DCCsi
_PATH_DCCSI_TOOLS_DCC = Path(_DCCSI_TOOLS_MAYA_PATH.parent)
_PATH_DCCSI_TOOLS_DCC = Path(os.getenv('PATH_DCCSI_TOOLS_DCC', _PATH_DCCSI_TOOLS_DCC.as_posix()))
site.addsitedir(_PATH_DCCSI_TOOLS_DCC.as_posix())

# we need to set up basic access to the DCCsi
_PATH_DCCSI_TOOLS = Path(_PATH_DCCSI_TOOLS_DCC.parent)
_PATH_DCCSI_TOOLS = Path(os.getenv('PATH_DCCSI_TOOLS', _PATH_DCCSI_TOOLS.as_posix()))

# we need to set up basic access to the DCCsi
_PATH_DCCSIG = Path(_PATH_DCCSI_TOOLS.parent)
_PATH_DCCSIG = Path(os.getenv('PATH_DCCSIG', _PATH_DCCSIG.as_posix()))
site.addsitedir(_PATH_DCCSIG.as_posix())

# now we have azpy api access
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG

# bootstraps the DCCsi
import Tools.DCC.Maya.config as maya_config
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
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

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {}.'.format({_MODULENAME}))
_LOGGER.debug('_DCCSI_GDEBUG: {}'.format(_DCCSI_GDEBUG))
_LOGGER.debug('_DCCSI_DEV_MODE: {}'.format(_DCCSI_DEV_MODE))
_LOGGER.debug('_DCCSI_LOGLEVEL: {}'.format(_DCCSI_LOGLEVEL))

#os.environ['PYTHONINSPECT'] = 'True'
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def install_pip(_SETTINGS):
    # path to python.exe
    python_exe = Path(sys.prefix, 'bin', 'python.exe').resolve()

    # upgrade pip
    subprocess.call([python_exe, "-m", "ensurepip"])
    subprocess.call([python_exe, "-m", "pip", "install", "--upgrade", "pip"])
    
    python_exe
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def install_requirements():

    ## install required packages
    #subprocess.call([python_exe, "-m", "pip", "install", "PACKAGE_TO_INSTALL"])

    print("DONE")
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def install_pkg(pkg_name='pathlib'):
    # path to python.exe
    python_exe = Path(sys.prefix, 'bin', 'python.exe').resolve()

    # upgrade pip
    subprocess.call([python_exe, "-m", "ensurepip"])
    subprocess.call([python_exe, "-m", "pip", "install", "--upgrade", "pip"])

    # install required packages
    subprocess.call([python_exe, "-m", "pip", "install", pkg_name])

    print("DONE")
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

    args = parser.parse_args()

    # easy overrides
    if args.global_debug:
        _DCCSI_GDEBUG = True
        os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)

    _SETTINGS = config.get_dccsi_maya_settings()    

    install(_SETTINGS)
# --- END -----------------------------------------------------------------