#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import os
import sys
from typing import Dict, List
import difflib

from commit_validation.commit_validation import Commit, validate_commit
from p4 import run_p4_command


class P4Changelist(Commit):
    """An implementation of the :class:`Commit` interface for accessing details about a Perforce changelist"""

    def __init__(self, client: str = None, change: str = 'default') -> None:
        """Creates a new instance of :class:`P4Changelist`

        :param client: the Perforce client
        :param change: the Perforce changelist
        """
        self.client = client
        self.change = change
        self.files: List[str] = []
        self.removed_files: List[str] = []
        self.file_diffs: Dict[str, str] = {}
        self._load_files()

    def _load_files(self):
        self.files = []
        self.removed_files = []
        self.added_files = []

        client_spec = run_p4_command(f'client -o', client=self.client)[0]
        files = run_p4_command(f'opened -c {self.change}', client=self.client)
        for f in files:
            file_path = os.path.abspath(f['clientFile'].replace(f'//{client_spec["Client"]}', client_spec['Root']))
            if 'delete' in f['action']:
                self.removed_files.append(file_path)
            elif 'add' in f['action']:
                self.added_files.append(file_path)
            elif 'branch' in f['action']:
                self.added_files.append(file_path)
            else:
                self.files.append(file_path)

    def get_file_diff(self, file) -> str:
        if file in self.file_diffs:  # allow caching
            return self.file_diffs[file]

        if file in self.added_files: # added files return the entire file as a diff.
            with open(file, "rt", encoding='utf8', errors='replace') as opened_file:
                data = opened_file.readlines()
                diffs = difflib.unified_diff([], data, fromfile=file, tofile=file)
                diff_being_built = ''.join(diffs)
                self.file_diffs[file] = diff_being_built
                return diff_being_built
            
        if file not in self.files:
            raise RuntimeError(f"Cannot calculate a diff for a file not in the changelist:  {file}")
        
        try:
            result = run_p4_command(f'diff -du {file}', client=self.client)
            if len(result) > 1:
                diff = result[1]['data'] # p4 returns a normal code but with no result if theres no diff
            else:
                diff = ''
                print(f'Warning: File being committed contains no changes {file}')
            # note that the p4 command handles the data and errors internally, no need to check.
            self.file_diffs[file] = diff
            return diff
        except RuntimeError as e:
            print(f'error during p4 operation, unable to get a diff: {e}')
        
        return ''

    def get_files(self) -> List[str]:
        # this is just files relevant to the operation
        return self.files + self.added_files

    def get_removed_files(self) -> List[str]:
        return self.removed_files

    def get_description(self) -> str:
        raise NotImplementedError

    def get_author(self) -> str:
        raise NotImplementedError


def init_parser():
    """Prepares the command line parser"""
    parser = argparse.ArgumentParser()
    parser.add_argument('--client', help='Perforce client')
    parser.add_argument('--change', default='default', help='Perforce changelist')
    return parser


def main():
    parser = init_parser()
    args = parser.parse_args()

    change = P4Changelist(client=args.client, change=args.change)

    if not validate_commit(commit=change, ignore_validators=["NewlineValidator", "WhitespaceValidator"]):
        sys.exit(1)

    sys.exit(0)


if __name__ == '__main__':
    main()
