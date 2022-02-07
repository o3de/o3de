#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import fnmatch
import os.path
from typing import Type, List

from commit_validation.commit_validation import Commit, CommitValidator, IsFileSkipped, SOURCE_AND_SCRIPT_FILE_EXTENSIONS, EXCLUDED_VALIDATION_PATTERNS, VERBOSE

class TabsValidator(CommitValidator):
    """A file-level validator that makes sure a file does not contain tabs"""

    def run(self, commit: Commit, errors: List[str]) -> bool:
        for file_name in commit.get_files():
            if IsFileSkipped(file_name):
                if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED TabsValidator - File excluded based on extension.')
                continue

            for pattern in EXCLUDED_VALIDATION_PATTERNS:
                if fnmatch.fnmatch(file_name, pattern):
                    if VERBOSE: print(f'{file_name} SKIPPED TabsValidator - Validation pattern excluded on path.')
                    break
            else:
                tab_line_count = 0
                file_diff = commit.get_file_diff(file_name)

                # Usually, code either has a very small number of tabs in the file by accident,
                # or the entire file is full of tabs.  
                # So we count the tabs, but we only print the first one in full.
                first_tab_line_found = None
                  
                for line in file_diff.splitlines():
                    # we only care about added lines.
                    if line.startswith('+'):
                        if '\t' in line:
                            line = line.replace('\t','\\t') # make it obvious!
                            if not first_tab_line_found:
                                first_tab_line_found = str(
                                    f'     {previous_line_context}\n'
                                    f'---> {line}\n')
                            tab_line_count = tab_line_count + 1
                            
                    previous_line_context = line
                if tab_line_count:
                    error_message = str(
                                f'{file_name}::{self.__class__.__name__} FAILED TabsValidator - {tab_line_count} tabs in this file\n'
                                f'First instance of a tab: \n'
                                f'{first_tab_line_found}')
                    errors.append(error_message)
                    if VERBOSE: print(error_message)

        return (not errors)


def get_validator() -> Type[TabsValidator]:
    """Returns the validator class for this module"""
    return TabsValidator
