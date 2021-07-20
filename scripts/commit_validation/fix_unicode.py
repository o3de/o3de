#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
replacement_map = {
    0xA0: ' ',
    0xA6: '|',
    0x2019: '\'',
    0x2014: '-',
    0x2191: '^',
    0x2212: '-',
    0x2217: '*',
    0x2248: 'is close to',
    0xFEFF: '',
}

def fixUnicode(input_file):
    try:
        basename = os.path.basename(input_file)
        for pattern in handled_file_patterns:
            if fnmatch.fnmatch(basename, pattern):
                with open(input_file, 'r', encoding='utf-8', errors='replace') as fh:
                    fileContents = fh.read()
                modified = False
                for uni, repl in replacement_map.items():
                    uni_str = chr(uni)
                    if uni_str in fileContents:
                        fileContents = fileContents.replace(uni_str, repl)
                        modified = True
                if modified:
                    with open(input_file, 'w') as destination_file:
                        destination_file.writelines(fileContents)
                    print(f'[INFO] Patched {input_file}')
                break

    except (IOError, UnicodeDecodeError) as err:
        print('[ERROR] reading {}: {}'.format(input_file, err))
        return

def main():
    """script main function"""
    parser = argparse.ArgumentParser(description='This script replaces unicode characters, some of them are replaced for spaces (e.g. xA0), others are replaced with the escape sequence',
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('file_or_dir', type=str, nargs='+',
        help='list of files or directories to search within for files to fix up unicode characters')

    args = parser.parse_args()

    for input_file in args.file_or_dir:
        if os.path.isdir(input_file):
            for dp, dn, filenames in os.walk(input_file):
                for f in filenames:
                    fixUnicode(os.path.join(dp, f))
        else:
            fixUnicode(input_file)

#entrypoint
if __name__ == '__main__':
    main()
