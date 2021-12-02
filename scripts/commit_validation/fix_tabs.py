#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import fnmatch
import os

handled_file_patterns = [
    '*.c', '*.cc', '*.cpp', '*.cxx', '*.h', '*.hpp', '*.hxx', '*.inl', '*.m', '*.mm', '*.cs', '*.java',
    '*.py', '*.lua', '*.bat', '*.cmd', '*.sh', '*.js',
    '*.cmake', 'CMakeLists.txt'
]
def fixTabs(input_file):
    try:
        basename = os.path.basename(input_file)
        for pattern in handled_file_patterns:
            if fnmatch.fnmatch(basename, pattern):
                with open(input_file, 'r') as source_file:
                    fileContents = source_file.read()
                if '\t' in fileContents:
                    newFileContents = fileContents.replace('\t', '    ')
                    with open(input_file, 'w') as destination_file:
                        destination_file.write(newFileContents)
                    print(f'[INFO] Patched {input_file}')
                break

    except (IOError, UnicodeDecodeError) as err:
        print('[ERROR] reading {}: {}'.format(input_file, err))
        return

def main():
    """script main function"""
    parser = argparse.ArgumentParser(description='This script replaces tabs with spaces',
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('file_or_dir', type=str, nargs='+',
        help='list of files or directories to search within for files to fix up tabs')

    args = parser.parse_args()

    for input_file in args.file_or_dir:
        if os.path.isdir(input_file):
            for dp, dn, filenames in os.walk(input_file):
                for f in filenames:
                    fixTabs(os.path.join(dp, f))
        else:
            fixTabs(input_file)

#entrypoint
if __name__ == '__main__':
    main()
