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

from commit_validation.commit_validation import Commit, CommitValidator, IsFileSkipped, SOURCE_AND_SCRIPT_FILE_EXTENSIONS, EXCLUDED_VALIDATION_PATTERNS, VERBOSE

allowed_chars = {  
    0xAD, # '_'
    0xAE, # '®'
    0xB0, # '°'
}
class UnicodeValidator(CommitValidator):
    """A file-level validator that makes sure a file does not contain unicode characters"""

    def run(self, commit: Commit, errors: List[str]) -> bool:
        for file_name in commit.get_files():
            if IsFileSkipped(file_name):
                if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED UnicodeValidator - File excluded based on extension.')
                continue

            for pattern in EXCLUDED_VALIDATION_PATTERNS:
                if fnmatch.fnmatch(file_name, pattern):
                    if VERBOSE: print(f'{file_name} SKIPPED UnicodeValidator - Validation pattern excluded on path.')
                    break
            else:
                with open(file_name, 'r', encoding='utf-8', errors='strict') as fh:
                    linecount = 1
                    for line in fh:
                        columncount = 0
                        for ch in line:
                            ord_ch = ord(ch)
                            if ord_ch > 127 and ord_ch not in allowed_chars:
                                error_message = str(f'{file_name}::{self.__class__.__name__}:{linecount},{columncount} FAILED - Source file contains unicode character, replace with \\u{ord_ch:X}.')
                                errors.append(error_message)
                                if VERBOSE: print(error_message)
                            columncount += 1
                        linecount += 1

        return (not errors)

def get_validator() -> Type[UnicodeValidator]:
    """Returns the validator class for this module"""
    return UnicodeValidator
