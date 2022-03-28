#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import binascii
import fnmatch
import pathlib
import re
from typing import Type, List

from commit_validation.commit_validation import Commit, CommitValidator, SOURCE_FILE_EXTENSIONS, EXCLUDED_VALIDATION_PATTERNS, VERBOSE

class CrcValidator(CommitValidator):
    """A file-level validator that makes sure a file does not contain an invalid CRC"""

    def run(self, commit: Commit, errors: List[str]) -> bool:
        for file_name in commit.get_files():
            for pattern in EXCLUDED_VALIDATION_PATTERNS:
                if fnmatch.fnmatch(file_name, pattern):
                    if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED - Validation pattern excluded on path.')
                    break
            else:
                if pathlib.Path(file_name).suffix.lower() not in SOURCE_FILE_EXTENSIONS:
                    if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED - File excluded based on extension.')
                    continue
                
                with open(file_name, mode='r', encoding='utf8') as fh:
                    fileContents = fh.read()
                    matchesFound = re.findall(r'AZ_CRC\("([^"]+)",([^)]*)\)', fileContents)
                    for element in matchesFound:
                        stringInCode = element[0]
                        valueInCode = element[1].strip()
                        expectedValue = "{0:#0{1}x}".format(binascii.crc32(stringInCode.lower().encode('utf8')), 10)
                        if expectedValue != valueInCode:
                            error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - Source file contains a CRC mismatch!\n'
                                                f'     AZ_CRC("{stringInCode}", {valueInCode}), expected value {expectedValue}')
                            if VERBOSE: print(error_message)
                            errors.append(error_message)
        return (not errors)


def get_validator() -> Type[CrcValidator]:
    """Returns the validator class for this module"""
    return CrcValidator
