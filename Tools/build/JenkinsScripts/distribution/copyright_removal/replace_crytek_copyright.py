"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import re
import os
import shutil
import stat

max_num_lines_to_read = 30
copyright_removed_count = 0
special_case_count = 0
star_comments = re.compile(r'/\*.*\*/', re.DOTALL)
flagged_extensions = ['.c', '.cpp', '.h', '.hpp', '.inl']
skippable_extensions = ['.log', '.p4ignore', '.obj', '.dll', '.png', '.pdb', '.dylib', '.lib',\
                        '.exe', '.flt', '.asi', '.exp', '.ilk', '.pch', '.res', '.bmp', '.cur',\
                        '.ico', '.resx', '.jpg', '.psd', '.gif', '.a', '.fxcb', '.icns', '.cab',\
                        '.chm', '.hxc', '.xsd', '.tif', '.xml']
crytek_replacement_header = '// Original file Copyright Crytek GMBH or its affiliates, used under license.\n'
#categorizer = Categorizer()

def remove_crytek_copyrights():
    for dirname, dirnames, filenames in os.walk('.'):
        if skippable_directory(dirname):
            continue
        for filename in filenames: 
            full_filename = os.path.join(dirname, filename)
            if skippable_file(filename):
                continue
            elif cpp_style_file(filename):
                remove_crytek_copyright_from_file(full_filename)


def remove_crytek_copyright_from_file(full_filename): 
    comment_start_index, comment_end_index, comment_type = fetch_comment_indices_and_comment_type(full_filename)
    if crytek_copyright_not_found(comment_start_index, comment_end_index, full_filename):
        return
    temp_file = full_filename + '_temp'
    shutil.copyfile(full_filename, temp_file)
    os.chmod(full_filename, stat.S_IWRITE)
    with open(temp_file, 'r') as t, open(full_filename, 'w') as f:
        # first, put in our replacement header
        f.write(crytek_replacement_header)
        for line_index, line in enumerate(t):
            if inside_copyright_block(line_index, comment_start_index, comment_end_index):
                continue
            else:
                f.write(line)
    os.chmod(temp_file, stat.S_IWRITE)
    os.remove(temp_file)

def fetch_comment_indices_and_comment_type(full_filename):
    start_index = -1
    end_index = -1
    comment_type = None
    with open(full_filename, 'r') as f:
        for index, line in enumerate(f):
            if index <= max_num_lines_to_read:
                start_index, end_index, comment_type = update_indices_and_type(start_index, end_index, comment_type, index, line)
            else:
                break
    return start_index, end_index, comment_type

def update_indices_and_type(start_index, end_index, comment_type, index, line):
    # set start, end, or comment type if they haven't been set yet
    if start_index == -1:
        if '/*' in line:
            start_index = index
    if end_index == -1:
        if '*/' in line:
            end_index = index

    if comment_type == None:
        if '/*' in line:
            comment_type = 'star'
    return start_index, end_index, comment_type

def crytek_copyright_detected(lines):
    return 'Crytek' in lines and 'Copyright' in lines

def skippable_directory(directory):
    skippable = 'SDKs' in directory
    if skippable:
        log('skipping directory' + directory)
    return skippable

def skippable_file(filename):
    skippable = os.path.splitext(filename)[1] in skippable_extensions
    if skippable:
        log('skipping file ' + filename)
    return skippable

def cpp_style_file(filename):
    return os.path.splitext(filename)[1] in flagged_extensions

def crytek_copyright_not_found(comment_start_index, comment_end_index, filename):
    not_found = comment_start_index == -1 or comment_end_index == -1
    if not_found:
        log('Crytek copyright not found in file: {}.'.format(filename))
    else:
        log('replacing copyright of ' + filename)
    return not_found

def inside_copyright_block(line_index, comment_start_index, comment_end_index):
    return line_index >= comment_start_index and line_index <= comment_end_index


def main():
    remove_crytek_copyrights()


def log(msg, log_name=None):
    print msg
    if log_name != None:
        log_name.write(msg)

if __name__ == '__main__':
    main()
