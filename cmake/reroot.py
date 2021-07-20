#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import subprocess
import argparse
import sys

restricted_platforms = ['Provo', 'Salem']

def move_root(current_source_dir, csd_to_platform_parent, platform_dir, ly_dir):
    cwd = os.getcwd()
    if not ly_dir:
        ly_dir = cwd
    ly_to_csd = os.path.relpath(os.path.join(cwd, current_source_dir), ly_dir)
    platform_parent_dir = os.path.normpath(os.path.join(current_source_dir, csd_to_platform_parent, platform_dir))
    for p in os.listdir(platform_parent_dir):
        if p in restricted_platforms:
            source_dir = os.path.normpath(os.path.join(platform_parent_dir, p))
            dest_dir = os.path.normpath(os.path.join(ly_dir, 'restricted', p, ly_to_csd, csd_to_platform_parent))
            for root, _, files in os.walk(source_dir):
                for f in files:
                    old = os.path.join(root, f)
                    source_to_old = os.path.relpath(old, source_dir)
                    new = os.path.join(dest_dir, source_to_old)
                    subprocess.run(['p4', 'move', '-r', old, new])

parser = argparse.ArgumentParser(description='Re-root restricted platform files')
parser.add_argument('source_path', type=str, help='Relative path to the source tree of restricted files to move')
parser.add_argument('--pal', type=str, default='Platform', help='Name of folder that contains the pal-ified folders (defaults to "Platform")')
parser.add_argument('--path-to-pal', type=str, default='', help='Additional path between source path and the pal folder (defaults to no additional path)')
parser.add_argument('--out-dir', type=str, help='root directory that holds the "restricted" folder. Defaults to current working directory')
args = parser.parse_args()

move_root(args.source_path, args.path_to_pal, args.pal, args.out_dir)