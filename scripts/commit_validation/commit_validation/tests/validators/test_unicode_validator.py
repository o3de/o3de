#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import unittest
from unittest.mock import patch, mock_open

from commit_validation.tests.mocks.mock_commit import MockCommit
from commit_validation.validators.unicode_validator import UnicodeValidator


class UnicodeValidatorTests(unittest.TestCase):

    @patch('builtins.open', mock_open(read_data='This file contains no unicode characters'))
    def test_fileWithNoUnicode_passes(self):
        commit = MockCommit(files=['/someCppFile.cpp'])
        error_list = []
        self.assertTrue(UnicodeValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    @patch('builtins.open', mock_open(read_data='This file contains unicode character \u2318'))
    def test_fileWithUnicode_fails(self):
        commit = MockCommit(files=['/someCppFile.cpp'])
        error_list = []
        self.assertFalse(UnicodeValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='This file contains unicode character \u2318'))
    def test_fileExtensionIgnored_passes(self):
        commit = MockCommit(files=['/someCppFile.somerandomextension'])
        error_list = []
        self.assertTrue(UnicodeValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    @patch('builtins.open', mock_open(read_data='This file contains unicode character: \\u2318'))
    def test_fileWithEscapedUnicode_passes(self):
        commit = MockCommit(files=['/someCppFile.cpp'])
        error_list = []
        self.assertTrue(UnicodeValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

if __name__ == '__main__':
    unittest.main()
