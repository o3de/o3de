"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import unittest

import pytest
import unittest.mock as mock

import ly_test_tools.o3de.editor_test as editor_test
from ly_test_tools.launchers import launcher_helper

pytestmark = pytest.mark.SUITE_smoke


class TestEditorTestBase(unittest.TestCase):

    def test_EditorSharedTest_Init_CorrectDefaultAttributes(self):
        mock_editorsharedtest = editor_test.EditorSharedTest()
        assert mock_editorsharedtest.timeout == 180
        assert mock_editorsharedtest.test_module is None
        assert mock_editorsharedtest.attach_debugger is False
        assert mock_editorsharedtest.wait_for_debugger is False
        assert mock_editorsharedtest.is_batchable
        assert mock_editorsharedtest.is_parallelizable

    def test_EditorParallelTest_Init_CorrectDefaultAttributes(self):
        mock_editorsharedtest = editor_test.EditorParallelTest()
        assert mock_editorsharedtest.timeout == 180
        assert mock_editorsharedtest.test_module is None
        assert mock_editorsharedtest.attach_debugger is False
        assert mock_editorsharedtest.wait_for_debugger is False
        assert not mock_editorsharedtest.is_batchable
        assert mock_editorsharedtest.is_parallelizable

    def test_EditorBatchedTest_Init_CorrectDefaultAttributes(self):
        mock_editorsharedtest = editor_test.EditorBatchedTest()
        assert mock_editorsharedtest.timeout == 180
        assert mock_editorsharedtest.test_module is None
        assert mock_editorsharedtest.attach_debugger is False
        assert mock_editorsharedtest.wait_for_debugger is False
        assert mock_editorsharedtest.is_batchable
        assert not mock_editorsharedtest.is_parallelizable


class TestEditorTestSuite(unittest.TestCase):

    def test_EditorTestSuite_Init_CorrectDefaultAttributes(self):
        mock_editortestsuite = editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        assert mock_editortestsuite.global_extra_cmdline_args == ["-BatchMode", "-autotest_mode"]
        assert mock_editortestsuite.use_null_renderer is True
        assert mock_editortestsuite.timeout_shared_test == 300
        assert mock_editortestsuite._timeout_crash_log == 20
        assert mock_editortestsuite._test_fail_retcode == 0xF
        assert mock_editortestsuite._single_test_class is editor_test.EditorSingleTest
        assert mock_editortestsuite._shared_test_class is editor_test.EditorSharedTest
        assert mock_editortestsuite._log_attribute == "editor_log"
        assert mock_editortestsuite._log_name == "editor_test.log"
        mock_create_editor = launcher_helper.create_editor(mock_workspace)
        assert type(mock_editortestsuite._executable_function(mock_workspace)) == type(mock_create_editor)
