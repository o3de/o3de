#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import fnmatch
import os
import re
from typing import Type, List

from commit_validation.commit_validation import Commit, CommitValidator, IsFileSkipped, SOURCE_AND_SCRIPT_FILE_EXTENSIONS, EXCLUDED_VALIDATION_PATTERNS, VERBOSE

__STARTS_WITH_UNCHANGED_LINE = r'(\A|^(?=[^+-]).*\n)'
__NONZERO_CHANGED_WHITESPACE_LINES = r'(^[+-][\r\t\f\v ]*\n)+'
__ENDS_WITH_UNCHANGED_LINE = r'([^+-]|\Z)'
__NONADJACENT_WHITESPACE =\
    __STARTS_WITH_UNCHANGED_LINE + __NONZERO_CHANGED_WHITESPACE_LINES + __ENDS_WITH_UNCHANGED_LINE
_NONADJACENT_WHITESPACE_DIFF_REGEX = re.compile(__NONADJACENT_WHITESPACE, re.MULTILINE)

_NONWHITESPACE_DIFF_REGEX = re.compile(r'^[+-][\t ]*\S', re.MULTILINE)


class WhitespaceValidator(CommitValidator):
    """A diff-level validator that makes sure a change does not mix whitespace-only changes with code changes"""

    def run(self, commit: Commit, errors: List[str]) -> bool:
        for file_name in commit.get_files():
            file_identifier = f"{file_name}::{self.__class__.__name__}"
            for pattern in EXCLUDED_VALIDATION_PATTERNS:
                if fnmatch.fnmatch(file_name, pattern):
                    if VERBOSE: print(f'{file_identifier} SKIPPED - Validation pattern excluded on path.')
                    break
            else:
                if IsFileSkipped(file_name):
                    if VERBOSE: print(f'{file_identifier} SKIPPED - File excluded based on extension.')
                    continue

                diff = commit.get_file_diff(file_name)
                
                if _NONADJACENT_WHITESPACE_DIFF_REGEX.search(diff) and _NONWHITESPACE_DIFF_REGEX.search(diff):
                    error_message = str(f'{file_identifier} FAILED - Source file contains whitespace-only changes which are '
                          f'non-contiguous with other non-whitespace changes.  Make whitespace-only changes as a '
                          f'separate change.')
                    errors.append(error_message)
                    if VERBOSE: print(error_message)

        return (not errors)


def get_validator() -> Type[WhitespaceValidator]:
    """Returns the validator class for this module"""
    return WhitespaceValidator
