# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# lut_compositor.py

import sys
import os
import argparse
import logging as _logging

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.lut_compositor'

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

operations = {"composite": 0, "extract": 1}
invalidOp = -1

parser = argparse.ArgumentParser()
parser.add_argument('--i', type=str, required=True, help='input file')
parser.add_argument('--l', type=str, required=False, help='LUT to composite with screenshot in \'composite\' mode')
parser.add_argument('--op', type=str, required=True, help='operation. Should be \'composite\' or \'extract\'')
parser.add_argument('--s', type=int, required=True, help='size of the LUT (i.e. 16)')
parser.add_argument('--o', type=str, required=True, help='output file')
args = parser.parse_args()

op = operations.get(args.op, invalidOp)
if op == invalidOp:
    _LOGGER.warning("invalid operation")
    sys.exit(1)
elif op == 0:
    if args.l is None:
        _LOGGER.warning("no LUT file specified")
        sys.exit()

# read in the input image
image_buffer = oiio.ImageBuf(args.i)
image_spec = image_buffer.spec()
_LOGGER.info("Input resolution is ", image_buffer.spec().width, " x ", image_buffer.spec().height)

if op == 0:
    out_file_name = args.o
    _LOGGER.info("writing %s..." % (out_file_name))
    lut_buffer = oiio.ImageBuf(args.l)
    lut_spec = lut_buffer.spec()
    _LOGGER.info("Resolution is ", lut_buffer.spec().width, " x ", lut_buffer.spec().height)
    if lut_spec.width != lut_spec.height*lut_spec.height:
        _LOGGER.warning("invalid input file dimensions. Expect lengthwise LUT with dimension W: s*s  X  H: s, where s is the size of the LUT")
        sys.exit(1)
    lut_size = lut_spec.height
    out_image_spec = oiio.ImageSpec(image_spec.width, image_spec.height, 3, oiio.TypeFloat)
    out_image_buffer = oiio.ImageBuf(out_image_spec)
    out_image_buffer.write(out_file_name)
    for y in range(out_image_buffer.ybegin, out_image_buffer.yend):
        for x in range(out_image_buffer.xbegin, out_image_buffer.xend):
            src_pixel = image_buffer.getpixel(x, y)
            dst_pixel = (src_pixel[0], src_pixel[1], src_pixel[2])
            if y < lut_spec.height and x < lut_spec.width:
                lut_pixel = lut_buffer.getpixel(x, y)
                dst_pixel = (lut_pixel[0], lut_pixel[1], lut_pixel[2])
            out_image_buffer.setpixel(x, y, dst_pixel)
    out_image_buffer.write(out_file_name)
elif op == 1:
    out_file_name = args.o
    _LOGGER.info("writing %s..." % (out_file_name))
    lut_size = args.s
    lut_spec = oiio.ImageSpec(lut_size*lut_size, lut_size, 3, oiio.TypeFloat)
    lut_buffer = oiio.ImageBuf(lut_spec)
    for y in range(lut_buffer.ybegin, lut_buffer.yend):
        for x in range(lut_buffer.xbegin, lut_buffer.xend):
            src_pixel = image_buffer.getpixel(x, y)
            dst_pixel = (src_pixel[0], src_pixel[1], src_pixel[2])
            lut_buffer.setpixel(x, y, dst_pixel)
    lut_buffer.write(out_file_name)