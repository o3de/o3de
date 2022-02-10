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
    < DCCsi >:: Tools//DCC//Blender//constants.py

This module contains default values for commony used constants & strings.
We can make an update here easily that is propogated elsewhere.
"""
# -------------------------------------------------------------------------
# built-ins
from os.path import expanduser
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'O3DE.DCCsi.Tools.DCC.Blender.constants'

# to do: maybe just set up azpy access and pull common consts from there
# if we need to pull in more then definitely
FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
FRMT_LOG_SHRT = "[%(asctime)s][%(name)s][%(levelname)s] >> %(message)s"

# configure basic logger
# note: not using a common logger to reduce cyclical imports
_logging.basicConfig(level=_DCCSI_LOGLEVEL,
                    format=FRMT_LOG_LONG,
                    datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Blender consts
USER_HOME = Path.home()

# Note: we've developed and tested with Blender 3.0 (experimental)
# change at your own risk, we are just future proofing.
DCCSI_BLENDER_VERSION = 3.0
DCCSI_BLENDER_PATH =      f"C:/Program Files/Blender Foundation/Blender {DCCSI_BLENDER_VERSION}"
DCCSI_BLENDER_EXE =       f"{DCCSI_BLENDER_PATH}/blender.exe"
DCCSI_BLENDER_LAUNCHER =  f"{DCCSI_BLENDER_PATH}/blender-launcher.exe"

DCCSI_BLENDER_PYTHON =    f"{DCCSI_BLENDER_PATH}/{DCCSI_BLENDER_VERSION}/python"
DCCSI_BLENDER_PY_EXE =    f"{DCCSI_BLENDER_PYTHON}/bin/python.exe"

DCCSI_BLENDER_WIKI_URL =  'https://github.com/o3de/o3de/wiki/O3DE-DCCsi-Blender'
# -------------------------------------------------------------------------