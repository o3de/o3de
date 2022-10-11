"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import unittest

import pytest
import unittest.mock as mock

import ly_test_tools.o3de.material_editor_test as material_editor_test

pytestmark = pytest.mark.SUITE_smoke


class TestMaterialEditorTestSuite(unittest.TestCase):

    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_TestData_ValidAP_TeardownAPOnce(self, mock_kill_processes):
        mock_editor_test_suite = material_editor_test.MaterialEditorTestSuite()
        mock_test_data_generator = mock_editor_test_suite._collected_test_data(mock.MagicMock())
        mock_asset_processor = mock.MagicMock()
        for test_data in mock_test_data_generator:
            test_data.asset_processor = mock_asset_processor
        mock_asset_processor.teardown.assert_called()
        assert test_data.asset_processor is None
        mock_kill_processes.assert_called_once_with(include_asset_processor=True)

    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_TestData_NoAP_NoTeardownAP(self, mock_kill_processes):
        mock_editor_test_suite = material_editor_test.MaterialEditorTestSuite()
        mock_test_data_generator = mock_editor_test_suite._collected_test_data(mock.MagicMock())
        for test_data in mock_test_data_generator:
            test_data.asset_processor = None
        mock_kill_processes.assert_called_once_with(include_asset_processor=False)

    @mock.patch('ly_test_tools.o3de.material_editor_test.MaterialEditorTestSuite.filter_session_shared_tests')
    def test_PytestCustomModifyItems_FunctionsMatch_AddsRunners(self, mock_filter_tests):
        class MockTestSuite(material_editor_test.MaterialEditorTestSuite):
            pass
        mock_func_1 = mock.MagicMock()
        mock_test = mock.MagicMock()
        runner_1 = material_editor_test.MaterialEditorTestSuite.Runner('mock_runner_1', mock_func_1, [mock_test])
        mock_run_pytest_func = mock.MagicMock()
        runner_1.run_pytestfunc = mock_run_pytest_func
        mock_result_pytestfuncs = [mock.MagicMock()]
        runner_1.result_pytestfuncs = mock_result_pytestfuncs
        mock_items = []
        mock_items.extend(mock_result_pytestfuncs)

        MockTestSuite._runners = [runner_1]
        mock_test_1 = mock.MagicMock()
        mock_test_2 = mock.MagicMock()
        mock_filter_tests.return_value = [mock_test_1, mock_test_2]

        MockTestSuite.pytest_custom_modify_items(mock.MagicMock(), mock_items, mock.MagicMock())
        assert mock_items == [mock_run_pytest_func, mock_result_pytestfuncs[0]]

    def test_GetSingleTests_NoSingleTests_EmptyList(self):
        class MockTestSuite(material_editor_test.MaterialEditorTestSuite):
            pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_single_tests()
        assert len(tests) == 0

    def test_GetSingleTests_OneSingleTests_ReturnsOne(self):
        class MockTestSuite(material_editor_test.MaterialEditorTestSuite):
            class MockSingleTest(material_editor_test.MaterialEditorSingleTest):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_single_tests()
        assert len(tests) == 1
        assert tests[0].__name__ == "MockSingleTest"
        assert issubclass(tests[0], material_editor_test.MaterialEditorSingleTest)

    def test_GetSingleTests_AllTests_ReturnsOnlySingles(self):
        class MockTestSuite(material_editor_test.MaterialEditorTestSuite):
            class MockSingleTest(material_editor_test.MaterialEditorSingleTest):
                pass
            class MockAnotherSingleTest(material_editor_test.MaterialEditorSingleTest):
                pass
            class MockNotSingleTest(material_editor_test.MaterialEditorSharedTest):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_single_tests()
        assert len(tests) == 2
        for test in tests:
            assert issubclass(test, material_editor_test.MaterialEditorSingleTest)

    def test_GetSharedTests_NoSharedTests_EmptyList(self):
        class MockTestSuite(material_editor_test.MaterialEditorTestSuite):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_shared_tests()
        assert len(tests) == 0

    def test_GetSharedTests_OneSharedTests_ReturnsOne(self):
        class MockTestSuite(material_editor_test.MaterialEditorTestSuite):
            class MockSharedTest(material_editor_test.MaterialEditorSharedTest):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_shared_tests()
        assert len(tests) == 1
        assert tests[0].__name__ == 'MockSharedTest'
        assert issubclass(tests[0], material_editor_test.MaterialEditorSharedTest)

    def test_GetSharedTests_AllTests_ReturnsOnlyShared(self):
        class MockTestSuite(material_editor_test.MaterialEditorTestSuite):
            class MockSharedTest(material_editor_test.MaterialEditorSharedTest):
                pass
            class MockAnotherSharedTest(material_editor_test.MaterialEditorSharedTest):
                pass
            class MockNotSharedTest(material_editor_test.MaterialEditorSingleTest):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_shared_tests()
        assert len(tests) == 2
        for test in tests:
            assert issubclass(test, material_editor_test.MaterialEditorSharedTest)

    @mock.patch('ly_test_tools.o3de.material_editor_test.MaterialEditorTestSuite.filter_session_shared_tests')
    @mock.patch('ly_test_tools.o3de.material_editor_test.MaterialEditorTestSuite.get_shared_tests')
    def test_GetSessionSharedTests_Valid_CallsCorrectly(self, mock_get_shared_tests, mock_filter_session):
        material_editor_test.MaterialEditorTestSuite.get_session_shared_tests(mock.MagicMock())
        assert mock_get_shared_tests.called
        assert mock_filter_session.called

    @mock.patch('ly_test_tools.o3de.multi_test_framework.skip_pytest_runtest_setup', mock.MagicMock())
    def test_FilterSessionSharedTests_OneSharedTest_ReturnsOne(self):
        def mock_test():
            pass
        mock_test.originalname = 'mock_test'
        mock_test.__name__ = mock_test.originalname
        mock_session_items = [mock_test]
        mock_shared_tests = [mock_test]

        selected_tests = material_editor_test.MaterialEditorTestSuite.filter_session_shared_tests(mock_session_items, mock_shared_tests)
        assert selected_tests == mock_session_items
        assert len(selected_tests) == 1

    @mock.patch('ly_test_tools.o3de.multi_test_framework.skip_pytest_runtest_setup', mock.MagicMock())
    def test_FilterSessionSharedTests_ManyTests_ReturnsCorrectTests(self):
        def mock_test():
            pass
        def mock_test_2():
            pass
        def mock_test_3():
            pass
        mock_test.originalname = 'mock_test'
        mock_test.__name__ = mock_test.originalname
        mock_test_2.originalname = 'mock_test_2'
        mock_test_2.__name__ = mock_test_2.originalname
        mock_test_3.originalname = 'mock_test_3'
        mock_test_3.__name__ = mock_test_3.originalname
        mock_session_items = [mock_test, mock_test_2]
        mock_shared_tests = [mock_test, mock_test_2, mock_test_3]

        selected_tests = material_editor_test.MaterialEditorTestSuite.filter_session_shared_tests(mock_session_items, mock_shared_tests)
        assert selected_tests == mock_session_items

    @mock.patch('ly_test_tools.o3de.multi_test_framework.skip_pytest_runtest_setup')
    def test_FilterSessionSharedTests_SkipOneTest_ReturnsCorrectTests(self, mock_skip):
        def mock_test():
            pass
        def mock_test_2():
            pass
        def mock_test_3():
            pass
        mock_skip.side_effect = [True, Exception]
        mock_test.originalname = 'mock_test'
        mock_test.__name__ = mock_test.originalname
        mock_test_2.originalname = 'mock_test_2'
        mock_test_2.__name__ = mock_test_2.originalname
        mock_test_3.originalname = 'mock_test_3'
        mock_test_3.__name__ = mock_test_3.originalname
        mock_session_items = [mock_test, mock_test_2]
        mock_shared_tests = [mock_test, mock_test_2, mock_test_3]

        selected_tests = material_editor_test.MaterialEditorTestSuite.filter_session_shared_tests(mock_session_items, mock_shared_tests)
        assert selected_tests == [mock_test]

    @mock.patch('ly_test_tools.o3de.multi_test_framework.skip_pytest_runtest_setup', mock.MagicMock(side_effect=Exception))
    def test_FilterSessionSharedTests_ExceptionDuringSkipSetup_SkipsAddingTest(self):
        def mock_test():
            pass
        mock_test.originalname = 'mock_test'
        mock_test.__name__ = mock_test.originalname
        mock_session_items = [mock_test]
        mock_shared_tests = [mock_test]

        selected_tests = material_editor_test.MaterialEditorTestSuite.filter_session_shared_tests(mock_session_items, mock_shared_tests)
        assert len(selected_tests) == 0

    def test_FilterSharedTests_TrueParams_ReturnsTrueTests(self):
        mock_test = mock.MagicMock()
        mock_test.is_batchable = True
        mock_test.is_parallelizable = True
        mock_test_2 = mock.MagicMock()
        mock_test_2.is_batchable = False
        mock_test_2.is_parallelizable = False
        mock_shared_tests = [mock_test, mock_test_2]

        filtered_tests = material_editor_test.MaterialEditorTestSuite.filter_shared_tests(
            mock_shared_tests, is_batchable=True, is_parallelizable=True)
        assert filtered_tests == [mock_test]

    def test_FilterSharedTests_FalseParams_ReturnsFalseTests(self):
        mock_test = mock.MagicMock()
        mock_test.is_batchable = True
        mock_test.is_parallelizable = True
        mock_test_2 = mock.MagicMock()
        mock_test_2.is_batchable = False
        mock_test_2.is_parallelizable = False
        mock_shared_tests = [mock_test, mock_test_2]

        filtered_tests = material_editor_test.MaterialEditorTestSuite.filter_shared_tests(
            mock_shared_tests, is_batchable=False, is_parallelizable=False)
        assert filtered_tests == [mock_test_2]
