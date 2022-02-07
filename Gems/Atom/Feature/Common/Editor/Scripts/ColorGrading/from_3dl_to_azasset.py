# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""Takes a 3dl file and encodes it into a .azasset file that can be read by the AssetProcessor"""
# ------------------------------------------------------------------------
import json
import sys, getopt
import logging as _logging
# ------------------------------------------------------------------------


# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.from_3dl_to_azasset'

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

from ColorGrading import AZASSET_LUT

# ------------------------------------------------------------------------
def find_first_line(alist):
    for lno, line in enumerate(alist):
        if line[0].isdigit():  # skip non-number metadata
            return lno
# ------------------------------------------------------------------------


# ------------------------------------------------------------------------
def write_azasset(file_path, lut_intervals, lut_values, azasset_json=AZASSET_LUT):
    values_str = ''
    for idx, px in enumerate(lut_values):
        values_str += f"{px[0]}, {px[1]}, {px[2]}"
        if idx < (len(lut_values) - 1):
            values_str += ",\n"

    intervals_str = ', '.join(map(str, lut_intervals))

    azasset_json = azasset_json % (intervals_str, values_str)

    asset_file_path = f"{file_path}.azasset"
    _LOGGER.info(f"Writting {asset_file_path}...")
    asset_file = open(asset_file_path, 'w')
    asset_file.write(azasset_json)
# ------------------------------------------------------------------------


# ------------------------------------------------------------------------
def convert_3dl_to_azasset(input_file, output_file):
    lut_intervals = []
    lut_values = []
    with open(input_file) as lut_file:
        alist = [line.rstrip().lstrip() for line in lut_file]
        intervals_idx = find_first_line(alist)
        
        lut_intervals = alist[intervals_idx].split(" ")

        lut_values_range = range(intervals_idx + 1, len(alist))
        for ln in lut_values_range:
            values = alist[ln].split(" ")
            if values is None or len(values) < 3:
                continue
            lut_values.append(values)

    write_azasset(output_file, lut_intervals, lut_values)
# ------------------------------------------------------------------------


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
