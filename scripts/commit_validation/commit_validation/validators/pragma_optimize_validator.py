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
import os.path
from typing import Type, List

from commit_validation.commit_validation import Commit, CommitValidator, SOURCE_FILE_EXTENSIONS, EXCLUDED_VALIDATION_PATTERNS, VERBOSE


class PragmaOptimizeValidator(CommitValidator):
    """A file-level validator that makes sure a file does not contain #pragma optimize directives"""

    def run(self, commit: Commit, errors: List[str]) -> bool:
        for file_name in commit.get_files():
            for pattern in EXCLUDED_VALIDATION_PATTERNS:
                if fnmatch.fnmatch(file_name, pattern):
                    if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED - Validation pattern excluded on path.')
                    break
            else:
                if os.path.splitext(file_name)[1].lower() not in SOURCE_FILE_EXTENSIONS:
                    if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED - File excluded based on extension.')
                    continue

                file_diff = commit.get_file_diff(file_name)
                previous_line_for_context = ""
                for line in file_diff.splitlines():
                    # we only care about added lines in a diff
                    if line.startswith('+'):
                        if '#pragma optimize' in line:
                            error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - Source file contains #pragma optimize!\n'
                                                f'     {previous_line_for_context}\n'
                                                f'---> {line}')
                            if VERBOSE: print(error_message)
                            errors.append(error_message)
                        previous_line_for_context = line

        return (not errors)


def get_validator() -> Type[PragmaOptimizeValidator]:
    """Returns the validator class for this module"""
    return PragmaOptimizeValidator
