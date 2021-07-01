#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import unittest

from commit_validation.tests.mocks.mock_commit import MockCommit
from commit_validation.validators.diff_whitespace_validator import WhitespaceValidator


class WhitespaceValidatorTests(unittest.TestCase):

    def test_DiffWhitespace_WhitespaceLinesContiguousWithCodeChange_pass(self):
        commit = MockCommit(
            files = ['/someCppFile.cpp'],
            file_diffs={'/someCppFile.cpp':
                            'This file\n'
                            '-was edited'
                            '+  and has a diff including plus and minus symbols\n'
                            '+  \n'
                            'and should pass\n'})
        error_list = []
        self.assertTrue(WhitespaceValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_DiffWhitespace_WhitespaceLinesSeparateAtEOF_fail(self):
        commit = MockCommit(
            files = ['/someCppFile.cpp'],
            file_diffs={'/someCppFile.cpp':
                            'This file\n'
                            '-was edited'
                            '+  and has an implementation diff\n'
                            'but should not pass because of a whitespace only change:\n'
                            '-    \n'})
        error_list = []
        self.assertFalse(WhitespaceValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_DiffWhitespace_WhitespaceLinesSeparateAtEOF_fail_More(self):
        commit = MockCommit(
            files = ['/someCppFile.cpp'],
            file_diffs={'/someCppFile.cpp':
                            '+    \n'
                            'This file\n'
                            '-was edited'
                            '+  and has an implementation diff\n'
                            'but should not pass because of a whitespace only change\n'
                            'on the first line\n'})
        error_list = []
        self.assertFalse(WhitespaceValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_DiffWhitespace_WhitespaceLinesSeparateFromCode_fail(self):
        commit = MockCommit(
            files = ['/someCppFile.cpp'],
            file_diffs={'/someCppFile.cpp':
                            'This file\n'
                            'should not pass because of a whitespace only change:\n'
                            '-    \n'
                            'since was edited'
                            '+  and has an implementation diff\n'
                            '-  later on'})
        error_list = []
        self.assertFalse(WhitespaceValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_DiffWhitespace_WhitespaceOnlyChange_pass(self):
        commit = MockCommit(
            files = ['/someCppFile.cpp'],
            file_diffs={'/someCppFile.cpp':
                            'This file has whitespace only changes\n'
                            '+  \n'
                            '+  \n'
                            'and should pass\n'
                            '-  \n'})
        error_list = []
        self.assertTrue(WhitespaceValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_DiffWhitespace_CodeChangeOnly_pass(self):
        commit = MockCommit(
            files = ['/someCppFile.cpp'],
            file_diffs={'/someCppFile.cpp':
                            'This file has'
                            '+  no whitespace only lines changed\n'
                            '  \n'
                            '-and should pass\n'
                            '  \n'})
        error_list = []
        self.assertTrue(WhitespaceValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")


if __name__ == '__main__':
    unittest.main()
