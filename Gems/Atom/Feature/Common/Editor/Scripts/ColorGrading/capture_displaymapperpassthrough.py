# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""Frame capture of the Displaymapper Passthrough (outputs .dds image)"""
# ------------------------------------------------------------------------
import logging as _logging
from env_bool import env_bool

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.capture_displaymapperpassthrough'

import ColorGrading.initialize
ColorGrading.initialize.start()

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# ------------------------------------------------------------------------


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
# ------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main
# -------------------------------------------------------------------------
if __name__ == '__main__':
    capture()
