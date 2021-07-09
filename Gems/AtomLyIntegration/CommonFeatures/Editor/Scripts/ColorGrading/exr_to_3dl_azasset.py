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
        lutIntervals.append(int(dv * i))
    # Texels are in R G B per line with indices increasing first with blue, then green, and then red.
    for r in range(lutSize):
        for g in range(lutSize):
            for b in range(lutSize):
                uv = GetUvCoord(lutSize, r, g, b)
                px = buf.getpixel(uv[0], uv[1])
                lutValues.append((int(px[0] * 4095), int(px[1] * 4095), int(px[2] * 4095)))

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