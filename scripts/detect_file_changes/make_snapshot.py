#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import pickle
import sys

"""This is a command line entry point that, given an out file name, and a folder to scan
generates a file which contains the current snapshot of the files and folders in that folder.
It is to be used later by compare_snapshot.py

If you want to use this in a scripting environment instead of a CLI, use the dump_snapshot 
function or use FolderSnapshot.CreateSnapshot directly.
"""

default_ignore_patterns = ['*.pyc', '__pycache__', '*.snapshot', 'Cache', 'build_*', 'build' ]

from snapshot_folder.snapshot_folder import FolderSnapshot, SnapshotComparison

def dump_snapshot(folder_to_scan, filename, ignore_patterns):
    """Workhorse function of this module.  Saves the snapshot to the given file"""
    snap = FolderSnapshot.CreateSnapshot(folder_to_scan, ignore_patterns=ignore_patterns)
    pickle.dump(snap, open(filename, 'wb'))

def init_parser():
    """Prepares the command line parser"""
    parser = argparse.ArgumentParser()
    parser.description = "Takes a snapshot of the current files and folders into a given file for later comparison"
    parser.add_argument('path_to_check', default='.', help='Path To start iterating at')
    parser.add_argument('--out', required=True, help='Path to output to')
    parser.add_argument('--ignore', action='append', nargs='+', default=default_ignore_patterns)
    return parser

def main():
    """ Entry point to use if you want to supply args on the command line"""
    parser = init_parser()
    args = parser.parse_args()
    print(f"Snapshotting: {args.path_to_check} into {args.out} with ignore {args.ignore}")
    return dump_snapshot(args.path_to_check, args.out, args.ignore)

if __name__ == '__main__':
    main()