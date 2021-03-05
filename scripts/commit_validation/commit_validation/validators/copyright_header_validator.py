#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import fnmatch
import os
import re
from typing import Type, List

from commit_validation.commit_validation import Commit, CommitValidator, IsFileSkipped, SOURCE_AND_SCRIPT_FILE_EXTENSIONS, EXCLUDED_VALIDATION_PATTERNS, VERBOSE


class CopyrightHeaderValidator(CommitValidator):
    """A file-level validator that makes sure a file contains the standard copyright header"""

    def run(self, commit: Commit, errors: List[str]) -> bool:
        for file_name in commit.get_files():
            for pattern in EXCLUDED_VALIDATION_PATTERNS:
                if fnmatch.fnmatch(file_name, pattern):
                    if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED - Validation pattern excluded on path.')
                    break
            else:
                if IsFileSkipped(file_name):
                    if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED - File excluded based on extension.')
                    continue

                copyright_regex = re.compile(r'copyright[\s]*(?:\(c\))?[\s]*amazon\.com', re.IGNORECASE)

                # copyright header validator does not use the diff, as it needs to check the front
                # of the file for the header.
                with open(file_name, 'rt', encoding='utf8', errors='replace') as fh:
                    for line in fh:
                        if copyright_regex.search(line):
                            break
                    else:
                        error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - Source file missing copyright headers.')
                        errors.append(error_message)
                        if VERBOSE: print(error_message)

        return (not errors)


def get_validator() -> Type[CopyrightHeaderValidator]:
    """Returns the validator class for this module"""
    return CopyrightHeaderValidator
