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
import inspect
import pathlib
import site
from pathlib import Path
import logging as _logging

_PACKAGENAME = 'Gems.AtomLyIntegration.CommonFeatures.Editor.Scripts.ColorGrading.bootstrap'

FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
_logging.basicConfig(level=_logging.DEBUG,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

# print (inspect.getfile(inspect.currentframe()) # script filename (usually with path)
# script directory
_MODULE_PATH = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
_MODULE_PATH = Path(_MODULE_PATH)
site.addsitedir(_MODULE_PATH.resolve())

from ColorGrading.initialize import start
start()