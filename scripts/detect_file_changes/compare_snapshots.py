#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import pickle
import sys

"""This is a command-line entry point which, when given two snapshots created using make_snapshot.py
compares them and reports on the differences.  It returns 0 if and only if there are no differences.

To use it in a scripting environment instead of a CLI, invoke do_compare(filename1, filename2)
Or do it manually using FolderSnapshot.CompareSnapshots and then enumerate_changes
"""

ignore_patterns = []

from snapshot_folder.snapshot_folder import FolderSnapshot, SnapshotComparison

def do_compare(filename1, filename2):
    """Given two filenames, returns the diffs as a list of tuples [(type, file)]"""
    snap1 = pickle.load(open(filename1, 'rb'))
    snap2 = pickle.load(open(filename2, 'rb'))

    comparison = FolderSnapshot.CompareSnapshots(snap1, snap2)

    changes = [change_entry for change_entry in comparison.enumerate_changes()]
    return changes

def init_parser():
    """Prepares the command line parser"""
    parser = argparse.ArgumentParser()
    parser.description = "Compares two snapshots previously saved, outputs the changes.  Exit code is 0 if no diff, 1 otherwise."
    parser.add_argument('first', default='.', help='first file to use')
    parser.add_argument('second', default='.', help='second file to use')
    return parser

def main():
    parser = init_parser()
    args = parser.parse_args()
    changes = do_compare(args.first, args.second)
    
    for change in changes:
        print(f"Change detected: {change}")
    
    if changes:
        return 1
    
    return 0

if __name__ == '__main__':
    sys.exit(main())