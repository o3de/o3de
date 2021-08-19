# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import sys
import os
import argparse
import math
import site
import pathlib
from pathlib import Path
import logging as _logging
import numpy as np

# local imports
from ColorGrading import initialize_logger

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.exr_to_3dl_azasset'

import ColorGrading.initialize
ColorGrading.initialize.start()

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

try:
    import OpenImageIO as oiio
    pass
except ImportError as e:
    _LOGGER.error(f"invalid import: {e}")
    sys.exit(1)
# ------------------------------------------------------------------------


# ------------------------------------------------------------------------
# Transform from high dynamic range to normalized
def inv_shaper_transform(bias, scale, v):
    return math.pow(2.0, (v - bias) / scale)

# Transform from normalized range to high dynamic range
def shaper_transform(bias, scale, v):
    return math.log(v, 2.0) * scale + bias

def get_uv_coord(size, r, g, b):
    u = g * lut_size + r
    v = b
    return (u, v)

def clamp(v):
    return max(0.0, min(1.0, v))

def generate_lut_values(image_spec):
    lut_size = image_spec.height
    
    lut_intervals = []
    lut_values = []
    
    # First line contains the vertex intervals
    dv = 1023.0 / float(lut_size-1)
    for i in range(lut_size):
        lut_intervals.append(np.uint64(dv * i))
    # Texels are in R G B per line with indices increasing first with blue, then green, and then red.
    for r in range(lut_size):
        for g in range(lut_size):
            for b in range(lut_size):
                uv = get_uv_coord(lut_size, r, g, b)
                px = image_buffer.getpixel(uv[0], uv[1])
                lut_values.append((np.uint64(px[0] * 4095), np.uint64(px[1] * 4095), np.uint64(px[2] * 4095)))

    return lut_values

# To Do: add some input file validation
# If the input file doesn't exist, you'll get a LUT with res of 0 x 0 and result in a math error
#Resolution is 0  x  0
#writing C:\Depot\o3de-engine\Gems\AtomLyIntegration\CommonFeatures\Tools\ColorGrading\TestData\Nuke\HDR\Nuke_Post_grade_LUT.3dl...
#Traceback (most recent call last):
    #File "..\..\Editor\Scripts\ColorGrading\exr_to_3dl_azasset.py", line 103, in <module>
        #dv = 1023.0 / float(lutSize)
# ZeroDivisionError: float division by zero

def write_3DL(file_path=args.o):
    lut_file_path = f'{file_path}.3dl'
    _LOGGER.info(f"Writing {lut_file_path}...")
    lut_file = open(lut_file_path, 'w')
    # First line contains the vertex intervals
    dv = 1023.0 / float(lut_size)
    for i in range(lut_size):
        lut_file.write("%d " % (lut_intervals[i]))
    lut_file.write("\n")
    for px in lut_values:
        lut_file.write("%d %d %d\n" % (px[0], px[1], px[2]))
    lut_file.close()


DEFAULT_AZASSET_HEADER = """\
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "LookupTableAsset",
    ...
}
"""

def write_azasset(file_path=args.o, header=DEFAULT_AZASSET_HEADER):
    asset_file_path = f"{file_path}.azasset"
    _LOGGER.info(f"Writting {asset_file_path}...")
    asset_file = open(asset_file_path, 'w')
    f.write(header)
    for i in range(lut_size):
        asset_file.write(" %d" % (lut_intervals[i]))
        if i < (lut_size-1):
            asset_file.write(", ")
    asset_file.write("],\n")
    asset_file.write("        \"Values\": [\n")
    for idx, px in enumerate(lut_values):
        asset_file.write(" %d, %d, %d" % (px[0], px[1], px[2]))
        if idx < (len(lut_values) - 1):
            asset_file.write(",")
        asset_file.write("\n")
    asset_file.write("]\n")
    asset_file.write("    }\n")
    asset_file.write("}\n")
    asset_file.close()


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""
    
    parser = argparse.ArgumentParser()
    parser.add_argument('--i', type=str, required=True, help='input file')
    parser.add_argument('--o', type=str, required=True, help='output file')
    args = parser.parse_args()
    
    # Read input image
    #image_buffer = oiio.ImageBuf("linear_lut.exr")
    image_buffer = oiio.ImageBuf(args.i)
    image_spec = image_buffer.spec()
    _LOGGER.info(f"Resolution is, x: {image_buffer.spec().width} and y: {image_buffer.spec().height}")
    
    if image_spec.width != image_spec.height * image_spec.height:
        _LOGGER.info(f"invalid input file dimensions. Expect lengthwise LUT with dimension W: s*s  X  H: s, where s is the size of the LUT")
        sys.exit(1)
    
    lut_values = generate_lut_values(image_spec)

    write_3DL(args.o)
    write_azasset(args.o)
    
    # example from command line
    # python % DCCSI_COLORGRADING_SCRIPTS %\lut_helper.py - -i C: \Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_32_LUT.exr - -op pre - grading - -shaper Log2 - 48nits - -o C: \Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2 - 48nits_32_LUT.exr
