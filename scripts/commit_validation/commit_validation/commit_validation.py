#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import abc
import importlib
import os
import pkgutil
import re
import time
from typing import Dict, List, Tuple

VERBOSE = False

class Commit(abc.ABC):
    """An interface for accessing details about a commit"""

    @abc.abstractmethod
    def get_files(self) -> List[str]:
        """Returns a list of local files added/modified by the commit"""
        pass

    @abc.abstractmethod
    def get_removed_files(self) -> List[str]:
        """Returns a list of local files removed by the commit"""
        pass

    @abc.abstractmethod
    def get_file_diff(self, str) -> str:
        """
        Given a file name, returns a string in unified diff format
        that represents the changes made to that file for this commit.
        Most validators will only pay attention to added lines (with + in front)
        """
        pass

    @abc.abstractmethod
    def get_description(self) -> str:
        """Returns the description of the commit"""
        pass

    @abc.abstractmethod
    def get_author(self) -> str:
        """Returns the author of the commit"""
        pass


def validate_commit(commit: Commit, out_errors: List[str] = None, ignore_validators: List[str] = None) -> bool:
    """Validates a commit against all validators

    :param commit: The commit to validate
    :param out_errors: if not None, will populate with the list of errors given by the validators
    :param ignore_validators: Optional list of CommitValidator classes to ignore, by class name
    :return: True if there are no validation errors, and False otherwise
    """
    failed_count = 0
    passed_count = 0
    start_time = time.time()

    # Find all the validators in the validators package (recursively)
    validator_classes = []
    validators_dir = os.path.join(os.path.dirname(__file__), 'validators')
    for _, module_name, is_package in pkgutil.iter_modules([validators_dir]):
        if not is_package:
            module = importlib.import_module('commit_validation.validators.' + module_name)
            validator = module.get_validator()
            if ignore_validators and validator.__name__ in ignore_validators:
                print(f"Disabled validation for '{validator.__name__}'")
            else:
                validator_classes.append(validator)

    error_summary = {}

    # Process validators
    for validator_class in validator_classes:
        validator = validator_class()
        validator_name = validator.__class__.__name__

        error_list = []
        passed = validator.run(commit, errors = error_list)
        if passed:
            passed_count += 1
            print(f'{validator.__class__.__name__} PASSED')
        else:
            failed_count += 1
            print(f'{validator.__class__.__name__} FAILED')
        error_summary[validator_name] = error_list
        
    end_time = time.time()

    if failed_count:
        print("VALIDATION FAILURE SUMMARY")
        for val_name in error_summary.keys():
            errors = error_summary[val_name]
            if errors:
                for error_message in errors:
                    first_line = True
                    for line in error_message.splitlines():
                        if first_line:
                            first_line = False
                            print(f'VALIDATOR_FAILED: {val_name} {line}')
                        else:
                            print(f'    {line}') # extra detail lines do not need machine parsing

    stats_strs = []
    if failed_count > 0:
        stats_strs.append(f'{failed_count} failed')
    if passed_count > 0:
        stats_strs.append(f'{passed_count} passed')
    stats_str = ', '.join(stats_strs) + f' in {end_time - start_time:.2f}s'

    print()
    print(stats_str)

    return failed_count == 0

def IsFileSkipped(file_name) -> bool:
    if os.path.splitext(file_name)[1].lower() not in SOURCE_AND_SCRIPT_FILE_EXTENSIONS:
        skipped = True
        for pattern in SOURCE_AND_SCRIPT_FILE_PATTERNS:
            if pattern.match(file_name):
                skipped = False
                break
        return skipped
    return False

class CommitValidator(abc.ABC):
    """A commit validator"""

    @abc.abstractmethod
    def run(self, commit: Commit, errors: List[str]) -> bool:
        """Validates a commit

        :param commit: The commit to validate
        :param errors: List of errors generated, append them to this list
        :return: True if the commit is valid, and False otherwise
        """
        pass


SOURCE_FILE_EXTENSIONS: Tuple[str, ...] = (
    '.c', '.cc', '.cpp', '.cxx', '.h', '.hpp', '.hxx', '.inl', '.m', '.mm', '.cs', '.java'
)
"""File extensions for compiled source code"""

SCRIPT_FILE_EXTENSIONS: Tuple[str, ...] = (
    '.py', '.lua', '.bat', '.cmd', '.sh', '.js'
)
"""File extensions for interpreted code"""

BUILD_FILE_EXTENSIONS: Tuple[str, ...] = (
    '.cmake',
)
"""File extensions for build files"""

SOURCE_AND_SCRIPT_FILE_EXTENSIONS: Tuple[str, ...] = SOURCE_FILE_EXTENSIONS + SCRIPT_FILE_EXTENSIONS + BUILD_FILE_EXTENSIONS
"""File extensions for both compiled and interpreted code"""

BUILD_FILE_PATTERNS: Tuple[re.Pattern, ...] = (
    re.compile(r'.*CMakeLists\.txt'),
    re.compile(r'.*Jenkinsfile')
)
"""File patterns for build files"""

SOURCE_AND_SCRIPT_FILE_PATTERNS: Tuple[re.Pattern, ...] = BUILD_FILE_PATTERNS

EXCLUDED_VALIDATION_PATTERNS = [
    '*/.git/*',
    '*/3rdParty/*',
    '*/__pycache__/*',
    '*/External/*',
    'build', # build artifacts
    '*/Cache/*', # Asset processing artifacts
    'install', # install layout artifacts
    'python/runtime',
    'restricted/*/Code/Framework/AzCore/azgnmx/azgnmx/*',
    'restricted/*/Tools/*RemoteControl',
    '*/user/Cache/*',
    '*/user/log/*',
    '*/user/log_test_1/*',
    '*/user/log_test_2/*',
]
