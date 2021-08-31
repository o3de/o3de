# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
Boostraps O3DE editor access to the python scripts within Atom.Feature.Commmon
Example: color grading related scripts
"""
# ------------------------------------------------------------------------
# standard imports
import sys
import os
import inspect
import pathlib
import site
from pathlib import Path
import logging as _logging
# ------------------------------------------------------------------------
_MODULENAME = 'Gems.Atom.Feature.Common.bootstrap'

# print (inspect.getfile(inspect.currentframe()) # script filename (usually with path)
# script directory
_MODULE_PATH = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
_MODULE_PATH = Path(_MODULE_PATH)
site.addsitedir(_MODULE_PATH.resolve())

from ColorGrading import env_bool
from ColorGrading import initialize_logger
from ColorGrading import DCCSI_GDEBUG
from ColorGrading import DCCSI_DEV_MODE
from ColorGrading import DCCSI_LOGLEVEL

if DCCSI_GDEBUG:
    DCCSI_LOGLEVEL = int(10)

_LOGGER = initialize_logger(_MODULENAME, log_to_file=False, default_log_level=DCCSI_LOGLEVEL)
_LOGGER.info('Initializing: {0}.'.format({_MODULENAME}))
_LOGGER.info(f'site.addsitedir({_MODULE_PATH.resolve()})')

# early connect to the debugger
if DCCSI_DEV_MODE:
    APP_DATA_WING = Path('C:/Users/gallowj/AppData/Roaming/Wing Pro 7')
    APP_DATA_WING.resolve()
    site.addsitedir(pathlib.PureWindowsPath(APP_DATA_WING).as_posix())
    import wingdbstub as debugger
    try:
        debugger.Ensure()
        _LOGGER.info("Wing debugger attached")
    except Exception as e:
        _LOGGER.debug('Can not attach Wing debugger (running in IDE already?)')


from ColorGrading.initialize import start
start()
# ------------------------------------------------------------------------