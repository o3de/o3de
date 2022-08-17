"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import unittest

import pytest
import unittest.mock as mock

import ly_test_tools.o3de.material_editor_test as material_editor_test
from ly_test_tools.launchers import launcher_helper

pytestmark = pytest.mark.SUITE_smoke


class TestMaterialEditorTestBase(unittest.TestCase):

    def test_MaterialEditorSharedTest_Init_CorrectAttributes(self):
        mock_materialeditorsharedtest = material_editor_test.MaterialEditorSharedTest()
        assert mock_materialeditorsharedtest.timeout == 180
        assert mock_materialeditorsharedtest.test_module is None
        assert mock_materialeditorsharedtest.attach_debugger is False
        assert mock_materialeditorsharedtest.wait_for_debugger is False
        assert mock_materialeditorsharedtest.is_batchable
        assert mock_materialeditorsharedtest.is_parallelizable

    def test_MaterialEditorParallelTest_Init_CorrectAttributes(self):
        mock_materialeditorparalleltest = material_editor_test.MaterialEditorParallelTest()
        assert mock_materialeditorparalleltest.timeout == 180
        assert mock_materialeditorparalleltest.test_module is None
        assert mock_materialeditorparalleltest.attach_debugger is False
        assert mock_materialeditorparalleltest.wait_for_debugger is False
        assert not mock_materialeditorparalleltest.is_batchable
        assert mock_materialeditorparalleltest.is_parallelizable

    def test_MaterialEditorBatchedTest_Init_CorrectAttributes(self):
        mock_materialeditorbatchedtest = material_editor_test.MaterialEditorBatchedTest()
        assert mock_materialeditorbatchedtest.timeout == 180
        assert mock_materialeditorbatchedtest.test_module is None
        assert mock_materialeditorbatchedtest.attach_debugger is False
        assert mock_materialeditorbatchedtest.wait_for_debugger is False
        assert mock_materialeditorbatchedtest.is_batchable
        assert not mock_materialeditorbatchedtest.is_parallelizable


class TestMaterialEditorTestSuite(unittest.TestCase):

    def test_EditorTestSuite_Init_CorrectDefaultAttributes(self):
        mock_materialeditorbatchedtest = material_editor_test.MaterialEditorTestSuite()
        mock_workspace = mock.MagicMock()
        assert mock_materialeditorbatchedtest.global_extra_cmdline_args == ["-BatchMode", "-autotest_mode"]
        assert mock_materialeditorbatchedtest.use_null_renderer is True
        assert mock_materialeditorbatchedtest.timeout_shared_test == 300
        assert mock_materialeditorbatchedtest._timeout_crash_log == 20
        assert mock_materialeditorbatchedtest._test_fail_retcode == 0xF
        assert mock_materialeditorbatchedtest._single_test_class is material_editor_test.MaterialEditorSingleTest
        assert mock_materialeditorbatchedtest._shared_test_class is material_editor_test.MaterialEditorSharedTest
        assert mock_materialeditorbatchedtest._log_attribute == "material_editor_log"
        assert mock_materialeditorbatchedtest._log_name == "material_editor_test.log"
        mock_create_material_editor = launcher_helper.create_material_editor(mock_workspace)
        assert type(
            mock_materialeditorbatchedtest._executable_function(mock_workspace)) == type(mock_create_material_editor)
