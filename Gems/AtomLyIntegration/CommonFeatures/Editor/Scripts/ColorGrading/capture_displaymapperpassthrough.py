# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import logging as _logging
from env_bool import env_bool

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.capture_displaymapperpassthrough'

# set these true if you want them set globally for debugging
_DCCSI_GDEBUG = env_bool('DCCSI_GDEBUG', False)
_DCCSI_DEV_MODE = env_bool('DCCSI_DEV_MODE', False)
_DCCSI_GDEBUGGER = env_bool('DCCSI_GDEBUGGER', False)
_DCCSI_LOGLEVEL = env_bool('DCCSI_LOGLEVEL', int(20))

if _DCCSI_GDEBUG:
    _DCCSI_LOGLEVEL = int(10)

FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
_logging.basicConfig(level=_DCCSI_LOGLEVEL,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# ------------------------------------------------------------------------


import azlmbr.bus
import azlmbr.atom

default_passtree = ["Root",
                    "MainPipeline_0",
                    "MainPipeline",
                    "PostProcessPass",
                    "LightAdaptation",
                    "DisplayMapperPass",
                    "DisplayMapperPassthrough"]

default_path = "FrameCapture\DisplayMapperPassthrough.dds"

# To Do: we should try to set display mapper to passthrough,
# then back after capture?

# To Do: we can wrap this, to call from a PySide2 GUI

def capture(command="CapturePassAttachment",
            passtree=default_passtree,
            pass_type="Output",
            output_path=default_path):
    azlmbr.atom.FrameCaptureRequestBus(azlmbr.bus.Broadcast,
                                       command,
                                       passtree,
                                       pass_type,
                                       output_path, 1)

###########################################################################
# Main Code Block, runs this script as main
# -------------------------------------------------------------------------
if __name__ == '__main__':
    capture()
    