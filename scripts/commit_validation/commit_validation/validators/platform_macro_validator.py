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

import commit_validation.pal_allowedlist as pal_allowedlist
from commit_validation.commit_validation import Commit, CommitValidator, SOURCE_FILE_EXTENSIONS, VERBOSE

platform_macros = [
    'ANDROID',
    'APPLE',
    'APPLETV',
    'DARWIN',
    'IOS',
    'LINUX',
    'LINUX64',
    'MAC',
    'PROVO',
    'SALEM',
    'WIN32',
    'WIN32_LEAN_AND_MEAN',
    'WIN64',
    '_WIN32',
    '_WIN32_WINDOWS',
    '_WIN32_WINNT',
    'linux',
]

platform_macro_regex = re.compile(r'^\+\s*#.*?\b(' + str.join('|', platform_macros) + r')\b')


class PlatformMacroValidator(CommitValidator):
    """A file-level validator that makes sure a file does not contain certain platform macros"""

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
            if fnmatch.fnmatch(file_name, '*/Platform/*'):
                if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED - File excluded because it is in a "Platform" '
                      f'folder.')
                continue

            file_diff = commit.get_file_diff(file_name)
            previous_line_context = ""

            line_number = 1
            for line in file_diff.splitlines():
                # we only care about added lines.
                if line.startswith('+'):
                    match = platform_macro_regex.search(line)
                    if match:
                        error_message = str(f'{file_name}:{line_number}::{self.__class__.__name__} FAILED - Source file contains a forbidden '
                                f'platform macro "{match.group(1)}" here:\n'
                                f'     {previous_line_context}\n'
                                f'---> {line}')
                        if VERBOSE: print(error_message)
                        errors.append(error_message)
                previous_line_context = line
                line_number += 1

        return (not errors)


def get_validator() -> Type[PlatformMacroValidator]:
    """Returns the validator class for this module"""
    return PlatformMacroValidator
