#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT#
#

import unittest
from unittest.mock import patch, mock_open

from commit_validation.tests.mocks.mock_commit import MockCommit
from commit_validation.validators.copyright_header_validator import CopyrightHeaderValidator


class CopyrightHeaderValidatorTests(unittest.TestCase):
    @patch('builtins.open', mock_open(read_data='This file does contain\n'
                                                'Copyright (c) Contributors to the Open 3D Engine Project.\nFor complete copyright and license terms please see the LICENSE at the root of this distribution., so it should pass\n'))
    def test_fileWithCopyrightHeader_passes(self):
        commit = MockCommit(files=['/someCppFile.cpp'])
        files = [
            'This file does contain\n'
            'Copyright (c) Contributors to the Open 3D Engine Project.\nFor complete copyright and license terms please see the LICENSE at the root of this distribution., so it should pass\n',

            'This file does contain\n'
            'Copyright(c) Contributors to the Open 3D Engine Project.\nFor complete copyright and license terms please see the LICENSE at the root of this distribution., so it should pass\n'
            'and there\'s no space between "Copyright" and "(c)"',

            'This file has a upper-case C between the parenthesis\n'
            '// Copyright (C) Contributors to the Open 3D Engine Project.\nFor complete copyright and license terms please see the LICENSE at the root of this distribution.',
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

    @patch('builtins.open', mock_open(read_data='This file does contains legacy header\n'
                                                'this{0}file{0}Copyright{0}(c){0}Amazon.com\n'.format(' ')))
    def test_fileWithLegacyAmazonCopyrightHeader_fails(self):
        commit = MockCommit(files=['/someCppFile.cpp'])
        error_list = []
        self.assertFalse(CopyrightHeaderValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='This file does contains legacy header\n'
                                                'Modifications{0}copyright{0}Amazon.com,{0}Inc.{0}or{0}its affiliates\n'.format(' ')))
    def test_fileWithAmazonModificationCopyrightHeader_passes(self):
        commit = MockCommit(files=['/someCppFile.cpp'])
        error_list = []
        self.assertFalse(CopyrightHeaderValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='Copyright (c) Contributors to the Open 3D Engine Project.\n'
                                                '******************\n'.format(' ')))
    def test_fileWithStaleO3DECopyrightHeader_fails(self):
        commit = MockCommit(files=['/someCppFile.cpp'])
        error_list = []
        self.assertFalse(CopyrightHeaderValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='This file does contains legacy header\n'
                                                'Copyright{0}Crytek\n'.format(' ')))
    def test_fileWithCrytekCopyrightHeader_fails(self):
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

