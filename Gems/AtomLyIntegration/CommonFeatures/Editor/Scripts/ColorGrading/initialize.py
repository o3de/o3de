# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# standard imports
import sys
import os
import pathlib
import site
from pathlib import Path
import logging as _logging

import azlmbr

__all__ = ['start']

_MODULENAME = 'ColorGrading.initialize'

FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
_logging.basicConfig(level=_logging.DEBUG,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

def start():
    # this one is less optional and if not set bootstrapping azpy will fail
    _O3DE_DEV = Path(os.getenv('O3DE_DEV', Path(azlmbr.paths.engroot)))  # assume usually not set
    os.environ['O3DE_DEV'] = pathlib.PureWindowsPath(_O3DE_DEV).as_posix()
    _LOGGER.debug(_O3DE_DEV)

    _O3DE_BIN_PATH = Path(str(_O3DE_DEV),Path(azlmbr.paths.executableFolder))

    # this one is less optional and if not set bootstrapping azpy will fail
    _O3DE_BIN = Path(os.getenv('O3DE_BIN', _O3DE_BIN_PATH.resolve()))  # assume usually not set
    os.environ['O3DE_BIN'] = pathlib.PureWindowsPath(_O3DE_BIN).as_posix()

    _LOGGER.debug(_O3DE_BIN)

    site.addsitedir(_O3DE_BIN)

    import OpenImageIO as oiio

###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""

    start()