# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# lut_helper.py

import sys
import os
import argparse
import math
import site
import pathlib
from pathlib import Path
import logging as _logging
import numpy as np
from pathlib import Path

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.lut_helper'

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
# Transform from high dynamic range to normalized
from ColorGrading import inv_shaper_transform

# Transform from normalized range to high dynamic range
from ColorGrading import shaper_transform

# utils
from ColorGrading import get_uv_coord
from ColorGrading import log2
from ColorGrading import is_power_of_two

shaper_presets = {"Log2-48nits": (-6.5, 6.5),
                 "Log2-1000nits": (-12.0, 10.0),
                 "Log2-2000nits": (-12.0, 11.0),
                 "Log2-4000nits": (-12.0, 12.0)}

def transform_exr(image_buffer, out_image_buffer, op, out_path, write_exr):
    # Set the destination image pixels by applying the shaperfunction
    for y in range(out_image_buffer.ybegin, out_image_buffer.yend):
        for x in range(out_image_buffer.xbegin, out_image_buffer.xend):
            src_pixel = image_buffer.getpixel(x, y)
            # _LOGGER.debug(f'src_pixel is: {src_pixel}')
            if op == 0:
                dst_pixel = (inv_shaper_transform(bias, scale, src_pixel[0]),
                             inv_shaper_transform(bias, scale, src_pixel[1]),
                             inv_shaper_transform(bias, scale, src_pixel[2]))
                out_image_buffer.setpixel(x, y, dst_pixel)
            elif op == 1:
                dst_pixel = (shaper_transform(bias, scale, src_pixel[0]),
                             shaper_transform(bias, scale, src_pixel[1]),
                             shaper_transform(bias, scale, src_pixel[2]))
                out_image_buffer.setpixel(x, y, dst_pixel)
            else:
                # Unspecified operation. Just write zeroes
                out_image_buffer.setpixel(x, y, 0.0, 0.0, 0.0)

    if write_exr:
        _LOGGER.info(f"writing {out_path}.exr ...")
        out_image_buffer.write(out_path + '.exr', "float")
    
    return out_image_buffer


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""

    operations = {"pre-grading": 0, "post-grading": 1}

    parser = argparse.ArgumentParser()
    parser.add_argument('--i', type=str, required=True, help='input file')
    parser.add_argument('--shaper', type=str, required=True,
                        help='shaper preset. Should be one of \'Log2-48nits\', \'Log2-1000nits\', \'Log2-2000nits\', \'Log2-4000nits\'')
    parser.add_argument('--op', type=str, required=True, help='operation. Should be \'pre-grading\' or \'post-grading\'')
    parser.add_argument('--o', type=str, required=True, help='output file')
    parser.add_argument('-e', dest='writeExr', action='store_true', help='output lut as exr file (float)')
    parser.add_argument('-l', dest='write3dl', action='store_true', help='output lut as .3dl file')
    parser.add_argument('-a', dest='writeAsset', action='store_true', help='write out lut as O3dE .azasset file')
    args = parser.parse_args()

    # Check for valid shaper type
    invalid_shaper = (0, 0)
    invalid_op = -1

    shaper_limits = shaper_presets.get(args.shaper, invalid_shaper)
    if shaper_limits == invalid_shaper:
        _LOGGER.error("invalid shaper")
        sys.exit(1)
    op = operations.get(args.op, invalid_op)
    if op == invalid_op:
        _LOGGER.error("invalid operation")
        sys.exit(1)

    # input validation
    input_file = Path(args.i)
    if input_file.is_file():
        # file exists
        pass
    else:
        FILE_ERROR_MSG = f'File does not exist: {input_file}'
        _LOGGER.error(FILE_ERROR_MSG)
        #raise FileNotFoundError(FILE_ERROR_MSG)
        sys.exit(1)

    # Read input image
    #buf = oiio.ImageBuf("linear_lut.exr")
    image_buffer = oiio.ImageBuf(args.i)
    image_spec = image_buffer.spec()

    _LOGGER.info(f"Resolution is x:{image_spec.height}, y:{image_spec.width}")

    if image_spec.height < 16:
        _LOGGER.info(f"invalid input file dimensions: x is {image_spec.height}. Expected LUT with height dimension >= 16 pixels")
        sys.exit(1)

    if not is_power_of_two(image_buffer.spec().height):
        _LOGGER.info(f"invalid input file dimensions: {buf.spec().height}. Expected LUT dimensions power of 2: 16, 32, or 64 height")
        sys.exit(1)

    elif image_spec.width != image_spec.height * image_spec.height:
        _LOGGER.info("invalid input file dimensions. Expect lengthwise LUT with dimension W: s*s  X  H: s, where s is the size of the LUT")
        sys.exit(1)

    lut_size = image_spec.height

    middle_grey = 0.18
    lower_stops = shaper_limits[0]
    upper_stops = shaper_limits[1]

    middle_grey = math.log(middle_grey, 2.0)
    log_min = middle_grey + lower_stops
    log_max = middle_grey + upper_stops
    
    scale = 1.0 / (log_max - log_min)
    bias = -scale * log_min

    _LOGGER.info("Shaper: range in stops  %.1f -> %.1f  (linear: %.3f -> %.3f)  logMin %.3f  logMax %.3f  scale %.3f  bias %.3f\n" %
                 (lower_stops, upper_stops, middle_grey * math.pow(2.0, lower_stops), middle_grey * math.pow(2.0, upper_stops),
                  log_min, log_max, scale, bias))
    
    buffer_name = Path(args.o).name
    
    # Create a writing image
    out_image_spec = oiio.ImageSpec(image_buffer.spec().width, image_buffer.spec().height, 3, "float")
    out_image_buffer = oiio.ImageBuf(out_image_spec)

    # write out the modified exr file
    write_exr = False
    if args.writeExr:
         write_exr = True
    out_image_buffer = transform_exr(image_buffer, out_image_buffer, op, args.o, write_exr)

    from ColorGrading.exr_to_3dl_azasset import generate_lut_values
    lut_intervals, lut_values = generate_lut_values(image_spec, out_image_buffer)

    if args.write3dl:
        from ColorGrading.exr_to_3dl_azasset import write_3DL
        write_3DL(args.o, lut_size, lut_intervals, lut_values)

    if args.writeAsset:
        from ColorGrading import AZASSET_LUT
        from ColorGrading.from_3dl_to_azasset import write_azasset
        write_azasset(args.o, lut_intervals, lut_values, AZASSET_LUT)
