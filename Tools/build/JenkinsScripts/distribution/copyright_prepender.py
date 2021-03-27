"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import os
import stat
import shutil
import argparse

flagged_extensions = ['.c', '.cpp', '.h', '.hpp', '.inl']
skippable_extensions = ['.log', '.p4ignore', '.obj', '.dll', '.png', '.pdb', '.dylib', '.lib',\
                        '.exe', '.flt', '.asi', '.exp', '.ilk', '.pch', '.res', '.bmp', '.cur',\
                        '.ico', '.resx', '.jpg', '.psd', '.gif', '.a', '.fxcb', '.icns', '.cab',\
                        '.chm', '.hxc', '.xsd', '.tif']

copyright_path = os.path.join( os.path.dirname(os.path.realpath(__file__)), 'copyright.txt')

prepend_yes_log = open('prepend_yes.log', 'w')
prepend_no_log = open('prepend_no.log', 'w')
prepend_skip_log = open('prepend_skip.log', 'w')

gQuietMode = False

def prepend_copyrights():
    for dirname, dirnames, filenames in os.walk('.'):
        for filename in filenames:
            full_filename = os.path.join(dirname, filename)
            if copyright_required(filename):
                apply_copyright(full_filename)
            elif skippable(filename):
                if not gQuietMode:
                    print 'Skipping ' + full_filename
                prepend_skip_log.write('Skipping ' + full_filename + '\n')
            else:
                if not gQuietMode:
                    print 'Not prepending to ' + full_filename 
                prepend_no_log.write('Not prepending to ' + full_filename + '\n')

def apply_copyright(full_filename):
    if not gQuietMode:
        print 'Prepending copyright to ' +  full_filename 
    prepend_yes_log.write('Prepending copyright to ' +  full_filename + '\n')
    temp_file = full_filename + '_temp'
    os.rename(full_filename, temp_file)
    shutil.copyfile(copyright_path, full_filename)
    with open(full_filename, 'a') as f:
        with open(temp_file) as t:
            for line in t:
                f.write(line)
    os.chmod(temp_file, stat.S_IWRITE)
    os.remove(temp_file)

def copyright_required(filename):
    return os.path.splitext(filename)[1] in flagged_extensions

def skippable(filename):
    return os.path.splitext(filename)[1] in skippable_extensions


def main():
    prepend_copyrights()
    prepend_yes_log.close()
    prepend_no_log.close()
    prepend_skip_log.close()


if __name__ =="__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument( '-q', '--quiet', dest='quiet', help='quiet mode - only print output to logs', action='store_true')

    args = parser.parse_args()
    gQuietMode = args.quiet

    main()

