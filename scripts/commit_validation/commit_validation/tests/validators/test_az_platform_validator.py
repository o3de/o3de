#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import unittest
from unittest.mock import patch, mock_open

from commit_validation import pal_allowedlist
from commit_validation.tests.mocks.mock_commit import MockCommit
from commit_validation.validators.az_platform_validator import AzPlatformValidator

class AzPlatformValidatorTests(unittest.TestCase):
    def test_fileWithNoAzPlatformMacro_passes(self):
        commit = MockCommit(
            files=['someCppFile.cpp'],
            file_diffs = {
                'someCppFile.cpp' : '+This file does not contain\n+AZ_PLATFORM_MACRO\n'
            }
        )
        error_list = []
        self.assertTrue(AzPlatformValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    # make sure it only examines parts that have actually been changed by this commit
    # note that the line does not start with +
    def test_fileWithMacroInNonChangedSection_passes(self):
        commit = MockCommit(
            files=['someCppFile.cpp', 'otherfile.cpp'],
            file_diffs = { # one file has no diff marker, one file indicates its removed.
                'someCppFile.cpp' : 'Stuff\n#if defined(AZ_PLATFORM_MACRO\n)',
                'otherfile.cpp'  : 'Stuff\n-#if defined(AZ_PLATFORM_MACRO\n)' 
            }
        )
        error_list = []
        self.assertTrue(AzPlatformValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileWithAzPlatformMacro_fails(self):
        commit = MockCommit(
            files=['someCppFile.cpp', 'otherfile.cpp'],
            file_diffs = {
                'someCppFile.cpp' : '+This file does contain\n'
                                    '+#if defined(AZ_PLATFORM_MACRO)\n',
                'otherfile.cpp' : 'This File has a different form\n'
                                  '+#     if defined(AZ_PLATFORM_MACRO)\n'
            })
        error_list = []
        self.assertFalse(AzPlatformValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_fileExtensionIgnored_passes(self):
        commit = MockCommit(files=['someCppFile.waf_files'])
        error_list = []
        self.assertTrue(AzPlatformValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    @patch('commit_validation.pal_allowedlist.load', return_value=pal_allowedlist.PALAllowedlist(['*/some/path/*']))
    def test_fileAllowedlisted_passes(self, mocked_load):
        commit = MockCommit(files=['/path/to/some/path/someCppFile.cpp'])
        error_list = []
        self.assertTrue(AzPlatformValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

if __name__ == '__main__':
    unittest.main()
