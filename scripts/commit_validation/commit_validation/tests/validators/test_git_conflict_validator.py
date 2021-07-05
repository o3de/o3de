#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import unittest
from unittest.mock import patch, mock_open

from commit_validation.tests.mocks.mock_commit import MockCommit
from commit_validation.validators.git_conflict_validator import GitConflictValidator


class NewlineValidatorTests(unittest.TestCase):
    @patch('builtins.open', mock_open(read_data='This file is completely normal\n'
                                                'and should pass\n'))
    def test_HasConflictMarkers_NoMarkers_Pass(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertTrue(GitConflictValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    @patch('builtins.open', mock_open(read_data='This file has a start marker\n'
                                                '<<<<<<< ours\n'
                                                'and should fail\n'))
    def test_HasConflictMarkers_StartMarker_Fail(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertFalse(GitConflictValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='This file has a diff3 marker from using --conflict=diff3\n'
                                                '||||||| base\n'
                                                'and should fail\n'))
    def test_HasConflictMarkers_BaseMarker_Fail(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertFalse(GitConflictValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='This file has a diff marker\n'
                                                '=======\n'
                                                'and should fail\n'))
    def test_HasConflictMarkers_DiffMarker_Fail(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertFalse(GitConflictValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='This file has and end marker\n'
                                                '>>>>>>> theirs\n'
                                                'and should fail\n'))
    def test_HasConflictMarkers_EndMarker_Fail(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertFalse(GitConflictValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    @patch('builtins.open', mock_open(read_data='This file has a equals sign divider of length seven, but is indented\n'
                                                '    /*\n'
                                                '    =======\n'
                                                '    */'
                                                'and should pass\n'))
    def test_HasConflictMarkers_IndentedCommentDivider_Pass(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertTrue(GitConflictValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    @patch('builtins.open', mock_open(read_data='This file has an unindented equals sign divider, of length eight\n'
                                                '/*\n'
                                                '========\n'
                                                '*/'
                                                'and should pass\n'))
    def test_HasConflictMarkers_LongCommentDivider_Pass(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertTrue(GitConflictValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    @patch('builtins.open', mock_open(read_data='This file has an unindented equals sign divider, of length six\n'
                                                '/*\n'
                                                '======\n'
                                                '*/'
                                                'and should pass\n'))
    def test_HasConflictMarkers_ShortCommentDivider_Pass(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertTrue(GitConflictValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")


if __name__ == '__main__':
    unittest.main()
