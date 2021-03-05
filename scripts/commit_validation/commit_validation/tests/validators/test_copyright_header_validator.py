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

import unittest
from unittest.mock import patch, mock_open

from commit_validation.tests.mocks.mock_commit import MockCommit
from commit_validation.validators.copyright_header_validator import CopyrightHeaderValidator


class CopyrightHeaderValidatorTests(unittest.TestCase):
    @patch('builtins.open', mock_open(read_data='This file does contain\n'
                                                'Copyright (c) Amazon.com, so it should pass\n'))
    def test_fileWithCopyrightHeader_passes(self):
        commit = MockCommit(files=['/someCppFile.cpp'])
        files = [
            'This file does contain\n'
            'Copyright (c) Amazon.com, so it should pass\n',

            'This file does contain\n'
            'Copyright(c) Amazon.com, so it should pass\n'
            'and there\'s no space between "Copyright" and "(c)"',

            'This file has a upper-case C between the parenthesis\n'
            '// Copyright (C) Amazon.com, Inc. or its affiliates.',
        ]
        for file in files:
            with patch('builtins.open', mock_open(read_data=file)):
                error_list = []
                self.assertTrue(CopyrightHeaderValidator().run(commit, error_list))
                self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileWithCopyrightHeaderExternalDir_passes(self):
        commit = MockCommit(files=['/External/someCppFile.cpp'])
        error_list = []
        self.assertTrue(CopyrightHeaderValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    @patch('builtins.open', mock_open(read_data='This file does not contain\n'
                                                'the copyright header\n'))
    def test_fileWithNoCopyrightHeader_fails(self):
        commit = MockCommit(files=['/someCppFile.cpp'])
        error_list = []
        self.assertFalse(CopyrightHeaderValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_fileExtensionIgnored_passes(self):
        commit = MockCommit(files=['/someCppFile.waf_files'])
        error_list = []
        self.assertTrue(CopyrightHeaderValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileWith3rdPartyPath_passes(self):
        commit = MockCommit(files=['/3rdParty/someCppFile.cpp'])
        error_list = []
        self.assertTrue(CopyrightHeaderValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileWithExternal_passes(self):
        commit = MockCommit(files=['/External/someCppFile.cpp'])
        error_list = []
        self.assertTrue(CopyrightHeaderValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")


if __name__ == '__main__':
    unittest.main()
