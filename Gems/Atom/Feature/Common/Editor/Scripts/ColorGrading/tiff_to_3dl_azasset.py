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
    input:  a shaped .tiff representing a LUT (for instance coming out of photoshop)
    output: a inverse shaped LUT as .tiff
            ^ as a .3DL (normalized lut file type)
            ^ as a .azasset (for o3de engine)
"""

import sys
import os
import argparse
import math
import site
import pathlib
from pathlib import Path
import logging as _logging
import numpy as np

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.tiff_to_3dl_azasset'

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

import ColorGrading.initialize
if ColorGrading.initialize.start():
    try:
        import OpenImageIO as oiio
        pass
    except ImportError as e:
        _LOGGER.error(f"invalid import: {e}")
        sys.exit(1)
# ------------------------------------------------------------------------


# ------------------------------------------------------------------------
from ColorGrading.from_3dl_to_azasset import write_azasset

from ColorGrading.azasset_converter_utils import generate_lut_values, write_3DL

###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""

    parser=argparse.ArgumentParser()
    parser.add_argument('--i', type=str, required=True, help='input file')
    parser.add_argument('--o', type=str, required=True, help='output file')
    args=parser.parse_args()

    # Read input image
    image_buffer=oiio.ImageBuf(args.i)
    image_spec=image_buffer.spec()
    
    #img = oiio.ImageInput.open(args.i)
    #_LOGGER.info(f"Resolution is, x: {img.spec().width} and y: {img.spec().height}")
    
    _LOGGER.info(f"Resolution is, x: {image_buffer.spec().width} and y: {image_buffer.spec().height}")

    if image_spec.width != image_spec.height * image_spec.height:
        _LOGGER.info(f"invalid input file dimensions. Expect lengthwise LUT with dimension W: s*s  X  H: s, where s is the size of the LUT")
        sys.exit(1)

    lut_intervals, lut_values = generate_lut_values(image_spec, image_buffer)

    write_3DL(args.o, image_spec.height, lut_intervals, lut_values)
    
    # write_azasset(file_path, lut_intervals, lut_values, azasset_json=AZASSET_LUT)
    write_azasset(args.o, lut_intervals, lut_values)

    # example from command line
    # python % DCCSI_COLORGRADING_SCRIPTS %\lut_helper.py - -i C: \Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_32_LUT.tiff - -op pre - grading - -shaper Log2 - 48nits - -o C: \Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-48nits_32_LUT.exr
