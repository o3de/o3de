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
from commit_validation.validators.generated_files_validator import GeneratedFilesValidator


@patch('builtins.open', mock_open(read_data='unused\n'))
class GeneratedFilesValidatorTests(unittest.TestCase):

    def test_IsGenerated_NormalFile_pass(self):
        commit = MockCommit(files=['/someFile.cpp'])
        error_list = []
        self.assertTrue(GeneratedFilesValidator().run(commit, error_list))
        self.assertEqual(len(error_list), 0, f"Unexpected errors: {error_list}")

    def test_IsGenerated_DotGeneratedFile_fail(self):
        commit = MockCommit(files=['/someFile.generated.cpp'])
        error_list = []
        self.assertFalse(GeneratedFilesValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_IsGenerated_QtMOC_fail(self):
        commit = MockCommit(files=['/moc_someFile.cpp'])
        error_list = []
        self.assertFalse(GeneratedFilesValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_IsGenerated_QtQRC_fail(self):
        commit = MockCommit(files=['/qrc_someFile.cpp'])
        error_list = []
        self.assertFalse(GeneratedFilesValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_IsGenerated_QtUiDotH_fail(self):
        commit = MockCommit(files=['/ui_someFile.h'])
        error_list = []
        self.assertFalse(GeneratedFilesValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_IsGenerated_CMakeCacheFile_fail(self):
        commit = MockCommit(files=['/CMakeCache.txt'])
        error_list = []
        self.assertFalse(GeneratedFilesValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_IsGenerated_CMakeCacheExtension_fail(self):
        commit = MockCommit(files=['/AzCore.Benchmarks.rule'])
        error_list = []
        self.assertFalse(GeneratedFilesValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_IsGenerated_Tempfile_fail(self):
        commit = MockCommit(files=['/someFile.tmp'])
        error_list = []
        self.assertFalse(GeneratedFilesValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_IsGenerated_UnixObject_fail(self):
        commit = MockCommit(files=['/someFile.o'])
        error_list = []
        self.assertFalse(GeneratedFilesValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_IsGenerated_WindowsObject_fail(self):
        commit = MockCommit(files=['/someFile.obj'])
        error_list = []
        self.assertFalse(GeneratedFilesValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_IsGenerated_MicrosoftSolution_fail(self):
        commit = MockCommit(files=['/someFile.sln'])
        error_list = []
        self.assertFalse(GeneratedFilesValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")

    def test_IsGenerated_Logfile_fail(self):
        commit = MockCommit(files=['/someFile.log'])
        error_list = []
        self.assertFalse(GeneratedFilesValidator().run(commit, error_list))
        self.assertNotEqual(len(error_list), 0, f"Errors were expected but none were returned.")


if __name__ == '__main__':
    unittest.main()
