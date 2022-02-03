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
from commit_validation.validators.crc_validator import CrcValidator


class CrcValidatorTests(unittest.TestCase):

    @patch('builtins.open', mock_open(read_data='This file does not contain an AZ_CRC macro'))
    def test_fileWithNoCrc_passes(self):
        commit = MockCommit(files=['/someCppFile.cpp'])
        error_list = []
        self.assertTrue(CrcValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    @patch('builtins.open', mock_open(read_data='This file contains an invalid CRC macro AZ_CRC("My string", 0xabcdef00)'))
    def test_fileWithInvalidCrc_fails(self):
        commit = MockCommit(files=['/someCppFile.cpp'])
        error_list = []
        self.assertFalse(CrcValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='This file contains a valid CRC macro AZ_CRC("My string", 0x18fbd270)'))
    def test_fileWithValidCrc_fails(self):
        commit = MockCommit(files=['/someCppFile.cpp'])
        error_list = []
        self.assertTrue(CrcValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    @patch('builtins.open', mock_open(read_data='This file contains an invalid CRC macro AZ_CRC("My string", 0xabcdef00)'))
    def test_fileExtensionIgnored_passes(self):
        commit = MockCommit(files=['/someCppFile.somerandomextension'])
        error_list = []
        self.assertTrue(CrcValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

if __name__ == '__main__':
    unittest.main()
