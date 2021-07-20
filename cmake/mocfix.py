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
import re
import subprocess
import argparse

mocre = re.compile(r'[\/<"](([^.\/<]*)\.moc)')
alreadyFixedMocre = re.compile(r'[\/<"]moc_(([^.\/<]*)\.cpp)')
includere = re.compile(r'^[\s]*#include[\s]?')
qobjectre = re.compile(r'^[\s]*Q_OBJECT \/\/[\s]*AUTOMOC')
qmocrun_start = '#if !defined(Q_MOC_RUN)'
qmocrun_end = '#endif'
header_extensions = ['.h', '.hxx']

def fixAutoMocHeader(input_file):
    try:
        with open(input_file, 'r') as source_file:
            print("Considering file {} for automoc fix".format(os.path.abspath(input_file)))
            fileLines = source_file.readlines()
    except (IOError, UnicodeDecodeError) as err:
        print('Error reading {}: {}'.format(input_file, err))
        return
    for line_number in range(0, len(fileLines)):
        if fileLines[line_number].find(qmocrun_start) != -1:
            print("Already fixed file {}".format(os.path.abspath(input_file)))
            break
        reResult = qobjectre.search(fileLines[line_number])
        if reResult:
            fixHFile(input_file)
            return

def fixHFile(input_file):
    try:
        with open(input_file, 'r') as source_file:
            print("Considering file {} for header fix".format(os.path.abspath(input_file)))
            try:
                fileLines = source_file.readlines()
            except UnicodeDecodeError as err:
                print('Error reading file {}, err: {}'.format(input_file, err))
                return

        first_include_line_number = -1
        last_include_line_number = -1
        for line_number in range(0, len(fileLines)):
            if fileLines[line_number].find(qmocrun_start) != -1:
                # Already injected Q_MOC_RUN guard
                print("Already fixed file {}".format(os.path.abspath(input_file)))
                break
            reResult = includere.search(fileLines[line_number])
            if reResult:
                if first_include_line_number == -1:
                    first_include_line_number = line_number
                last_include_line_number = line_number

        if first_include_line_number != -1 and last_include_line_number != -1:
            print('{}:{},{} Inserting Q_MOC_RUN'.format(os.path.abspath(input_file), first_include_line_number, last_include_line_number))
            fileLines.insert(last_include_line_number+1, qmocrun_end + '\n')
            fileLines.insert(first_include_line_number, qmocrun_start +'\n')

            # p4 edit the file
            retProcess = subprocess.run(['p4', 'edit', input_file])
            if retProcess.returncode != 0:
                print('Error opening {}: {}'.format(input_file, retProcess.returncode))
                sys.exit(1)
            with open(input_file, 'w') as destination_file:
                destination_file.writelines(fileLines)
    except IOError as err:
        print('Error opening {}: {}'.format(input_file, err))
        return

def fixCppFile(input_file):
    # parse input file
    try:
        hasEdit = False
        with open(input_file, 'r') as source_file:
            print("Reading file " + os.path.abspath(input_file))
            try:
                fileLines = source_file.readlines()
            except UnicodeDecodeError as err:
                print('Error reading file {}, err: {}'.format(input_file, err))
                return
            for line_number in range(0, len(fileLines)):
                if alreadyFixedMocre.search(fileLines[line_number]):
                    for h_extension in header_extensions:
                        if os.path.exists(os.path.splitext(input_file)[0] + h_extension):
                            fixHFile(os.path.splitext(input_file)[0] + h_extension)
                reResult = mocre.search(fileLines[line_number])
                while reResult:
                    # there is a match, we need to replace
                    hasEdit = True
                    # replace using the group
                    newInclude = 'moc_' + reResult.group(2) + '.cpp'
                    print('{}:{} Converting {} to {} '.format(os.path.abspath(input_file), line_number, reResult.group(1), newInclude))
                    fileLines[line_number] = fileLines[line_number].replace(reResult.group(1), newInclude)
                    for h_extension in header_extensions:
                        if os.path.exists(os.path.splitext(input_file)[0] + h_extension):
                            fixHFile(os.path.splitext(input_file)[0] + h_extension)
                    reResult = mocre.search(fileLines[line_number])
        if hasEdit:
            # p4 edit the file
            retProcess = subprocess.run(['p4', 'edit', input_file])
            if retProcess.returncode != 0:
                print('Error opening {}: {}'.format(input_file, retProcess.returncode))
                sys.exit(1)
            with open(input_file, 'w') as destination_file:
                destination_file.writelines(fileLines)
    except IOError as err:
        print('Error opening {}: {}'.format(input_file, err))
        return


def fileMayRequireFixing(f):
    return os.path.splitext(f)[1].lower() == '.cpp'

def main():
    """script main function"""
    parser = argparse.ArgumentParser(description='This script converts includes of moc files from\n'
    '#include .*filename.moc -> #include .*moc_filename.cpp',
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('file_or_dir', type=str, nargs='+',
        help='list of files or directories to search within for cpp files to fix up moc includes')

    args = parser.parse_args()

    for input_file in args.file_or_dir:
        if os.path.isdir(input_file):
            for dp, dn, filenames in os.walk(input_file):
                for f in filenames:
                    extension = os.path.splitext(f)[1]
                    extension_lower = extension.lower()
                    if extension_lower == '.cpp':
                        fixCppFile(os.path.join(dp, f))
                    elif extension_lower in header_extensions:
                        fixAutoMocHeader(os.path.join(dp, f))
        else:
            extension = os.path.splitext(input_file)[1]
            extension_lower = extension.lower()
            if extension_lower == '.cpp':
                fixCppFile(input_file)
            elif extension_lower in header_extensions:
                fixAutoMocHeader(input_file)

#entrypoint
if __name__ == '__main__':
    main()
