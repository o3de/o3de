#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import unittest
from unittest.mock import patch, mock_open

from commit_validation.tests.mocks.mock_commit import MockCommit
from commit_validation.validators.pragma_optimize_validator import PragmaOptimizeValidator


class PragmaOptimizeValidatorTests(unittest.TestCase):
    def test_fileWithNoPragmaOptimize_passes(self):
        commit = MockCommit(
                files=['/someCppFile.cpp'],
                file_diffs={ 
                    '/someCppFile.cpp' : '+// This file does not contain\n'
                                         '+// the p r a g m a o p t i m i z e'
                })
        error_list = []
        self.assertTrue(PragmaOptimizeValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileWithNoPragma3rdPartyPath_passes(self):
        commit = MockCommit(files=['/3rdParty/someCppFile.cpp'])
        error_list = []
        self.assertTrue(PragmaOptimizeValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileWithNoPragmaExternal_passes(self):
        commit = MockCommit(files=['/External/someCppFile.cpp'])
        error_list = []
        self.assertTrue(PragmaOptimizeValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileWithPragmaOptimize_fails(self):
        commit = MockCommit(
            files=['/someCppFile.cpp'],
            file_diffs={ 
                    '/someCppFile.cpp' : '+// This file contains\n'
                                         '+// a line\n'
                                         '+// with #pragma optimize("", off)'
                })
        error_list = []
        self.assertFalse(PragmaOptimizeValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_fileWithPragmaOptimize_in_unchanged_part_passes(self):
        commit = MockCommit(
            files=['/someCppFile.cpp'],
            file_diffs={ 
                    '/someCppFile.cpp' : '+// This file contains\n'
                                         '+// a line\n'
                                         '-#pragma optimize("", off)\n'
                                         'but its in a deleted line or a line that is not part of the diff\n'
                                         '#pragma optimize("", off)\n'
                })
        error_list = []
        self.assertTrue(PragmaOptimizeValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileExtensionIgnored_passes(self):
        commit = MockCommit(files=['/someCppFile.waf_files'])
        error_list = []
        self.assertTrue(PragmaOptimizeValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")
        

    def test_fileWithPragma3rdPartyPath_passes(self):
        commit = MockCommit(files=['/3rdParty/someCppFile.cpp'])
        error_list = []
        self.assertTrue(PragmaOptimizeValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_fileWithPragmaExternal_passes(self):
        commit = MockCommit(files=['/External/someCppFile.cpp'])
        error_list = []
        self.assertTrue(PragmaOptimizeValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

if __name__ == '__main__':
    unittest.main()
