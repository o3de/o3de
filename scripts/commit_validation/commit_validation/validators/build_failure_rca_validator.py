#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import fnmatch
import json
import re
from typing import Type, List

from commit_validation.commit_validation import Commit, CommitValidator, EXCLUDED_VALIDATION_PATTERNS, VERBOSE

RCA_PATTERN_PATH = '*/scripts/build/build_failure_rca/rca_patterns/*'

class BuildFailureRCAValidator(CommitValidator):
    """A file-level validator that makes sure build failure RCA patterns can catch all the test cases"""

    def run(self, commit: Commit, errors: List[str]) -> bool:
        for file_name in commit.get_files():
            for pattern in EXCLUDED_VALIDATION_PATTERNS:
                if fnmatch.fnmatch(file_name, pattern):
                    if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED - Validation pattern excluded on path.')
                    break
            else:
                if not fnmatch.fnmatch(file_name, RCA_PATTERN_PATH):
                    if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED - Validation pattern excluded on path.')
                    continue
                
                with open(file_name, 'r', encoding='utf8') as fh:
                    try:
                        data = json.load(fh)
                    except json.decoder.JSONDecodeError:
                        error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - JSON format error.\n')
                        if VERBOSE: print(error_message)
                        errors.append(error_message)
                        continue
                    for item in data['indications']:
                        test_cases = item['log_testcases']
                        if not test_cases:
                            error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - No test cases found ({item["name"]}):\n')
                            if VERBOSE: print(error_message)
                            errors.append(error_message)
                            continue
                        for test_case in test_cases:
                            for pattern in item['log_patterns']:
                                if item['single_line']:
                                    regex = re.compile(pattern)
                                else:
                                    regex = re.compile(pattern, re.DOTALL)
                                if regex.search(test_case):
                                    break
                            else:
                                error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - Unmatched test case ({item["name"]}):\n'
                                                f'    ({test_case})')
                                if VERBOSE: print(error_message)
                                errors.append(error_message)
        return (not errors)


def get_validator() -> Type[BuildFailureRCAValidator]:
    """Returns the validator class for this module"""
    return BuildFailureRCAValidator
