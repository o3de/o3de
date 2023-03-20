# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import numpy as np
import logging as _logging
from ColorGrading import get_uv_coord

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.azasset_converter_utils'

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# ------------------------------------------------------------------------

""" Utility functions for generating LUT azassets """
def generate_lut_values(image_spec, image_buffer):
    lut_size = image_spec.height

    lut_intervals = []
    lut_values = []

    # First line contains the vertex intervals
    dv = 1023.0 / float(lut_size-1)
    for i in range(lut_size):
        lut_intervals.append(np.uint16(dv * i))
    # Texels are in R G B per line with indices increasing first with blue, then green, and then red.
    for r in range(lut_size):
        for g in range(lut_size):
            for b in range(lut_size):
                uv = get_uv_coord(lut_size, r, g, b)
                px = np.array(image_buffer.getpixel(uv[0], uv[1]), dtype='f')
                px = np.clip(px, 0.0, 1.0)
                px = np.uint16(px * 4095)
                lut_values.append(px)

    return lut_intervals, lut_values

# To Do: add some input file validation
# If the input file doesn't exist, you'll get a LUT with res of 0 x 0 and result in a math error
#Resolution is 0  x  0
#writing C:\Depot\o3de-engine\Gems\AtomLyIntegration\CommonFeatures\Tools\ColorGrading\TestData\Nuke\HDR\Nuke_Post_grade_LUT.3dl...
#Traceback (most recent call last):
    #File "..\..\Editor\Scripts\ColorGrading\exr_to_3dl_azasset.py", line 103, in <module>
        #dv = 1023.0 / float(lutSize)
# ZeroDivisionError: float division by zero

def write_3DL(file_path, lut_size, lut_intervals, lut_values):
    lut_file_path = f'{file_path}.3dl'
    _LOGGER.info(f"Writing {lut_file_path}...")
    lut_file = open(lut_file_path, 'w')
    for i in range(lut_size):
        lut_file.write(f"{lut_intervals[i]} ")
    lut_file.write("\n")
    for px in lut_values:
        lut_file.write(f"{px[0]} {px[1]} {px[2]}\n")
    lut_file.close()

