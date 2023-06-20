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
"""! A module to discover installed versions of Blender for selection

:file: < DCCsi >/Tools/DCC/Blender/discovery.py
:Status: Prototype, very bare bones
:Version: 0.0.1
Future: provide a variety of ways to discover Blender, i.e. use winreg
"""
# -------------------------------------------------------------------------
# standard imports
from pathlib import Path
from typing import Union
import logging as _logging
# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.Tools.DCC.Blender import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.discovery'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------
# dccsi imports
# the default and currently only supported discovery path
from DccScriptingInterface.Tools.DCC.Blender.constants import PATH_DCCSI_BLENDER_LOCATION
# -------------------------------------------------------------------------
# currently we only support the default win install path for
# for Blender 3.1, you can modify constants.py or you can
# override the envar/path in
def get_default_install(default_app_home: Union[str, Path] = PATH_DCCSI_BLENDER_LOCATION) -> Path:
    """! validates then returns the default dccsi winghome
    :return: Path(default_app_home), or None"""
    default_app_home = Path(default_app_home).resolve()
    if default_app_home.exists():
        return default_app_home
    else:
        _LOGGER.error(f'default_app_home not valid: {default_app_home.as_posix()}')
        return None

# in the future, we might want to help the user find all installs
# and select the install they would like to utilize. And this should
# provide robust per-platform and os-style registry support.
def find_all_installs() -> list:
    """! finds all platform installations of wing
    :return: list of wing install locations"""
    # ! this method is not fully implemented, future work.

    all_install_paths = list()
    all_install_paths.append(get_default_install())

    _LOGGER.info('Wing Installs found:')
    for index, install in enumerate(all_install_paths):
        _LOGGER.info(f'Install:{index}:: {install}')

    return all_install_paths

# then during configuration (from cli or otherwise), present the installs
# to the user and allow them to select which install to use
# this would potentially end up in a couple of places:
# <dccsi>\Tools\DCC\Blender\settings.local.json for a developer
# <project>\registry\dccsi_config_wing.json for a project configuration
# --- END -----------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """! Run this file as main, local testing"""

    blender_home = get_default_install(default_app_home=PATH_DCCSI_BLENDER_LOCATION)

    find_all_installs()

    if blender_home.exists():
        _LOGGER.info(f'blender_home: {blender_home}')
    else:
        _LOGGER.error(f'blender_home not valid: {blender_home.as_posix()}')
