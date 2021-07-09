# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project
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
from env_bool import env_bool

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.lut_helper'

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


# ------------------------------------------------------------------------
# set up access to OpenImageIO, within o3de or without
try:
    # running in o3de
    import azlmbr
    from ColorGrading.initialize import start
    start()
except Exception as e:
    # running external, start this module from:
    # Gems\AtomLyIntegration\CommonFeatures\Editor\Scripts\ColorGrading\cmdline\O3DE_py_cmd.bat
    pass

    try:
        _O3DE_DEV = Path(os.getenv('O3DE_DEV'))
        os.environ['O3DE_DEV'] = pathlib.PureWindowsPath(_O3DE_DEV).as_posix()
        _LOGGER.debug(_O3DE_DEV)
    except EnvironmentError as e:
        _LOGGER.error('O3DE engineroot not set or found')
        raise e

    try:
        _O3DE_BIN_PATH = Path(str(_O3DE_DEV))
        _O3DE_BIN = Path(os.getenv('O3DE_BIN', _O3DE_BIN_PATH.resolve()))
        os.environ['O3DE_BIN'] = pathlib.PureWindowsPath(_O3DE_BIN).as_posix()
        _LOGGER.debug(_O3DE_BIN)
        site.addsitedir(_O3DE_BIN)
    except EnvironmentError as e:
        _LOGGER.error('O3DE bin folder not set or found')
        raise e

try:
    import OpenImageIO as oiio
except ImportError as e:
    _LOGGER.error('OpenImageIO not found')
    raise e
# ------------------------------------------------------------------------

# Transform from high dynamic range to normalized
def ShaperInv(bias, scale, v):
    return math.pow(2.0, (v - bias)/scale)

# Transform from normalized range to high dynamic range
def Shaper(bias, scale, v):
    return math.log(v, 2.0) * scale + bias

def GetUvCoord(size, r, g, b):
    u = g * lutSize + r
    v = b
    return (u, v)

def clamp(v):
    return max(0.0, min(1.0, v))

shaperPresets = {"Log2-48nits":(-6.5, 6.5),
                 "Log2-1000nits":(-12.0, 10.0),
                 "Log2-2000nits":(-12.0, 11.0),
                 "Log2-4000nits":(-12.0, 12.0)}

operations = {"pre-grading":0, "post-grading":1}

parser = argparse.ArgumentParser()
parser.add_argument('--i', type=str, required=True, help='input file')
parser.add_argument('--shaper', type=str, required=True, help='shaper preset. Should be one of \'Log2-48nits\', \'Log2-1000nits\', \'Log2-2000nits\', \'Log2-4000nits\'')
parser.add_argument('--op', type=str, required=True, help='operation. Should be \'pre-grading\' or \'post-grading\'')
parser.add_argument('--o', type=str, required=True, help='output file')
parser.add_argument('-l', dest='write3dl', action='store_true', help='output 3dl file')
parser.add_argument('-a', dest='writeAsset', action='store_true', help='write out azasset file')
args = parser.parse_args()

# Check for valid shaper type
invalidShaper = (0,0)
invalidOp = -1

shaperLimits = shaperPresets.get(args.shaper, invalidShaper)
if shaperLimits == invalidShaper:
    print("invalid shaper")
    sys.exit(1)
op = operations.get(args.op, invalidOp)
if op == invalidOp:
    print("invalid operation")
    sys.exit(1)

# Read input image
#buf = oiio.ImageBuf("linear_lut.exr")
buf = oiio.ImageBuf(args.i)
inSpec = buf.spec()
print("Resolution is ", buf.spec().width, " x ", buf.spec().height)
if inSpec.width != inSpec.height*inSpec.height:
    print("invalid input file dimensions. Expect lengthwise LUT with dimension W: s*s  X  H: s, where s is the size of the LUT")
    sys.exit(1)
lutSize = inSpec.height

middleGrey = 0.18
lowerStops = shaperLimits[0]
upperStops = shaperLimits[1]

logMin = math.log(middleGrey * math.pow(2.0, lowerStops), 2.0)
logMax = math.log(middleGrey * math.pow(2.0, upperStops), 2.0)
scale = 1.0/(logMax - logMin)
bias = -scale * logMin

print("Shaper: range in stops  %.1f -> %.1f  (linear: %.3f -> %.3f)  logMin %.3f  logMax %.3f  scale %.3f  bias %.3f\n" %
      (lowerStops, upperStops, middleGrey * math.pow(2.0, lowerStops), middleGrey * math.pow(2.0, upperStops),
       logMin, logMax, scale, bias))

# Create a writing image
outSpec = oiio.ImageSpec(buf.spec().width, buf.spec().height, 3, "float")
outBuf = oiio.ImageBuf(outSpec)

# Set the destination image pixels by applying the shaperfunction
for y in range(outBuf.ybegin, outBuf.yend):
    for x in range(outBuf.xbegin, outBuf.xend):
        srcPx = buf.getpixel(x, y)
        if op == 0:
            dstPx = (ShaperInv(bias, scale, srcPx[0]), ShaperInv(bias, scale, srcPx[1]), ShaperInv(bias, scale, srcPx[2]))
            outBuf.setpixel(x, y, dstPx)
        elif op == 1:
            dstPx = (Shaper(bias, scale, srcPx[0]), Shaper(bias, scale, srcPx[1]), Shaper(bias, scale, srcPx[2]))
            outBuf.setpixel(x,y, dstPx)
        else:
            # Unspecified operation. Just write zeroes
            outBuf.setpixel(x, y, 0.0, 0.0, 0.0)
print("writing %s..." % (args.o))
outBuf.write(args.o, "float")

lutIntervals = []
lutValues = []
if args.write3dl or args.writeAsset:
    # First line contains the vertex intervals
    dv = 1023.0 / float(lutSize-1)
    for i in range(lutSize):
        lutIntervals.append(int(dv * i))
    # Texels are in R G B per line with indices increasing first with blue, then green, and then red.
    for r in range(lutSize):
        for g in range(lutSize):
            for b in range(lutSize):
                uv = GetUvCoord(lutSize, r, g, b)
                px = outBuf.getpixel(uv[0], uv[1])
                lutValues.append((int(px[0] * 4095), int(px[1] * 4095), int(px[2] * 4095)))

if args.write3dl:
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

if args.writeAsset:
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