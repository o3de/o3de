# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Takes a 3dl file and encodes it into a .azasset file that can be read by the
# AssetProcessor

import json
import sys, getopt
import logging as _logging
from env_bool import env_bool

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.3dl_to_azasset'

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


################################################################################
gResourcePath = ''

def Convert3dlFile(inputFile, outputFile):
    lutIntervals = []
    lutValues = []
    with open(gResourcePath + inputFile) as lut_file:
        alist = [line.rstrip().lstrip() for line in lut_file]
        for idx, ln in enumerate(alist):
            if idx == 0:
                values = ln.split(" ")
                for v in values:
                    lutIntervals.append(v)
            else:
                values = ln.split(" ")
                if values is None or len(values) < 3:
                    continue
                lutValues.append(values)	
    # write to output file
    with open(gResourcePath + outputFile, mode='w') as f:
        f.write("{\n")
        f.write("	\"Type\": \"JsonSerialization\",\n")
        f.write("	\"Version\": 1,\n")
        f.write("	\"ClassName\": \"LookupTableAsset\",\n")
        f.write("	\"ClassData\": {\n")
        f.write("		\"Name\": \"LookupTable\",\n")
        f.write("		\"Intervals\": [\n")
        for idx, v in enumerate(lutIntervals):
            f.write(" %s" % (v))
            if idx < (len(lutIntervals)-1):
                f.write(", ")
        f.write("],\n")
        f.write("		\"Values\": [\n")	
        for idx, pix in enumerate(lutValues):
            f.write(" %s, %s, %s" % (pix[0], pix[1], pix[2]))
            if idx < (len(lutValues)-1):
                f.write(",")
            f.write("\n")
        f.write("]\n")
        f.write("	}\n")
        f.write("}\n")

inputfile = ''
outputfile = ''
try:
    opts, args = getopt.getopt(sys.argv[1:],"hi:o:",["ifile=","ofile="])
except getopt.GetoptError:
    print('test.py -i <inputfile> -o <outputfile>')
    sys.exit(2)
for opt, arg in opts:
    print("opt %s  arg %s" % (opt, arg))
    if opt == '-h':
        print('test.py -i <inputfile> -o <outputfile>')
        sys.exit()
    elif opt in ("-i", "--ifile"):
        inputfile = arg
    elif opt in ("-o", "--ofile"):
        outputfile = arg
print('Input file is "%s"' % inputfile)
print('Output file is "%s"' % outputfile)
if inputfile == '' or outputfile == '':
    print('test.py -i <inputfile> -o <outputfile>')
else:
    Convert3dlFile(inputfile, outputfile)
