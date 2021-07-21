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

from commit_validation.commit_validation import Commit, validate_commit
from p4 import run_p4_command


class P4SubmittedChangelists(Commit):
    """An implementation of the :class:`Commit` interface for accessing details about Perforce submitted changelists"""

    def __init__(self, from_change: str, to_change: str, client: str = None) -> None:
        """Creates a new instance of :class:`P4SubmittedChangelists`

        :param from_change: The oldest Perforce changelist to include
        :param to_change: The newest Perforce changelist to include
        :param client: The Perforce client
        """
        self.from_change = from_change
        self.to_change = to_change
        self.client = client
        self.files: List[str] = []
        self.removed_files: List[str] = []
        self.file_diffs: Dict[str, str] = {}
        self._load_files()

    def _load_files(self):
        self.files = []
        self.removed_files = []
        depot_root = None

        client_root = run_p4_command(f'client -o', client=self.client)[0]['Root']
        files = run_p4_command(f'files {client_root}{os.sep}...@{self.from_change},{self.to_change}',
                               client=self.client)

        for f in files:
            # The following code optimizes for the specific use case where all files are mapped the same way on the 
            # client. The reason for this is to avoid calling 'p4 where' on thousands of files. So instead, it only calls
            # 'p4 where' on the first file and applies the same mapping to the rest of the files.
            if depot_root is None:
                file_path = run_p4_command(f'where {f["depotFile"]}', client=self.client)[0]['path']
                relative_path = file_path.replace(client_root, '')
                relative_path = relative_path.replace('\\', '/')
                depot_root = f['depotFile'].replace(relative_path, '')
            else:
                file_path = os.path.abspath(f['depotFile'].replace(depot_root, client_root))

            if 'delete' in f['action']:
                self.removed_files.append(file_path)
            else:
                self.files.append(file_path)
        pass

    def get_file_diff(self, file) -> str:
        if file in self.file_diffs:
            return self.file_diffs[file]
        
        if file not in self.files:
            raise RuntimeError(f"Cannot compute diff for file not in change set: {file}")

        # the diff2 command does not behave like normal diff command, in that it only
        # returns the actual diff if you do not use '-G' mode.  So we ask it for the raw output:
        diff = run_p4_command(f'diff2 -du {file}@{self.from_change} {file}@{self.to_change}', client=self.client, raw_output=True)
        self.file_diffs[file] = diff
        # note that if you feed the same changelist as the 'before' and 'after' on the command line
        # you will get no diffs, because there is no difference between a changelist and itself.
        # In addition, if you are diffing across changelists, but the file only existed
        # in one changelist, it will also not indicate a diff (due to how the p4 diff2 command operates)
        # This means that this validator is blind to branch integrates.  This is not the case
        # for the 'live' or 'shelved' validator as those files show up as additions.
        return diff

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
    parser.add_argument('from_change', help='The oldest changelist to include')
    parser.add_argument('to_change', help='The newest changelist to include')
    parser.add_argument('--client', help='The Perforce client')
    return parser


def main():
    parser = init_parser()
    args = parser.parse_args()

    change = P4SubmittedChangelists(client=args.client, from_change=args.from_change, to_change=args.to_change)

    if not validate_commit(commit=change, ignore_validators=["NewlineValidator", "WhitespaceValidator"]):
        sys.exit(1)

    sys.exit(0)


if __name__ == '__main__':
    main()
