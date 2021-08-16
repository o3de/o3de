# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# lut_compositor.py

import sys
import os
import site
import argparse
import math
import pathlib
from pathlib import Path
import logging as _logging
from env_bool import env_bool

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.lut_compositor'

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

import ColorGrading.initialize
ColorGrading.initialize.start()

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
    print("invalid operation")
    sys.exit(1)
elif op == 0:
    if args.l is None:
        print("no LUT file specified")
        sys.exit()

# read in the input image
inBuf = oiio.ImageBuf(args.i)
inSpec = inBuf.spec()
print("Input resolution is ", inBuf.spec().width, " x ", inBuf.spec().height)

if op == 0:
    outFileName = args.o
    print("writing %s..." % (outFileName))
    lutBuf = oiio.ImageBuf(args.l)
    lutSpec = lutBuf.spec()
    print("Resolution is ", lutBuf.spec().width, " x ", lutBuf.spec().height)
    if lutSpec.width != lutSpec.height*lutSpec.height:
        print("invalid input file dimensions. Expect lengthwise LUT with dimension W: s*s  X  H: s, where s is the size of the LUT")
        sys.exit(1)
    lutSize = lutSpec.height
    outSpec = oiio.ImageSpec(inSpec.width, inSpec.height, 3, oiio.TypeFloat)
    outBuf = oiio.ImageBuf(outSpec)
    outBuf.write(outFileName)
    for y in range(outBuf.ybegin, outBuf.yend):
        for x in range(outBuf.xbegin, outBuf.xend):
            srcPx = inBuf.getpixel(x, y)
            dstPx = (srcPx[0], srcPx[1], srcPx[2])
            if y < lutSpec.height and x < lutSpec.width:
                lutPx = lutBuf.getpixel(x, y)
                dstPx = (lutPx[0], lutPx[1], lutPx[2])
            outBuf.setpixel(x, y, dstPx)
    outBuf.write(outFileName)
elif op == 1:
    outFileName = args.o
    print("writing %s..." % (outFileName))
    lutSize = args.s
    lutSpec = oiio.ImageSpec(lutSize*lutSize, lutSize, 3, oiio.TypeFloat)
    lutBuf = oiio.ImageBuf(lutSpec)
    for y in range(lutBuf.ybegin, lutBuf.yend):
        for x in range(lutBuf.xbegin, lutBuf.xend):
            srcPx = inBuf.getpixel(x, y)
            dstPx = (srcPx[0], srcPx[1], srcPx[2])
            lutBuf.setpixel(x, y, dstPx)
    lutBuf.write(outFileName)