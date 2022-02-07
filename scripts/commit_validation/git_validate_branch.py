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
from typing import Dict, List
import difflib

from commit_validation.commit_validation import Commit, validate_commit
import git


class GitChange(Commit):
    """An implementation of the :class:`Commit` interface for accessing details about a git change"""

    def __init__(self, source: str = None, target: str = 'origin/main') -> None:
        """Creates a new instance of :class:`GitChange`

        :param source: source of the change, e.g. commit hash or branch
        :param target: target of the change, e.g. main branch
        """
        root_path = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
        self.repo = git.Repo()
        self.git_root_path = self.repo.git.rev_parse("--show-toplevel")
        if source:
            git.cmd.Git().fetch('origin', source) # So we can compare it
            self.source_commit = self.repo.commit(target)
        else:
            self.source_commit = self.repo.commit()

        if 'origin' in target:
            origin_target = target.replace('origin/','')
            git.cmd.Git().fetch('origin', origin_target) # So we can compare it
        self.target_commit = self.repo.commit(target)       

        # We only want to run the verification on the changes introduced by the
        # branch being merged in. Find the merge base (the common ancestor) of
        # the two commits, and then get the diff from merge_base..target_commit
        self.merge_base = self.repo.merge_base(self.source_commit, self.target_commit)
        if not len(self.merge_base) == 1:
            raise RuntimeError(f"Cannot find the merge base of {self.source_commit} and {self.target_commit}")
        self.merge_base = self.merge_base[0]
        self.diff_index = self.merge_base.diff(self.source_commit)

        print(f"Running validation from '{source}' ({self.source_commit}) to '{target}' ({self.target_commit}) using baseline {self.merge_base}")

        # Cache the file lists since they are requested by each validator
        self.files_list: List[str] = []
        self.removed_files_list: List[str] = []

        for diff_item in self.diff_index:
            # 'A' for added paths
            # 'C' for changed paths
            # 'R' for renamed paths
            # 'M' for paths with modified data
            # 'T' for changed in the type paths
            # 'D' for deleted paths
            # 'R' for renamed paths
            if diff_item.change_type in ('A', 'C', 'R', 'M', 'T'):
                self.files_list.append(os.path.abspath(os.path.join(self.git_root_path, diff_item.b_path)))
            if diff_item.change_type in ('D', 'R'):
                self.removed_files_list.append(os.path.abspath(os.path.join(self.git_root_path, diff_item.a_path))) 

    def get_files(self) -> List[str]:
        """Returns a list of local files added/modified by the commit"""
        return self.files_list

    def get_removed_files(self) -> List[str]:
        """Returns a list of local files removed by the commit"""
        return self.removed_files_list

    def get_file_diff(self, str) -> str:
        """
        Given a file name, returns a string in unified diff format
        that represents the changes made to that file for this commit.
        Most validators will only pay attention to added lines (with + in front)
        """
        diff = self.repo.git.diff(self.merge_base, self.source_commit, str)
        return diff

    def get_description(self) -> str:
        """Returns the description of the commit"""
        return self.target_commit.message

    def get_author(self) -> str:
        """Returns the author of the commit"""
        return self.target_commit.author

def init_parser():
    """Prepares the command line parser"""
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', default=None, help='Change source (e.g. commit hash or branch), defaults to active branch')
    parser.add_argument('--target', default='origin/main', help='Change target, defaults to "origin/main"')
    return parser

def main():
    parser = init_parser()
    args = parser.parse_args()

    change = GitChange(source=args.source, target=args.target)

    if not validate_commit(commit=change, ignore_validators=["NewlineValidator", "WhitespaceValidator"]):
        sys.exit(1)

    sys.exit(0)

if __name__ == '__main__':
    main()
