#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import fnmatch
import pathlib

"""This module contains FolderSnapshot, a class which can create and compare 'snapshots'
of folders (The snapshots just store the modtimes / existence of files and folders), and
also can compare two snapshots to return a SnapshotComparison which represents the diffs
"""

class SnapshotComparison:
    """ This class just holds the diffs calculated between two folder trees."""
    def __init__(self):
        self.deleted_files = []
        self.added_files = []
        self.changed_files = []
        self.dirs_added = []
        self.dirs_removed = []
    
    def any_changed(self):
        """Returns True if any changes were detected"""
        return self.deleted_files or self.added_files or self.changed_files or self.dirs_added or self.dirs_removed

    def enumerate_changes(self):
        """Enumerates changes, yielding each as a pair of (string, string), that is, (type of change, filename)"""
        for file_entry in self.deleted_files:
            yield ("DELETED", file_entry)
        for file_entry in self.added_files:
            yield ("ADDED", file_entry)
        for file_entry in self.changed_files:
            yield ("CHANGED", file_entry)
        for dir_entry in self.dirs_added:
            yield ("FOLDER_ADDED", dir_entry)
        for dir_entry in self.dirs_removed:
            yield ("FOLDER_DELETED", dir_entry)

class FolderSnapshot:
    """ This class stores a snapshot of a folder state and has utility functions to compare snapshots"""
    def __init__(self):
        self.file_modtimes = {}
        self.folder_paths = []
        pass

    @staticmethod
    def _matches_ignore_pattern(in_string, ignore_patterns):
        for pattern in ignore_patterns:
            if fnmatch.fnmatch(in_string, pattern):
                return True
            # we also care if the last part is in the patterns
            # this is to cover situatiosn where the pattern has 'build' in it as opposed to *build*
            # and the name of the file is literally 'build'
            _, filepart = os.path.split(in_string)
            if fnmatch.fnmatch(filepart, pattern):
                return True

        return False
    
    @staticmethod
    def CreateSnapshot(root_folder, ignore_patterns):
        """Create a new FolderSnapshot based on a root folder and ignore patterns."""
        folder_snap = FolderSnapshot()
        ignored_folders = []
        for root, dir_names, file_names in os.walk(root_folder, followlinks=False):
            for dir_name in dir_names:
                fullpath = os.path.normpath(os.path.join(root, dir_name)).replace('\\', '/')
                if FolderSnapshot._matches_ignore_pattern(fullpath, ignore_patterns):
                    ignored_folders.append(fullpath)
                    continue
                if os.path.dirname(fullpath) in ignored_folders:
                    # we want to emulate not 'walking' down any folders themselves that have been omitted:
                    ignored_folders.append(fullpath)
                    continue
                folder_snap.folder_paths.append(fullpath)
            
            for file_name in file_names:
                fullpath = os.path.normpath(os.path.join(root, file_name)).replace('\\', '/')
                if FolderSnapshot._matches_ignore_pattern(fullpath, ignore_patterns):
                    continue
                if os.path.dirname(fullpath) in ignored_folders:
                    # we want to emulate not 'walking' down any folders themselves that have been omitted:
                    continue
                folder_snap.file_modtimes[fullpath] = os.stat(fullpath).st_mtime
        
        return folder_snap        

    @staticmethod
    def CompareSnapshots(before, after):
        """Return a SnapshotComparison representing the difference between two FolderShapshot objects"""
        comparison = SnapshotComparison()
        for file_name in before.file_modtimes.keys():
            if file_name not in after.file_modtimes:
                comparison.deleted_files.append(file_name)
            else:
                if before.file_modtimes[file_name] != after.file_modtimes[file_name]:
                    comparison.changed_files.append(file_name)
        
        for file_name in after.file_modtimes.keys():
            if file_name not in before.file_modtimes:
                comparison.added_files.append(file_name)

        for folder_path in before.folder_paths:
            if folder_path not in after.folder_paths:
                comparison.dirs_removed.append(folder_path)
        
        for folder_path in after.folder_paths:
            if folder_path not in before.folder_paths:
                comparison.dirs_added.append(folder_path)
        return comparison
