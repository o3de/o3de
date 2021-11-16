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

_MODULENAME = 'ColorGrading.capture_displaymapperpassthrough'

import ColorGrading.initialize
ColorGrading.initialize.start()

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# ------------------------------------------------------------------------


# ------------------------------------------------------------------------
import azlmbr.bus
import azlmbr.atom

# This requires the level to have the DisplayMapper component added
# and configured to 'Passthrough'
# but now we can capture the parent input 
# so this is here for reference for how it previously worked
passtree_displaymapperpassthrough = ["Root",
                                     "MainPipeline_0",
                                     "MainPipeline",
                                     "PostProcessPass",
                                     "LightAdaptation",
                                     "DisplayMapperPass",
                                     "DisplayMapperPassthrough"]

# we can grad the parent pass input to the displaymapper directly
passtree_default = ["Root",
                    "MainPipeline_0",
                    "MainPipeline",
                    "PostProcessPass",
                    "LightAdaptation",
                    "DisplayMapperPass"]

default_path = "FrameCapture\DisplayMappeInput.dds"

# To Do: we can wrap this, to call from a PySide2 GUI

def capture(command="CapturePassAttachment",
            passtree=passtree_default,
            pass_type="Input",
            output_path=default_path):
    """Writes frame capture into project cache"""
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
