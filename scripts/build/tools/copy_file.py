#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import os
import sys
import glob
import shutil


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--src-dir', dest='src_dir', required=True, help='Source directory to copy files from, if not specified, current directory is used.')
    parser.add_argument('-r', '--file-regex', dest='file_regex', required=True, help='Globbing pattern used to match file names to copy.')
    parser.add_argument('-t', '--target-dir', dest="target_dir", required=True, help='Target directory to copy files to.')
    args = parser.parse_args()
    if not os.path.isdir(args.src_dir):
        print('ERROR: src_dir is not a valid directory.')
        exit(1)
    return args


def extended_path(path):
    """
    Maximum Path Length Limitation on Windows is 260 characters, use extended-length path to bypass this limitation
    """
    if sys.platform in ('win32', 'cli') and len(path) >= 260:
        if path.startswith('\\'):
            return r'\\?\UNC\{}'.format(path.lstrip('\\'))
        else:
            return r'\\?\{}'.format(path)
    else:
        return path


def copy_file(src_dir, file_regex, target_dir):
    if not os.path.isdir(args.target_dir):
        os.makedirs(target_dir)
    for f in glob.glob(os.path.join(src_dir, file_regex), recursive=True):
        if os.path.isfile(f):
            relative_path = os.path.relpath(f, src_dir)
            target_file_path = os.path.join(target_dir, relative_path)
            target_file_dir = os.path.dirname(target_file_path)
            if not os.path.isdir(target_file_dir):
                os.makedirs(target_file_dir)
            shutil.copy2(f, extended_path(target_file_path))
            print(f'{f} -> {target_file_path}')


if __name__ == "__main__":
    args = parse_args()
    copy_file(args.src_dir, args.file_regex, args.target_dir)
