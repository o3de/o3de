#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import os
import re

copyrightre = re.compile(r'All or portions of this file Copyright \(c\) Amazon\.com')
copyright_text = """
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

""".splitlines(keepends=True)

class copyright_builder:

    def get(self):
        pass

    def apply(self, file_contents):
        new_file_contents = self.get()
        new_file_contents += '\n' + file_contents
        return new_file_contents

class copyright_builder_first_last_line(copyright_builder):
    def __init__(self, comment_str):
        self.comment_str = comment_str

    def get(self):
        copyright_str = self.comment_str + copyright_text[0]
        for i in range(1, len(copyright_text)-1):
            copyright_str += copyright_text[i]
        copyright_str += self.comment_str + copyright_text[len(copyright_text)-1]
        return copyright_str

class copyright_builder_each_line(copyright_builder):
    def __init__(self, comment_str):
        self.comment_str = comment_str

    def get(self):
        copyright_str = ''
        for i in range(0, len(copyright_text)):
            copyright_str += self.comment_str + copyright_text[i]
        return copyright_str

class copyright_builder_each_line_bat(copyright_builder_each_line):
    def __init__(self, comment_str):
        self.comment_str = comment_str
        self.echo_off_re = re.compile('@echo off\n', re.IGNORECASE) 

    def apply(self, file_contents):
        re_result = self.echo_off_re.search(file_contents)
        if re_result:
            return self.echo_off_re.sub(re_result.group(0) + '\n' + self.get() + '\n', file_contents)
        return super().apply(file_contents)

class copyright_builder_each_line_sh(copyright_builder_each_line):
    def __init__(self, comment_str):
        self.comment_str = comment_str
        self.bin_str = [
            '#!/bin/sh\n', 
            '#!/bin/bash\n'
        ]

    def apply(self, file_contents):
        for bin_str in self.bin_str:
            if bin_str in file_contents:
                return file_contents.replace(bin_str, bin_str + '\n' + self.get() + '\n')
        return super().apply(file_contents)

class copyright_builder_c(copyright_builder):
    def get(self):
        copyright_str = '/*' + copyright_text[0]
        for i in range(1, len(copyright_text)-1):
            copyright_str += '*' + copyright_text[i]
        copyright_str += '*\n'
        copyright_str += '*/' + copyright_text[len(copyright_text)-1]
        return copyright_str

copyright_comment_by_extension = {
    'lua': copyright_builder_each_line('-- '),
    'py': copyright_builder_first_last_line('"""'),
    'bat': copyright_builder_each_line_bat('REM '),
    'cmd': copyright_builder_each_line_bat('REM '),
    'sh': copyright_builder_each_line_sh('# '),
    'cs': copyright_builder_c(),
}

def fixCopyright(input_file):
    try:
        extension = os.path.splitext(input_file)[1].replace('.','')
        if not extension in copyright_comment_by_extension:
            print(f'[WARN] Extension {extension} not handled')
        else:
            copyright_builder = copyright_comment_by_extension[extension]
            with open(input_file, 'r') as source_file:
                fileContents = source_file.read()

            reResult = copyrightre.search(fileContents)
            if reResult:
                # Copyright found, skip
                return

            newFileContents = copyright_builder.apply(fileContents)
            
            with open(input_file, 'w') as destination_file:
                destination_file.write(newFileContents)

            print(f'[INFO] Patched {input_file}')

    except (IOError, UnicodeDecodeError) as err:
        print('[ERROR] reading {}: {}'.format(input_file, err))
        return

def main():
    """script main function"""
    parser = argparse.ArgumentParser(description='This script fixes copyright headers',
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('file_or_dir', type=str, nargs='+',
        help='list of files or directories to search within for files to fix up copyright headers')

    args = parser.parse_args()

    for input_file in args.file_or_dir:
        if os.path.isdir(input_file):
            for dp, dn, filenames in os.walk(input_file):
                for f in filenames:
                    fixCopyright(os.path.join(dp, f))
        else:
            fixCopyright(input_file)

#entrypoint
if __name__ == '__main__':
    main()
