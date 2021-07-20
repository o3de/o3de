#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import unittest
from unittest.mock import patch, mock_open

from commit_validation.tests.mocks.mock_commit import MockCommit
from commit_validation.validators.newline_validator import NewlineValidator

class NewlineValidatorTests(unittest.TestCase):
    @patch('builtins.open', mock_open(read_data='This file uses carriage returns with line feeds\r\n'
                                                'and should fail since only line feeds are allowed\r\n'
                                                'despite appropriately ending with an extra line\r\n'))
    def test_LineEndings_CRLF_fail(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertFalse(NewlineValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='This file uses line feeds\n'
                                                'and ends with an extra line\n'))
    def test_LineEndings_LF_pass(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertTrue(NewlineValidator().run(commit, error_list))
        self.assertTrue(not error_list)

    @patch('builtins.open', mock_open(read_data='This file is full of garbage\n'
                                                'which would normally fail\r\n'
                                                'though it should not due to its file extension\n\r'
                                                'which should skip validation'))
    def test_LineEndings_RandomFileExtension_skip(self):
        commit = MockCommit(files=['/someFile.garbage'])
        error_list = []
        self.assertTrue(NewlineValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    @patch('builtins.open', mock_open(read_data='This file mixes line feed endings\n'
                                                'with carriage return and line feed endings\r\n'))
    def test_LineEndings_Mixed_fail(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertFalse(NewlineValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='This file ends\n'
                                                'with no newline'))
    def test_LineEndings_NoFinalNewline_fail(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertFalse(NewlineValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='This file ends with CRLF newlines\r\n'
                                                'but not at the end'))
    def test_LineEndings_NoFinalCRLF_fail(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertFalse(NewlineValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='This file ends with extra newlines\n'
                                                '\n'))
    def test_LineEndings_ExtraFinalNewline_fail(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertFalse(NewlineValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='This file ends with CRLF extra newlines\r\n'
                                                '\r\n'))
    def test_LineEndings_ExtraFinalCRLF_fail(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertFalse(NewlineValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")


if __name__ == '__main__':
    unittest.main()
