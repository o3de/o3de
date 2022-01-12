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

"""<DCCsi>/Tools/DCC/Blender/setup.py
Running this module installs the DCCsi python requirements.txt for Blender.

It installs based on the python version into a location here:
<o3de>/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/3rdParty/Python/Lib/3.x
This is to ensure that we are not modifying the users Blender install directly.

For this script to function on windows you will need Administrator privledges.
Open Blender by Right-clicking on the executable in the start menu and select "Run as Administrator"
^ You only have to start with Admin rights if you are running setup.py or otherwise updating packages

1. windows use: <o3de path to>/DccScriptingInterface/Tools/DCC/Blender/setup.bat
2. Or run the scripts cli from command terminal
3. Or setup directly within Blender:
    Open up the System Console (under the Window menu)
	Select the 'scripting workspace' tab
	Do one of the following:
	    Use the 'open' button and open this script (the one you are reading):
			<o3de path to>/DccScriptingInterface/Tools/DCC/Blender/setup.py
	    Or copy/paste the contents of this script into the 'scripting workspace'
	Then run the code



Be sure to restart Blender! (you don't need Admin rights when not updating)
"""
# -------------------------------------------------------------------------
# standard imports
import subprocess
import sys
import os
from pathlib import Path
import logging as _logging
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
# global scope
_MODULENAME = 'O3DE.DCCsi.Tools.DCC.Blender.setup'

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