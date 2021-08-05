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
from commit_validation.validators.platform_macro_validator import PlatformMacroValidator


class PlatformMacroValidatorTests(unittest.TestCase):
    def test_fileWithNoPlatformMacro_passes(self):
        commit = MockCommit(
            files=['someCppFile.cpp'],
            file_diffs={ 'someCppFile.cpp' : '+This file does not contain\n'
                                             '+a platform macro\n'
                                             '+#define ACCEPTABLE_MACRO\n'
                        })
        error_list = []
        self.assertTrue(PlatformMacroValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileWithPlatformMacro_errors(self):
        commit = MockCommit(
            files=['someCppFile.cpp'],
            file_diffs={ 'someCppFile.cpp' : '+This file does contain\n'
                                             '+a platform macro\n'
                                             '+#if defined(APPLE)\n'
                    })
        error_list = []
        self.assertFalse(PlatformMacroValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")
    
    def test_fileWithPlatformMacro_ButNotInChangedPart_passes(self):
        commit = MockCommit(
            files=['someCppFile.cpp'],
            file_diffs={ 'someCppFile.cpp' : '+This file does contain\n'
                                             '+a platform macro\n'
                                             '-#if defined(APPLE)\n'
                                             'But not in a part thats relevant to the diff'
                                             '#if defined(APPLE)\n'
                    })
        error_list = []
        self.assertTrue(PlatformMacroValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileExtensionIgnored_passes(self):
        commit = MockCommit(files=['someCppFile.waf_files'])
        error_list = []
        self.assertTrue(PlatformMacroValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_platformFolderIgnored_passes(self):
        commit = MockCommit(files=['/path/to/Platform/SomePlatform/someCppFile.cpp'])
        error_list = []
        self.assertTrue(PlatformMacroValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    @patch('commit_validation.pal_allowedlist.load', return_value=pal_allowedlist.PALAllowedlist(['*/some/path/*']))
    def test_fileAllowedlisted_passes(self, mocked_load):
        commit = MockCommit(files=['/path/to/some/path/someCppFile.cpp'])
        error_list = []
        self.assertTrue(PlatformMacroValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")


if __name__ == '__main__':
    unittest.main()
