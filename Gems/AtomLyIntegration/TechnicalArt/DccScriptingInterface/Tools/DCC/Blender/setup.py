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
    < DCCsi >:: Tools//DCC//Blender//setup.py
    
Running this module installs the DCCsi python requirements.txt for Blender.

It installs based on the python version into a location (such as):
<o3de>/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/3rdParty/Python/Lib/3.x

This is to ensure that we are not modifying the users Blender install directly.

For this script to function on windows you will need Administrator privledges.
^ You only have to start with Admin rights if you are running setup.py or otherwise updating packages

1. Setup directly within Blender:
    Launch Blender by Right-clicking on blender executable in the 'start menu' and select "Run as Administrator"
    Open up the System Console (under the Window menu)
	Select the 'scripting workspace' tab
	Do one of the following:
	    Use the 'open' button and open this script (the one you are reading):
			<o3de path to>/DccScriptingInterface/Tools/DCC/Blender/setup.py
	    Or copy/paste the contents of this script into the 'scripting workspace'
	Then run the code

Be sure to restart Blender! (you don't need Admin rights when not updating)

2. On windows (automated .bat file):
    Find the following file (it's right next to the script you are reading)
    <o3de path to>/DccScriptingInterface/Tools/DCC/Blender/setup.bat
    Right-click on the setup.bat and select "Run as Administrator"
    (under the hood this just runs setup.py cli with default args)

3. Or run the setup.py (this scripts) cli from a command terminal (with elevated Admin Privledges)
    To Do: document
"""
# -------------------------------------------------------------------------
# standard imports
import subprocess
import sys
import os
import site
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'Blender.setup'

# Local access
_MODULE_PATH = Path(__file__)                   # this script
_DCCSI_BLENDER_PATH = Path(_MODULE_PATH.parent) # dcsi/tools/dcc/blender
os.environ['PATH_DCCSI_BLENDER'] = _DCCSI_BLENDER_PATH.as_posix()
site.addsitedir(_DCCSI_BLENDER_PATH.as_posix())  # python path

import config # bootstraps the DCCsi
# now we have azpy api access
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG
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


    _SETTINGS = config.get_dccsi_blender_settings()    

    install(_SETTINGS)
# --- END -----------------------------------------------------------------