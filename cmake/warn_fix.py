#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import sys
import json
import os
import re
import subprocess
import argparse

warnre = re.compile(r'(.*)\((\d*),(\d*)\): warning C(\d+): (.*)')
w4100_message_lambda = re.compile(r'.*\'(.*)\'.*')

# will contain a dictionary<filename, dictionary<linenumber, dictionary<column, warningcode>>>
all_warnings = dict()

def fix_debug_wrap(fix_function, warningNumber, fileLines, lineNumber, columnNumber, message):
    print('Input: ')
    print(fileLines[lineNumber].rstrip())
    print(' ' * (columnNumber-1) + f'^ w{warningNumber}')

    ret = fix_function(fileLines, lineNumber, columnNumber, message)

    if ret:
        print('Output: ')
        print(fileLines[lineNumber].rstrip())
    else:
        print('Unmodified')
    return ret

def fix_4100(fileLines, lineNumber, columnNumber, message):
    line = fileLines[lineNumber]
    startColumn = columnNumber-1
    lastNonSpacePosition = startColumn
    for colIndex in range(startColumn, 0, -1):
        if line[colIndex] in (',', '('):
            fileLines[lineNumber] = line[:colIndex+1] + ('',' ')[line[colIndex] == ','] + '[[maybe_unused]]' + ('',' ')[line[colIndex] == '('] + line[colIndex+1:]
            return True
        if line[colIndex] not in (' ', '\t'):
            lastNonSpacePosition = colIndex
    if lastNonSpacePosition < (startColumn-1): # cases where the parameter is in a new line
        fileLines[lineNumber] = line[:lastNonSpacePosition] + '[[maybe_unused]] ' + line[lastNonSpacePosition:]
        return True
    # This warning is also issued for lambdas, but in the lambda case, the warning points to the end of the lambda
    if line[columnNumber-1] == '}': # lambda case
        reResult = w4100_message_lambda.search(message)
        if reResult:
            variable = ' ' + reResult.group(1)
            # Now search lines above. Since the variable is not used in the lambda, we can iterate until we find it
            for lineIndex in range(lineNumber-1, 0, -1):
                col = fileLines[lineIndex].find(variable)
                if col != -1:
                    return fix_4100(fileLines, lineIndex, col, message)

    return False

def fix_4189(fileLines, lineNumber, columnNumber, message):
    del fileLines[lineNumber]
    return True

warning_fixers = dict()
warning_fixers[4100] = fix_4100
warning_fixers[4189] = fix_4189

def loadBuildLog(build_log):
    try:
        with open(build_log, 'r') as log_file:
            try:
                logLines = log_file.readlines()
            except UnicodeDecodeError as err:
                print('Error reading file {}, err: {}'.format(input_file, err))
                return
    except IOError as err:
        print('Error opening {}: {}'.format(input_file, err))
        return

    for logLine in logLines:
        reResult = warnre.search(logLine)
        if reResult:
            filename = os.path.abspath(reResult.group(1))
            lineNumber = int(reResult.group(2))
            columnNumber = int(reResult.group(3))
            warningNumber = int(reResult.group(4))
            message = reResult.group(5)

            (all_warnings.setdefault(filename, dict())
                         .setdefault(lineNumber, dict())
                         .setdefault(columnNumber, dict())
                         .setdefault(warningNumber, message))

def processWarnings():
    
    for filename, filename_warnings in all_warnings.items(): # file sorting irrelevant
        with open(filename, 'r') as read_file:
            try:
                fileLines = read_file.readlines()
            except UnicodeDecodeError as err:
                print('Error reading file {}, err: {}'.format(input_file, err))
                continue

        hasEdit = False
        for lineNumber, line_warnings in sorted(filename_warnings.items(), reverse=True): # Invert line number ordering to start from the bottom
            for columnNumber, column_warnings in sorted(line_warnings.items(), reverse=True): # Invert column number odering to start from the right
                for warningNumber, message in column_warnings.items(): # warning sorting irrelevant
                    if warningNumber in warning_fixers.keys():
                        #edited = fix_debug_wrap(warning_fixers[warningNumber], warningNumber, fileLines, lineNumber-1, columnNumber, message)
                        edited = warning_fixers[warningNumber](fileLines, lineNumber-1, columnNumber, message)
                        if not edited:
                            print(f'unfixed w{warningNumber}: {filename}({lineNumber},{columnNumber})')
                        hasEdit |= edited

        if hasEdit:
            # p4 edit the file
            # subprocess.check_call(['p4', 'edit', filename])
            with open(filename, 'w') as destination_file:
                destination_file.writelines(fileLines)

            # Stop during debugging
            #break

def main():
    """script main function"""
    parser = argparse.ArgumentParser(description='This script fixes warnings in msvc builds',
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('build_log', type=str,
        help='file with the build output')

    args = parser.parse_args()

    loadBuildLog(args.build_log)
    processWarnings()

#entrypoint
if __name__ == '__main__':
    main()
