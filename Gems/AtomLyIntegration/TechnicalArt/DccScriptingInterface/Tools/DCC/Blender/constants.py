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
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------

# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'Tools.DCC.Blender.constants'

# Blender consts
USER_HOME = Path.home()

# Note: we've developed and tested with Blender 3.0 (experimental)
# change at your own risk, we are just future proofing.
DCCSI_BLENDER_VERSION =   3.0
DCCSI_BLENDER_LOCATION =  f"C:/Program Files/Blender Foundation/Blender {DCCSI_BLENDER_VERSION}"

# I think this one will launch with a console
DCCSI_BLENDER_EXE =       f"{DCCSI_BLENDER_LOCATION}/blender.exe"

# this is the standard launcher that doesn't have
DCCSI_BLENDER_LAUNCHER =  f"{DCCSI_BLENDER_LOCATION}/blender-launcher.exe"

DCCSI_BLENDER_PYTHON =    f"{DCCSI_BLENDER_LOCATION}/{DCCSI_BLENDER_VERSION}/python"
DCCSI_BLENDER_PY_EXE =    f"{DCCSI_BLENDER_PYTHON}/bin/python.exe"

DCCSI_BLENDER_WIKI_URL =  'https://github.com/o3de/o3de/wiki/O3DE-DCCsi-Tools-DCC-Blender'

DCCSI_TOOLS_BLENDER_PATH = 'foo'

# -------------------------------------------------------------------------