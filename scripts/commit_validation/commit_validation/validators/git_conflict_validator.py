#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import re
from typing import Type, List

import commit_validation.pal_allowedlist as pal_allowedlist
from commit_validation.commit_validation import Commit, CommitValidator, SOURCE_FILE_EXTENSIONS, VERBOSE

MERGE_TO_MARKER_REGEX = re.compile(r'^<<<<<<<')
MERGE_BASE_REGEX = re.compile(r'^\|\|\|\|\|\|\|')
MERGE_DIFF_REGEX = re.compile(r'^=======\n')  # include newline as equals sign is a common visual separator
MERGE_FROM_MARKER_REGEX = re.compile(r'^>>>>>>>')


class GitConflictValidator(CommitValidator):
    """A file-level validator that for conflict markers"""

    def __init__(self) -> None:
        self.pal_allowedlist = pal_allowedlist.load()

    def run(self, commit: Commit, errors: List[str]) -> bool:
        for file_name in commit.get_files():
            if os.path.splitext(file_name)[1].lower() not in SOURCE_FILE_EXTENSIONS:
                if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED - File excluded based on extension.')
                continue
            if self.pal_allowedlist.is_match(file_name):
                if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED - File excluded based on PAL allowedlist.')
                continue

            # we never want conflict markers to be added to our repository
            # so we don't look at the file diffs, but the file contents.
            with open(file_name, 'rt', encoding='utf8', errors='replace') as fh:
                previous_line_context = ""
                for line_number, line in enumerate(fh):
                    if MERGE_TO_MARKER_REGEX.search(line):
                        error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - Source file contains git merge '
                              f'conflict start-marker on line {line_number + 1}:\n'
                              f'      {previous_line_context}'
                              f'----> {line}')
                        if VERBOSE: print(error_message)
                        errors.append(error_message)
                    if MERGE_BASE_REGEX.search(line):
                        error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - Source file contains git merge '
                              f'conflict diff3-marker on line {line_number + 1}:\n'
                              f'      {previous_line_context}'
                              f'----> {line}')
                        if VERBOSE: print(error_message)
                        errors.append(error_message)
                    if MERGE_DIFF_REGEX.search(line):
                        error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - Source file contains git merge '
                              f'conflict diff-marker on line {line_number + 1}:\n'
                              f'      {previous_line_context}'
                              f'----> {line}')
                        if VERBOSE: print(error_message)
                        errors.append(error_message)
                    if MERGE_FROM_MARKER_REGEX.search(line):
                        error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - Source file contains git merge '
                              f'conflict end-marker on line {line_number + 1}:\n'
                              f'      {previous_line_context}'
                              f'----> {line}')
                        if VERBOSE: print(error_message)
                        errors.append(error_message)
                    previous_line_context = line

        return (not errors)


def get_validator() -> Type[GitConflictValidator]:
    """Returns the validator class for this module"""
    return GitConflictValidator
