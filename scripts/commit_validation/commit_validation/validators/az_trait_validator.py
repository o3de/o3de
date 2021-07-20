#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import re
from typing import Type, List

import commit_validation.pal_allowedlist as pal_allowedlist
from commit_validation.commit_validation import Commit, CommitValidator, SOURCE_FILE_EXTENSIONS, VERBOSE

ifdef_regex = re.compile(r'^\+\s*#\s*ifn?def\s+AZ_TRAIT_')
defined_regex = re.compile(r'\sdefined\s*\(\s*AZ_TRAIT_')


class AzTraitValidator(CommitValidator):
    """A file-level validator that makes sure a file does not contain existence checks for AZ_TRAIT macros"""

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

            file_diff = commit.get_file_diff(file_name)
            previous_line_context = ""

            for line in file_diff.splitlines():
                # we only care about added lines.
                if line.startswith('+'):
                    if ifdef_regex.search(line) or defined_regex.search(line):
                        error_message = str(
                                f'{file_name}::{self.__class__.__name__} FAILED - Source file contains an existence '
                                f'check for an AZ_TRAIT macro in this code: \n'
                                f'       {previous_line_context}\n'
                                f' ----> {line}\n'
                                f'Traits should be tested for true/false, since they are guaranteed to exist on all platforms.')
                        if VERBOSE: print(error_message)
                        errors.append(error_message)
                previous_line_context = line

        return (not errors)


def get_validator() -> Type[AzTraitValidator]:
    """Returns the validator class for this module"""
    return AzTraitValidator
