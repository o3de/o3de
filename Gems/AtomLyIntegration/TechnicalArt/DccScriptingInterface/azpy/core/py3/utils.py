# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------
"""DCCsi.azpy.core.py3.utils
This module contain versions of utils speciufic to py3 snyntax"""
import sys
import pathlib
import logging as _logging

__all__ = ['get_datadir']

_MODULENAME = 'azpy.core.py3.utils'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

# -------------------------------------------------------------------------
def get_datadir() -> pathlib.Path:
    """
    persistent application data.
    # linux: ~/.local/share
    # macOS: ~/Library/Application Support
    # windows: C:/Users/<USER>/AppData/Roaming
    """

    home = pathlib.Path.home()

    if sys.platform.startswith('win'):
        return home / "AppData/Roaming"
    elif sys.platform == "linux":
        return home / ".local/share"
    elif sys.platform == "darwin":
        return home / "Library/Application Support"
    else: # unknown
        return None
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """module testing"""

    user_data_dir = get_datadir()
    _LOGGER.info(user_data_dir.resolve())
