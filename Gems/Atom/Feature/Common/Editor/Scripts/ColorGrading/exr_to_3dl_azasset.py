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
from env_bool import env_bool
import numpy as np

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.exr_to_3dl_azasset'

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


# Transform from high dynamic range to normalized
def ShaperInv(bias, scale, v):
    return math.pow(2.0, (v - bias) / scale)

# Transform from normalized range to high dynamic range
def Shaper(bias, scale, v):
    return math.log(v, 2.0) * scale + bias

def GetUvCoord(size, r, g, b):
    u = g * lutSize + r
    v = b
    return (u, v)

def clamp(v):
    return max(0.0, min(1.0, v))

parser = argparse.ArgumentParser()
parser.add_argument('--i', type=str, required=True, help='input file')
parser.add_argument('--o', type=str, required=True, help='output file')
args = parser.parse_args()

# Read input image
#buf = oiio.ImageBuf("linear_lut.exr")
buf = oiio.ImageBuf(args.i)
inSpec = buf.spec()
print("Resolution is ", buf.spec().width, " x ", buf.spec().height)
if inSpec.width != inSpec.height*inSpec.height:
    print("invalid input file dimensions. Expect lengthwise LUT with dimension W: s*s  X  H: s, where s is the size of the LUT")
    sys.exit(1)
lutSize = inSpec.height

lutIntervals = []
lutValues = []
if True:
    # First line contains the vertex intervals
    dv = 1023.0 / float(lutSize-1)
    for i in range(lutSize):
        lutIntervals.append(np.uint64(dv * i))
    # Texels are in R G B per line with indices increasing first with blue, then green, and then red.
    for r in range(lutSize):
        for g in range(lutSize):
            for b in range(lutSize):
                uv = GetUvCoord(lutSize, r, g, b)
                px = buf.getpixel(uv[0], uv[1])
                lutValues.append((np.uint64(px[0] * 4095), np.uint64(px[1] * 4095), np.uint64(px[2] * 4095)))

# To Do: add some input file validation
# If the input file doesn't exist, you'll get a LUT with res of 0 x 0 and result in a math error
#Resolution is 0  x  0
#writing C:\Depot\o3de-engine\Gems\AtomLyIntegration\CommonFeatures\Tools\ColorGrading\TestData\Nuke\HDR\Nuke_Post_grade_LUT.3dl...
#Traceback (most recent call last):
    #File "..\..\Editor\Scripts\ColorGrading\exr_to_3dl_azasset.py", line 103, in <module>
        #dv = 1023.0 / float(lutSize)
#ZeroDivisionError: float division by zero
if True:
    lutFileName = "%s.3dl" % (args.o)
    print("writing %s..." % (lutFileName))
    lutFile = open(lutFileName, 'w')
    # First line contains the vertex intervals
    dv = 1023.0 / float(lutSize)
    for i in range(lutSize):
        lutFile.write("%d " % (lutIntervals[i]))
    lutFile.write("\n")
    for px in lutValues:
        lutFile.write("%d %d %d\n" % (px[0], px[1], px[2]))
    lutFile.close()

if True:
    assetFileName = "%s.azasset" % (args.o)
    print("writing %s...", (assetFileName))
    assetFile = open(assetFileName, 'w')
    assetFile.write("{\n")
    assetFile.write("    \"Type\": \"JsonSerialization\",\n")
    assetFile.write("    \"Version\": 1,\n")
    assetFile.write("    \"ClassName\": \"LookupTableAsset\",\n")
    assetFile.write("    \"ClassData\": {\n")
    assetFile.write("        \"Name\": \"LookupTable\",\n")
    assetFile.write("        \"Intervals\": [\n")
    for i in range(lutSize):
        assetFile.write(" %d" % (lutIntervals[i]))
        if i < (lutSize-1):
            assetFile.write(", ")
    assetFile.write("],\n")
    assetFile.write("        \"Values\": [\n")
    for idx, px in enumerate(lutValues):
        assetFile.write(" %d, %d, %d" % (px[0], px[1], px[2]))
        if idx < (len(lutValues) - 1):
            assetFile.write(",")
        assetFile.write("\n")
    assetFile.write("]\n")
    assetFile.write("    }\n")
    assetFile.write("}\n")
    assetFile.close()