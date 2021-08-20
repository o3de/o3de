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

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.from_3dl_to_azasset'

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

DEFAULT_AZASSET_HEADER = """\
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "LookupTableAsset",
    "ClassData": {
        "Name": "LookupTable",
        "Intervals": [
}
"""

def write_azasset(file_path, lut_size, lut_intervals, lut_values, header=DEFAULT_AZASSET_HEADER):
    asset_file_path = f"{file_path}.azasset"
    _LOGGER.info(f"Writting {asset_file_path}...")
    asset_file = open(asset_file_path, 'w')
    asset_file.write(header)
    for i in range(lut_size):
        asset_file.write(" %d" % (lut_intervals[i]))
        if i < (lut_size - 1):
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


# ------------------------------------------------------------------------
def convert_3dl_to_azasset(input_file, output_file):
    lutIntervals = []
    lutValues = []
    with open(input_file) as lut_file:
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
                
    write_azasset(output_file)


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""

    input_file = ''
    output_file = ''
    try:
        opts, args = getopt.getopt(sys.argv[1:],"hi:o:",["ifile=","ofile="])
    except getopt.GetoptError:
        _LOGGER.info('test.py -i <input_file> -o <output_file>')
        sys.exit(2)
    for opt, arg in opts:
        _LOGGER.info(f"opt {opt}  arg {arg}")
        if opt == '-h':
            _LOGGER.info('test.py -i <input_file> -o <output_file>')
            sys.exit()
        elif opt in ("-i", "--ifile"):
            input_file = arg
        elif opt in ("-o", "--ofile"):
            output_file = arg
    _LOGGER.info(f'Input file is "{input_file}"')
    _LOGGER.info(f'Output file is "{output_file}"')
    if input_file == '' or output_file == '':
        _LOGGER.info('test.py -i <input_file> -o <output_file>')
    else:
        convert_3dl_to_azasset(input_file, output_file)
