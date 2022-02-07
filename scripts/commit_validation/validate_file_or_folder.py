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
import sys
from typing import Dict, List
import difflib

from commit_validation.commit_validation import Commit, validate_commit, IsFileSkipped, SOURCE_AND_SCRIPT_FILE_EXTENSIONS, EXCLUDED_VALIDATION_PATTERNS

class FolderBased(Commit):
    """An implementation of the :class:`Commit` which populates it from files and folders on disk"""

    def __init__(self, item_path: str = '.') -> None:
        """Creates a new instance of :class:`FolderBased`
        :param item_path - The file name or the folder to search (recursively)
        """
        self.item_path = os.path.abspath(item_path)
        self.files: List[str] = []
        self.removed_files: List[str] = []
        self.file_diffs: Dict[str, str] = {}
        self._load_files()

    def _load_files(self):
        self.files = []
        self.removed_files = []
        # if its a file, we just add that file.
        # if its a folder, we add it recursively
        if os.path.isfile(self.item_path):
            self.files.append(self.item_path)
        else:
            engine_root_path = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
            for root, _, file_names in os.walk(self.item_path):
                for file_name in file_names:
                    file_to_scan = os.path.abspath(os.path.join(root, file_name))
                    skip = False
                    for excluded_folder in EXCLUDED_VALIDATION_PATTERNS:
                        matcher = os.path.abspath(os.path.join(engine_root_path, excluded_folder, '*'))
                        if fnmatch.fnmatch(file_to_scan, matcher):
                            skip = True
                            break
                    if not skip:
                        self.files.append(file_to_scan)

    def get_file_diff(self, file) -> str:
        if file in self.file_diffs:
            return self.file_diffs[file]
        # we count the entire file as changed in this mode
        # we do not include diffs for things that are not source files
        # this also prevents us trying to decode binary files as utf8
        if IsFileSkipped(file):
            return ""
        
        diff_being_built = ""
        # We simulate every line having been changed by making a blank file
        # being the 'from' and the whole file being the 'to' part of the diff
        with open(file, "rt", encoding='utf8', errors='replace') as opened_file:
            data = opened_file.readlines()
            diffs = difflib.unified_diff([], data, fromfile=file, tofile=file)
            diff_being_built = ''.join(diffs)
    
        self.file_diffs[file] = diff_being_built
        return diff_being_built

    def get_files(self) -> List[str]:
        return self.files

    def get_removed_files(self) -> List[str]:
        return self.removed_files

    def get_description(self) -> str:
        raise NotImplementedError

    def get_author(self) -> str:
        raise NotImplementedError


def init_parser():
    """Prepares the command line parser"""
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', default='.', help='Filename of single file, or a folder path to scan recursively')
    return parser


def main():
    parser = init_parser()
    args = parser.parse_args()

    change = FolderBased(item_path = args.path)
    validators_to_ignore = [
                "NewlineValidator", 
                "WhitespaceValidator"
                ]
                
    if not validate_commit(commit=change, ignore_validators = validators_to_ignore):
        sys.exit(1)

    sys.exit(0)

if __name__ == '__main__':
    main()
