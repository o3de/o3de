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
from commit_validation.validators.tabs_validator import TabsValidator


class TabsValidatorTests(unittest.TestCase):
    def test_fileWithNoTabs_passes(self):
        commit = MockCommit(
            files=['/someCppFile.cpp'],
            file_diffs={ '/someCppFile.cpp' : '+This file contains\n'
                                              '+    no tabs\n'
                                              '+    but does have spaces\n'})
        error_list = []
        self.assertTrue(TabsValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileWithTabs_fails(self):
        commit = MockCommit(
            files=['/someCppFile.cpp'],
            file_diffs={ '/someCppFile.cpp' : '+This file contains\n'
                                              '+\tTHE DREADED TAB CHARACTER\n'})
        error_list = []
        self.assertFalse(TabsValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_fileWithTabs_butNotInDiffArea_passes(self):
        commit = MockCommit(
            files=['/someCppFile.cpp'],
            file_diffs={ '/someCppFile.cpp' : '+This file contains\n'
                                              '\tTHE DREADED TAB CHARACTER\n'
                                              'But since its not in a diff line\n'
                                              'It does not matter not even if in \n'
                                              '-\tA REMOVED LINE with the tab character\n'})
        error_list = []
        self.assertTrue(TabsValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")


    def test_fileExtensionIgnored_passes(self):
        commit = MockCommit(files=['/someCppFile.waf_files'])
        error_list = []
        self.assertTrue(TabsValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileWithNoTabsbutAnEmoji_passes(self):
        commit = MockCommit(
            files=['/someCppFile_trap.cpp'],
            file_diffs={ '/someCppFile_trap.cpp' : '+This file contains\n'
                                                   '+    no tabs\n'
                                                   '+    but has an emoji \U0001F4A9\n'})
        error_list = []
        self.assertTrue(TabsValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileWithTabsAndEmoji_fails(self):
        commit = MockCommit(
            files=['/another_trap.cpp'],
            file_diffs={ '/another_trap.cpp' : '+This file contains an emoji \U0001F4A9 \n'
                                               '+\tbut also a tab!\n'})
        error_list = []
        self.assertFalse(TabsValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

if __name__ == '__main__':
    unittest.main()
