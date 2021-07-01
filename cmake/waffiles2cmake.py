#!/usr/bin/env python3
#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from __future__ import (absolute_import, division,
                        print_function, unicode_literals)

import sys
import json
import os
import subprocess
import argparse

def get_banner():
    return """#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""

def convertFile(input_file):
    filename = os.path.basename(input_file)
    path = os.path.dirname(os.path.abspath(input_file))
    outFilename = (os.path.splitext(filename)[0] + '_files.cmake').lower()
    output_file = os.path.join(path, outFilename)

    print('Converting ' + os.path.abspath(input_file) + ' to ' + output_file)

    # parse input file
    try:
        with open(input_file, 'r') as source_file:
            waf_files = json.load(source_file)
    except IOError:
        print('Error opening ' + input_file)
        sys.exit(1)
    except ValueError:
        print('Error parsing ' + input_file + ': invalid JSON!')
        sys.exit(1)

    files_list = []

    for (i, j) in waf_files.items():
        for (k, grp) in j.items():
            for fname in grp:
                files_list.append(fname)

    alreadyExists = os.path.exists(output_file)
    if alreadyExists:
        subprocess.run(['p4', 'edit', output_file])

    # build output file list
    try:
        fhandle = open(output_file, 'w+')
        fhandle.write(get_banner() + '\nset(FILES\n')
        for fname in files_list:
            fhandle.write('    ' + fname + '\n')
        fhandle.write(')\n')
    except IOError:
        print('Error creating ' + output_file)

    if not alreadyExists:
        subprocess.run(['p4', 'add', output_file])

def convertPath(input_path):
    for dp, dn, filenames in os.walk(input_path):
        for f in filenames:
            if os.path.splitext(f)[1] == '.waf_files':
                convertFile(os.path.join(dp, f))

def main():
    """script main function"""
    parser = argparse.ArgumentParser(description='wafffiles2cmake.py (will recursively convert all waf_files)\n'
        'output: [file_or_dir. ..].cmake\n', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('file_or_dir', type=str, nargs='+',
                    help='list of files or directories to look for *.waf_files within and convert to cmake files')

    args = parser.parse_args()
    
    for input_file in args.file_or_dir:
        print(input_file)
        if os.path.splitext(input_file)[1] == '.waf_files':
            convertFile(input_file)
        elif os.path.isdir(input_file):
            for dp, dn, filenames in os.walk(input_file):
                for f in filenames:
                    if os.path.splitext(f)[1] == '.waf_files':
                        convertFile(os.path.join(dp, f))

#entrypoint
if __name__ == '__main__':
    main()
