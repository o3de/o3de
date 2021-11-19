"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import unittest

import pytest
import unittest.mock as mock

import ly_test_tools
import ly_test_tools.o3de.editor_test as editor_test

pytestmark = pytest.mark.SUITE_smoke

class TestEditorTestBase(unittest.TestCase):

    def test_EditorSharedTest_Init_CorrectAttributes(self):
        mock_editorsharedtest = editor_test.EditorSharedTest()
        assert mock_editorsharedtest.is_batchable == True
        assert mock_editorsharedtest.is_parallelizable == True

    def test_EditorParallelTest_Init_CorrectAttributes(self):
        mock_editorsharedtest = editor_test.EditorParallelTest()
        assert mock_editorsharedtest.is_batchable == False
        assert mock_editorsharedtest.is_parallelizable == True

    def test_EditorBatchedTest_Init_CorrectAttributes(self):
        mock_editorsharedtest = editor_test.EditorBatchedTest()
        assert mock_editorsharedtest.is_batchable == True
        assert mock_editorsharedtest.is_parallelizable == False

class TestResultBase(unittest.TestCase):

    def setUp(self):
        self.mock_result = editor_test.Result.Base()

    def test_GetOutputStr_HasOutput_ReturnsCorrectly(self):
        self.mock_result.output = 'expected output'
        assert self.mock_result.get_output_str() == 'expected output'

    def test_GetOutputStr_NoOutput_ReturnsCorrectly(self):
        self.mock_result.output = None
        assert self.mock_result.get_output_str() == '-- No output --'

    def test_GetEditorLogStr_HasOutput_ReturnsCorrectly(self):
        self.mock_result.editor_log = 'expected log output'
        assert self.mock_result.get_editor_log_str() == 'expected log output'

    def test_GetEditorLogStr_NoOutput_ReturnsCorrectly(self):
        self.mock_result.editor_log = None
        assert self.mock_result.get_editor_log_str() == '-- No editor log found --'

class TestResultPass(unittest.TestCase):

    def test_Create_ValidArgs_CorrectAttributes(self):
        mock_test_spec = mock.MagicMock()
        mock_output = mock.MagicMock()
        mock_editor_log = mock.MagicMock()

        mock_pass = editor_test.Result.Pass.create(mock_test_spec, mock_output, mock_editor_log)
        assert mock_pass.test_spec == mock_test_spec
        assert mock_pass.output == mock_output
        assert mock_pass.editor_log == mock_editor_log

    def test_Str_ValidString_ReturnsOutput(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_editor_log = mock.MagicMock()

        mock_pass = editor_test.Result.Pass.create(mock_test_spec, mock_output, mock_editor_log)
        assert mock_output in str(mock_pass)

class TestResultFail(unittest.TestCase):

    def test_Create_ValidArgs_CorrectAttributes(self):
        mock_test_spec = mock.MagicMock()
        mock_output = mock.MagicMock()
        mock_editor_log = mock.MagicMock()

        mock_pass = editor_test.Result.Fail.create(mock_test_spec, mock_output, mock_editor_log)
        assert mock_pass.test_spec == mock_test_spec
        assert mock_pass.output == mock_output
        assert mock_pass.editor_log == mock_editor_log

    def test_Str_ValidString_ReturnsOutput(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_editor_log = 'mock_editor_log'

        mock_pass = editor_test.Result.Fail.create(mock_test_spec, mock_output, mock_editor_log)
        assert mock_output in str(mock_pass)
        assert mock_editor_log in str(mock_pass)

class TestResultCrash(unittest.TestCase):

    def test_Create_ValidArgs_CorrectAttributes(self):
        mock_test_spec = mock.MagicMock()
        mock_output = mock.MagicMock()
        mock_editor_log = mock.MagicMock()
        mock_ret_code = mock.MagicMock()
        mock_stacktrace = mock.MagicMock()

        mock_pass = editor_test.Result.Crash.create(mock_test_spec, mock_output, mock_ret_code, mock_stacktrace,
                                                    mock_editor_log)
        assert mock_pass.test_spec == mock_test_spec
        assert mock_pass.output == mock_output
        assert mock_pass.editor_log == mock_editor_log
        assert mock_pass.ret_code == mock_ret_code
        assert mock_pass.stacktrace == mock_stacktrace

    def test_Str_ValidString_ReturnsOutput(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_editor_log = 'mock_editor_log'
        mock_return_code = 0
        mock_stacktrace = 'mock stacktrace'

        mock_pass = editor_test.Result.Crash.create(mock_test_spec, mock_output, mock_return_code, mock_stacktrace,
                                                    mock_editor_log)
        assert mock_stacktrace in str(mock_pass)
        assert mock_output in str(mock_pass)
        assert mock_editor_log in str(mock_pass)

    def test_Str_MissingStackTrace_ReturnsCorrectly(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_editor_log = 'mock_editor_log'
        mock_return_code = 0
        mock_stacktrace = None
        mock_pass = editor_test.Result.Crash.create(mock_test_spec, mock_output, mock_return_code, mock_stacktrace,
                                                    mock_editor_log)
        assert mock_output in str(mock_pass)
        assert mock_editor_log in str(mock_pass)

class TestResultTimeout(unittest.TestCase):

    def test_Create_ValidArgs_CorrectAttributes(self):
        mock_test_spec = mock.MagicMock()
        mock_output = mock.MagicMock()
        mock_editor_log = mock.MagicMock()
        mock_timeout = mock.MagicMock()

        mock_pass = editor_test.Result.Timeout.create(mock_test_spec, mock_output, mock_timeout, mock_editor_log)
        assert mock_pass.test_spec == mock_test_spec
        assert mock_pass.output == mock_output
        assert mock_pass.editor_log == mock_editor_log
        assert mock_pass.time_secs == mock_timeout

    def test_Str_ValidString_ReturnsOutput(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_editor_log = 'mock_editor_log'
        mock_timeout = 0

        mock_pass = editor_test.Result.Timeout.create(mock_test_spec, mock_output, mock_timeout, mock_editor_log)
        assert mock_output in str(mock_pass)
        assert mock_editor_log in str(mock_pass)

class TestResultUnknown(unittest.TestCase):

    def test_Create_ValidArgs_CorrectAttributes(self):
        mock_test_spec = mock.MagicMock()
        mock_output = mock.MagicMock()
        mock_editor_log = mock.MagicMock()
        mock_extra_info = mock.MagicMock()

        mock_pass = editor_test.Result.Unknown.create(mock_test_spec, mock_output, mock_extra_info, mock_editor_log)
        assert mock_pass.test_spec == mock_test_spec
        assert mock_pass.output == mock_output
        assert mock_pass.editor_log == mock_editor_log
        assert mock_pass.extra_info == mock_extra_info

    def test_Str_ValidString_ReturnsOutput(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_editor_log = 'mock_editor_log'
        mock_extra_info = 'mock extra info'

        mock_pass = editor_test.Result.Unknown.create(mock_test_spec, mock_output, mock_extra_info, mock_editor_log)
        assert mock_output in str(mock_pass)
        assert mock_editor_log in str(mock_pass)

class TestEditorTestSuite(unittest.TestCase):

    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_EditorTestData_ValidAP_TeardownAPOnce(self, mock_kill_processes):
        mock_editor_test_suite = editor_test.EditorTestSuite()
        mock_test_data_generator = mock_editor_test_suite._editor_test_data(mock.MagicMock())
        mock_asset_processor = mock.MagicMock()
        for test_data in mock_test_data_generator:
            test_data.asset_processor = mock_asset_processor
        mock_asset_processor.stop.assert_called_once_with(1)
        mock_asset_processor.teardown.assert_called()
        assert test_data.asset_processor is None
        mock_kill_processes.assert_called_once_with(include_asset_processor=True)

    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_EditorTestData_NoAP_NoTeardownAP(self, mock_kill_processes):
        mock_editor_test_suite = editor_test.EditorTestSuite()
        mock_test_data_generator = mock_editor_test_suite._editor_test_data(mock.MagicMock())
        for test_data in mock_test_data_generator:
            test_data.asset_processor = None
        mock_kill_processes.assert_called_once_with(include_asset_processor=False)

    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite.filter_session_shared_tests')
    def test_PytestCustomModifyItems_FunctionsMatch_AddsRunners(self, mock_filter_tests):
        class MockTestSuite(editor_test.EditorTestSuite):
            pass
        mock_func_1 = mock.MagicMock()
        mock_test = mock.MagicMock()
        runner_1 = editor_test.EditorTestSuite.Runner('mock_runner_1', mock_func_1, [mock_test])
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
        class MockTestSuite(editor_test.EditorTestSuite):
            pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_single_tests()
        assert len(tests) == 0

    def test_GetSingleTests_OneSingleTests_ReturnsOne(self):
        class MockTestSuite(editor_test.EditorTestSuite):
            class MockSingleTest(editor_test.EditorSingleTest):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_single_tests()
        assert len(tests) == 1
        assert tests[0].__name__ == "MockSingleTest"
        assert issubclass(tests[0], editor_test.EditorSingleTest)

    def test_GetSingleTests_AllTests_ReturnsOnlySingles(self):
        class MockTestSuite(editor_test.EditorTestSuite):
            class MockSingleTest(editor_test.EditorSingleTest):
                pass
            class MockAnotherSingleTest(editor_test.EditorSingleTest):
                pass
            class MockNotSingleTest(editor_test.EditorSharedTest):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_single_tests()
        assert len(tests) == 2
        for test in tests:
            assert issubclass(test, editor_test.EditorSingleTest)

    def test_GetSharedTests_NoSharedTests_EmptyList(self):
        class MockTestSuite(editor_test.EditorTestSuite):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_shared_tests()
        assert len(tests) == 0

    def test_GetSharedTests_OneSharedTests_ReturnsOne(self):
        class MockTestSuite(editor_test.EditorTestSuite):
            class MockSharedTest(editor_test.EditorSharedTest):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_shared_tests()
        assert len(tests) == 1
        assert tests[0].__name__ == 'MockSharedTest'
        assert issubclass(tests[0], editor_test.EditorSharedTest)

    def test_GetSharedTests_AllTests_ReturnsOnlyShared(self):
        class MockTestSuite(editor_test.EditorTestSuite):
            class MockSharedTest(editor_test.EditorSharedTest):
                pass
            class MockAnotherSharedTest(editor_test.EditorSharedTest):
                pass
            class MockNotSharedTest(editor_test.EditorSingleTest):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_shared_tests()
        assert len(tests) == 2
        for test in tests:
            assert issubclass(test, editor_test.EditorSharedTest)

    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite.filter_session_shared_tests')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite.get_shared_tests')
    def test_GetSessionSharedTests_Valid_CallsCorrectly(self, mock_get_shared_tests, mock_filter_session):
        editor_test.EditorTestSuite.get_session_shared_tests(mock.MagicMock())
        assert mock_get_shared_tests.called
        assert mock_filter_session.called

    @mock.patch('ly_test_tools.o3de.editor_test.skipping_pytest_runtest_setup', mock.MagicMock())
    def test_FilterSessionSharedTests_OneSharedTest_ReturnsOne(self):
        def mock_test():
            pass
        mock_test.originalname = 'mock_test'
        mock_test.__name__ = mock_test.originalname
        mock_session_items = [mock_test]
        mock_shared_tests = [mock_test]

        selected_tests = editor_test.EditorTestSuite.filter_session_shared_tests(mock_session_items, mock_shared_tests)
        assert selected_tests == mock_session_items
        assert len(selected_tests) == 1

    @mock.patch('ly_test_tools.o3de.editor_test.skipping_pytest_runtest_setup', mock.MagicMock())
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

        selected_tests = editor_test.EditorTestSuite.filter_session_shared_tests(mock_session_items, mock_shared_tests)
        assert selected_tests == mock_session_items

    @mock.patch('ly_test_tools.o3de.editor_test.skipping_pytest_runtest_setup')
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

        selected_tests = editor_test.EditorTestSuite.filter_session_shared_tests(mock_session_items, mock_shared_tests)
        assert selected_tests == [mock_test]

    @mock.patch('ly_test_tools.o3de.editor_test.skipping_pytest_runtest_setup', mock.MagicMock(side_effect=Exception))
    def test_FilterSessionSharedTests_ExceptionDuringSkipSetup_SkipsAddingTest(self):
        def mock_test():
            pass
        mock_test.originalname = 'mock_test'
        mock_test.__name__ = mock_test.originalname
        mock_session_items = [mock_test]
        mock_shared_tests = [mock_test]

        selected_tests = editor_test.EditorTestSuite.filter_session_shared_tests(mock_session_items, mock_shared_tests)
        assert len(selected_tests) == 0

    def test_FilterSharedTests_TrueParams_ReturnsTrueTests(self):
        mock_test = mock.MagicMock()
        mock_test.is_batchable = True
        mock_test.is_parallelizable = True
        mock_test_2 = mock.MagicMock()
        mock_test_2.is_batchable = False
        mock_test_2.is_parallelizable = False
        mock_shared_tests = [mock_test, mock_test_2]

        filtered_tests = editor_test.EditorTestSuite.filter_shared_tests(
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

        filtered_tests = editor_test.EditorTestSuite.filter_shared_tests(
            mock_shared_tests, is_batchable=False, is_parallelizable=False)
        assert filtered_tests == [mock_test_2]

class TestUtils(unittest.TestCase):

    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_PrepareAssetProcessor_APExists_StartsAP(self, mock_kill_processes):
        mock_test_suite = editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        mock_editor_data = mock.MagicMock()
        mock_ap = mock.MagicMock()
        mock_editor_data.asset_processor = mock_ap

        mock_test_suite._prepare_asset_processor(mock_workspace, mock_editor_data)
        assert mock_ap.start.called
        assert not mock_kill_processes.called

    @mock.patch('ly_test_tools.o3de.asset_processor.AssetProcessor.start')
    @mock.patch('ly_test_tools.environment.process_utils.process_exists')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_PrepareAssetProcessor_NoAP_KillAndCreateAP(self, mock_kill_processes, mock_proc_exists, mock_start):
        mock_test_suite = editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        mock_editor_data = mock.MagicMock()
        mock_editor_data.asset_processor = None
        mock_proc_exists.return_value = False

        mock_test_suite._prepare_asset_processor(mock_workspace, mock_editor_data)
        mock_kill_processes.assert_called_with(include_asset_processor=True)
        assert isinstance(mock_editor_data.asset_processor, ly_test_tools.o3de.asset_processor.AssetProcessor)
        assert mock_start.called

    @mock.patch('ly_test_tools.o3de.asset_processor.AssetProcessor.start')
    @mock.patch('ly_test_tools.environment.process_utils.process_exists')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_PrepareAssetProcessor_NoAPButProcExists_NoKill(self, mock_kill_processes, mock_proc_exists, mock_start):
        mock_test_suite = editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        mock_editor_data = mock.MagicMock()
        mock_editor_data.asset_processor = None
        mock_proc_exists.return_value = True

        mock_test_suite._prepare_asset_processor(mock_workspace, mock_editor_data)
        mock_kill_processes.assert_called_with(include_asset_processor=False)
        assert not mock_start.called
        assert mock_editor_data.asset_processor is None


    @mock.patch('ly_test_tools.o3de.asset_processor.AssetProcessor.start')
    @mock.patch('ly_test_tools.environment.process_utils.process_exists')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_PrepareAssetProcessor_NoAPButProcExists_NoKill(self, mock_kill_processes, mock_proc_exists, mock_start):
        mock_test_suite = editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        mock_editor_data = mock.MagicMock()
        mock_editor_data.asset_processor = None
        mock_proc_exists.return_value = True

        mock_test_suite._prepare_asset_processor(mock_workspace, mock_editor_data)
        mock_kill_processes.assert_called_with(include_asset_processor=False)
        assert not mock_start.called
        assert mock_editor_data.asset_processor is None

    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._prepare_asset_processor')
    def test_SetupEditorTest_ValidArgs_CallsCorrectly(self, mock_prepare_ap, mock_kill_processes):
        mock_test_suite = editor_test.EditorTestSuite()
        mock_editor = mock.MagicMock()
        mock_test_suite._setup_editor_test(mock_editor, mock.MagicMock(), mock.MagicMock())

        assert mock_editor.configure_settings.called
        assert mock_prepare_ap.called
        mock_kill_processes.assert_called_once_with(include_asset_processor=False)

    @mock.patch('ly_test_tools.o3de.editor_test.Result.Pass.create')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_module_filename')
    def test_GetResultsUsingOutput_ValidJsonSuccess_CreatesPassResult(self, mock_get_module, mock_create):
        mock_get_module.return_value = 'mock_module_name'
        mock_test_suite = editor_test.EditorTestSuite()
        mock_test = mock.MagicMock()
        mock_test.__name__ = 'mock_test_name'
        mock_test_list = [mock_test]
        mock_output = 'JSON_START(' \
                      '{"name": "mock_module_name", "output": "mock_std_out", "success": "mock_success_data"}' \
                      ')JSON_END'
        mock_pass = mock.MagicMock()
        mock_create.return_value = mock_pass

        results = mock_test_suite._get_results_using_output(mock_test_list, mock_output, '')
        assert mock_create.called
        assert len(results) == 1
        assert results[mock_test.__name__] == mock_pass

    @mock.patch('ly_test_tools.o3de.editor_test.Result.Fail.create')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_module_filename')
    def test_GetResultsUsingOutput_ValidJsonFail_CreatesFailResult(self, mock_get_module, mock_create):
        mock_get_module.return_value = 'mock_module_name'
        mock_test_suite = editor_test.EditorTestSuite()
        mock_test = mock.MagicMock()
        mock_test.__name__ = 'mock_test_name'
        mock_test_list = [mock_test]
        mock_output = 'JSON_START(' \
                      '{"name": "mock_module_name", "output": "mock_std_out", "failed": "mock_fail_data", "success": ""}' \
                      ')JSON_END'
        mock_fail = mock.MagicMock()
        mock_create.return_value = mock_fail

        results = mock_test_suite._get_results_using_output(mock_test_list, mock_output, '')
        assert mock_create.called
        assert len(results) == 1
        assert results[mock_test.__name__] == mock_fail

    @mock.patch('ly_test_tools.o3de.editor_test.Result.Unknown.create')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_module_filename')
    def test_GetResultsUsingOutput_ModuleNotInLog_CreatesUnknownResult(self, mock_get_module, mock_create):
        mock_get_module.return_value = 'different_module_name'
        mock_test_suite = editor_test.EditorTestSuite()
        mock_test = mock.MagicMock()
        mock_test.__name__ = 'mock_test_name'
        mock_test_list = [mock_test]
        mock_output = 'JSON_START(' \
                      '{"name": "mock_module_name", "output": "mock_std_out", "failed": "mock_fail_data"}' \
                      ')JSON_END'
        mock_unknown = mock.MagicMock()
        mock_create.return_value = mock_unknown

        results = mock_test_suite._get_results_using_output(mock_test_list, mock_output, '')
        assert mock_create.called
        assert len(results) == 1
        assert results[mock_test.__name__] == mock_unknown

    @mock.patch('ly_test_tools.o3de.editor_test.Result.Pass.create')
    @mock.patch('ly_test_tools.o3de.editor_test.Result.Fail.create')
    @mock.patch('ly_test_tools.o3de.editor_test.Result.Unknown.create')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_module_filename')
    def test_GetResultsUsingOutput_MultipleTests_CreatesCorrectResults(self, mock_get_module, mock_create_unknown,
                                                                       mock_create_fail, mock_create_pass):
        mock_get_module.side_effect = ['mock_module_name_pass', 'mock_module_name_fail', 'different_module_name']
        mock_test_suite = editor_test.EditorTestSuite()
        mock_test_pass = mock.MagicMock()
        mock_test_pass.__name__ = 'mock_test_name_pass'
        mock_test_fail = mock.MagicMock()
        mock_test_fail.__name__ = 'mock_test_name_fail'
        mock_test_unknown = mock.MagicMock()
        mock_test_unknown.__name__ = 'mock_test_name_unknown'
        mock_test_list = [mock_test_pass, mock_test_fail, mock_test_unknown]
        mock_output = 'JSON_START(' \
                      '{"name": "mock_module_name_pass", "output": "mock_std_out", "success": "mock_success_data"}' \
                      ')JSON_END' \
                      'JSON_START(' \
                      '{"name": "mock_module_name_fail", "output": "mock_std_out", "failed": "mock_fail_data", "success": ""}' \
                      ')JSON_END' \
                      'JSON_START(' \
                      '{"name": "mock_module_name_unknown", "output": "mock_std_out", "failed": "mock_fail_data", "success": ""}' \
                      ')JSON_END'
        mock_editor_log = 'JSON_START(' \
                          '{"name": "mock_module_name_pass"}' \
                          ')JSON_END' \
                          'JSON_START(' \
                          '{"name": "mock_module_name_fail"}' \
                          ')JSON_END'
        mock_unknown = mock.MagicMock()
        mock_pass = mock.MagicMock()
        mock_fail = mock.MagicMock()
        mock_create_unknown.return_value = mock_unknown
        mock_create_pass.return_value = mock_pass
        mock_create_fail.return_value = mock_fail

        results = mock_test_suite._get_results_using_output(mock_test_list, mock_output, mock_editor_log)
        mock_create_pass.assert_called_with(
            mock_test_pass, 'mock_std_out', 'JSON_START({"name": "mock_module_name_pass"})JSON_END')
        mock_create_fail.assert_called_with(
            mock_test_fail, 'mock_std_out', 'JSON_START({"name": "mock_module_name_fail"})JSON_END')
        mock_create_unknown.assert_called_with(
            mock_test_unknown, mock_output, "Couldn't find any test run information on stdout", mock_editor_log)
        assert len(results) == 3
        assert results[mock_test_pass.__name__] == mock_pass
        assert results[mock_test_fail.__name__] == mock_fail
        assert results[mock_test_unknown.__name__] == mock_unknown

    @mock.patch('builtins.print')
    def test_ReportResult_TestPassed_ReportsCorrectly(self, mock_print):
        mock_test_name = 'mock name'
        mock_pass = ly_test_tools.o3de.editor_test.Result.Pass()
        ly_test_tools.o3de.editor_test.EditorTestSuite._report_result(mock_test_name, mock_pass)
        mock_print.assert_called_with(f'Test {mock_test_name}:\nTest Passed\n------------\n|  Output  |\n------------\n'
                                      f'-- No output --\n')

    @mock.patch('pytest.fail')
    def test_ReportResult_TestFailed_FailsCorrectly(self, mock_pytest_fail):
        mock_fail = ly_test_tools.o3de.editor_test.Result.Fail()

        ly_test_tools.o3de.editor_test.EditorTestSuite._report_result('mock_test_name', mock_fail)
        mock_pytest_fail.assert_called_with('Test mock_test_name:\nTest FAILED\n------------\n|  Output  |'
                                            '\n------------\n-- No output --\n--------------\n| Editor log |'
                                            '\n--------------\n-- No editor log found --\n')

class TestRunningTests(unittest.TestCase):

    @mock.patch('ly_test_tools.o3de.editor_test.Result.Pass.create')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    def test_ExecEditorTest_TestSucceeds_ReturnsPass(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                     mock_retrieve_log, mock_retrieve_editor_log,
                                                     mock_get_output_results, mock_create):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        mock_editor = mock.MagicMock()
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_editor.get_returncode.return_value = 0
        mock_get_output_results.return_value = {}
        mock_pass = mock.MagicMock()
        mock_create.return_value = mock_pass

        results = mock_test_suite._exec_editor_test(mock.MagicMock(), mock_workspace, mock_editor, 0,
                                                    'mock_log_name', mock_test_spec, [])
        assert mock_cycle_crash.called
        assert mock_editor.start.called
        assert mock_create.called
        assert results == {mock_test_spec.__name__: mock_pass}

    @mock.patch('ly_test_tools.o3de.editor_test.Result.Fail.create')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    def test_ExecEditorTest_TestFails_ReturnsFail(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                     mock_retrieve_log, mock_retrieve_editor_log,
                                                     mock_get_output_results, mock_create):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        mock_editor = mock.MagicMock()
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_editor.get_returncode.return_value = 15
        mock_get_output_results.return_value = {}
        mock_fail = mock.MagicMock()
        mock_create.return_value = mock_fail

        results = mock_test_suite._exec_editor_test(mock.MagicMock(), mock_workspace, mock_editor, 0,
                                                    'mock_log_name', mock_test_spec, [])
        assert mock_cycle_crash.called
        assert mock_editor.start.called
        assert mock_create.called
        assert results == {mock_test_spec.__name__: mock_fail}

    @mock.patch('ly_test_tools.o3de.editor_test.Result.Crash.create')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_crash_output')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    def test_ExecEditorTest_TestCrashes_ReturnsCrash(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                     mock_retrieve_log, mock_retrieve_editor_log,
                                                     mock_get_output_results, mock_retrieve_crash, mock_create):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        mock_editor = mock.MagicMock()
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_editor.get_returncode.return_value = 1
        mock_get_output_results.return_value = {}
        mock_crash = mock.MagicMock()
        mock_create.return_value = mock_crash

        results = mock_test_suite._exec_editor_test(mock.MagicMock(), mock_workspace, mock_editor, 0,
                                                    'mock_log_name', mock_test_spec, [])
        assert mock_cycle_crash.call_count == 2
        assert mock_editor.start.called
        assert mock_retrieve_crash.called
        assert mock_create.called
        assert results == {mock_test_spec.__name__: mock_crash}

    @mock.patch('ly_test_tools.o3de.editor_test.Result.Timeout.create')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    def test_ExecEditorTest_TestTimeout_ReturnsTimeout(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                     mock_retrieve_log, mock_retrieve_editor_log,
                                                     mock_get_output_results, mock_create):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        mock_editor = mock.MagicMock()
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_editor.wait.side_effect = ly_test_tools.launchers.exceptions.WaitTimeoutError()
        mock_get_output_results.return_value = {}
        mock_timeout = mock.MagicMock()
        mock_create.return_value = mock_timeout

        results = mock_test_suite._exec_editor_test(mock.MagicMock(), mock_workspace, mock_editor, 0,
                                                    'mock_log_name', mock_test_spec, [])
        assert mock_cycle_crash.called
        assert mock_editor.start.called
        assert mock_editor.kill.called
        assert mock_create.called
        assert results == {mock_test_spec.__name__: mock_timeout}

    @mock.patch('ly_test_tools.o3de.editor_test.Result.Pass.create')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    def test_ExecEditorMultitest_AllTestsPass_ReturnsPasses(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                            mock_retrieve_log, mock_retrieve_editor_log, mock_create):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        mock_editor = mock.MagicMock()
        mock_editor.get_returncode.return_value = 0
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_test_spec_2 = mock.MagicMock()
        mock_test_spec_2.__name__ = 'mock_test_name_2'
        mock_test_spec_list = [mock_test_spec, mock_test_spec_2]
        mock_get_testcase_filepath.side_effect = ['mock_path', 'mock_path_2']
        mock_pass = mock.MagicMock()
        mock_pass_2 = mock.MagicMock()
        mock_create.side_effect = [mock_pass, mock_pass_2]

        results = mock_test_suite._exec_editor_multitest(mock.MagicMock(), mock_workspace, mock_editor, 0,
                                                         'mock_log_name', mock_test_spec_list, [])
        assert results == {mock_test_spec.__name__: mock_pass, mock_test_spec_2.__name__: mock_pass_2}
        assert mock_cycle_crash.called
        assert mock_create.call_count == 2

    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    def test_ExecEditorMultitest_OneFailure_CallsCorrectFunc(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                           mock_retrieve_log, mock_retrieve_editor_log, mock_get_results):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        mock_editor = mock.MagicMock()
        mock_editor.get_returncode.return_value = 15
        mock_test_spec = mock.MagicMock()
        mock_test_spec_2 = mock.MagicMock()
        mock_test_spec_list = [mock_test_spec, mock_test_spec_2]
        mock_get_testcase_filepath.side_effect = ['mock_path', 'mock_path_2']
        mock_get_results.return_value = {'mock_test_name': mock.MagicMock(), 'mock_test_name_2': mock.MagicMock()}

        results = mock_test_suite._exec_editor_multitest(mock.MagicMock(), mock_workspace, mock_editor, 0,
                                                         'mock_log_name', mock_test_spec_list, [])
        assert mock_cycle_crash.called
        assert mock_get_results.called

    @mock.patch('ly_test_tools.o3de.editor_test.Result.Crash.create')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_crash_output')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    def test_ExecEditorMultitest_OneCrash_ReportsOnUnknownResult(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                                 mock_retrieve_log, mock_retrieve_editor_log,
                                                                 mock_get_results, mock_retrieve_crash, mock_create):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        mock_editor = mock.MagicMock()
        mock_editor.get_returncode.return_value = 1
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_test_spec_2 = mock.MagicMock()
        mock_test_spec_2.__name__ = 'mock_test_name_2'
        mock_test_spec_list = [mock_test_spec, mock_test_spec_2]
        mock_unknown_result = ly_test_tools.o3de.editor_test.Result.Unknown()
        mock_unknown_result.test_spec = mock.MagicMock()
        mock_unknown_result.editor_log = mock.MagicMock()
        mock_get_testcase_filepath.side_effect = ['mock_path', 'mock_path_2']
        mock_get_results.return_value = {mock_test_spec.__name__: mock_unknown_result,
                                         mock_test_spec_2.__name__: mock.MagicMock()}
        mock_crash = mock.MagicMock()
        mock_create.return_value = mock_crash

        results = mock_test_suite._exec_editor_multitest(mock.MagicMock(), mock_workspace, mock_editor, 0,
                                                         'mock_log_name', mock_test_spec_list, [])
        assert mock_cycle_crash.call_count == 2
        assert mock_get_results.called
        assert results[mock_test_spec.__name__] == mock_crash

    @mock.patch('ly_test_tools.o3de.editor_test.Result.Crash.create')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_crash_output')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    def test_ExecEditorMultitest_ManyUnknown_ReportsUnknownResults(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                                   mock_retrieve_log, mock_retrieve_editor_log,
                                                                   mock_get_results, mock_retrieve_crash, mock_create):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        mock_editor = mock.MagicMock()
        mock_editor.get_returncode.return_value = 1
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_test_spec_2 = mock.MagicMock()
        mock_test_spec_2.__name__ = 'mock_test_name_2'
        mock_test_spec_list = [mock_test_spec, mock_test_spec_2]
        mock_unknown_result = ly_test_tools.o3de.editor_test.Result.Unknown()
        mock_unknown_result.__name__ = 'mock_test_name'
        mock_unknown_result.test_spec = mock.MagicMock()
        mock_unknown_result.test_spec.__name__ = 'mock_test_spec_name'
        mock_unknown_result.editor_log = mock.MagicMock()
        mock_get_testcase_filepath.side_effect = ['mock_path', 'mock_path_2']
        mock_get_results.return_value = {mock_test_spec.__name__: mock_unknown_result,
                                         mock_test_spec_2.__name__: mock_unknown_result}
        mock_crash = mock.MagicMock()
        mock_create.return_value = mock_crash

        results = mock_test_suite._exec_editor_multitest(mock.MagicMock(), mock_workspace, mock_editor, 0,
                                                         'mock_log_name', mock_test_spec_list, [])
        assert mock_cycle_crash.call_count == 2
        assert mock_get_results.called
        assert results[mock_test_spec.__name__] == mock_crash
        assert results[mock_test_spec_2.__name__].extra_info

    @mock.patch('ly_test_tools.o3de.editor_test.Result.Timeout.create')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    def test_ExecEditorMultitest_EditorTimeout_ReportsCorrectly(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                                mock_retrieve_log, mock_retrieve_editor_log,
                                                                mock_get_results, mock_create):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_workspace = mock.MagicMock()
        mock_editor = mock.MagicMock()
        mock_editor.wait.side_effect = ly_test_tools.launchers.exceptions.WaitTimeoutError()
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_test_spec_2 = mock.MagicMock()
        mock_test_spec_2.__name__ = 'mock_test_name_2'
        mock_test_spec_list = [mock_test_spec, mock_test_spec_2]
        mock_unknown_result = ly_test_tools.o3de.editor_test.Result.Unknown()
        mock_unknown_result.test_spec = mock.MagicMock()
        mock_unknown_result.test_spec.__name__ = 'mock_test_spec_name'
        mock_unknown_result.output = mock.MagicMock()
        mock_unknown_result.editor_log = mock.MagicMock()
        mock_get_testcase_filepath.side_effect = ['mock_path', 'mock_path_2']
        mock_get_results.return_value = {mock_test_spec.__name__: mock_unknown_result,
                                         mock_test_spec_2.__name__: mock_unknown_result}
        mock_timeout = mock.MagicMock()
        mock_create.return_value = mock_timeout

        results = mock_test_suite._exec_editor_multitest(mock.MagicMock(), mock_workspace, mock_editor, 0,
                                                         'mock_log_name', mock_test_spec_list, [])
        assert mock_cycle_crash.called
        assert mock_get_results.called
        assert results[mock_test_spec_2.__name__].extra_info
        assert results[mock_test_spec.__name__] == mock_timeout

    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._report_result')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._exec_editor_test')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._setup_editor_test')
    def test_RunSingleTest_ValidTest_ReportsResults(self, mock_setup_test, mock_exec_editor_test, mock_report_result):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_test_data = mock.MagicMock()
        mock_test_spec = mock.MagicMock()
        mock_result = mock.MagicMock()
        mock_test_name = 'mock_test_result'
        mock_exec_editor_test.return_value = {mock_test_name: mock_result}

        mock_test_suite._run_single_test(mock.MagicMock(), mock.MagicMock(), mock.MagicMock(), mock_test_data,
                                         mock_test_spec)

        assert mock_setup_test.called
        assert mock_exec_editor_test.called
        assert mock_test_data.results.update.called
        mock_report_result.assert_called_with(mock_test_name, mock_result)

    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._exec_editor_multitest')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._setup_editor_test')
    def test_RunBatchedTests_ValidTests_CallsCorrectly(self, mock_setup_test, mock_exec_multitest):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_test_data = mock.MagicMock()

        mock_test_suite._run_batched_tests(mock.MagicMock(), mock.MagicMock(), mock.MagicMock(), mock_test_data,
                                           mock.MagicMock(), [])

        assert mock_setup_test.called
        assert mock_exec_multitest.called
        assert mock_test_data.results.update.called

    @mock.patch('threading.Thread')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_number_parallel_editors')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._setup_editor_test')
    def test_RunParallelTests_TwoTestsAndEditors_TwoThreads(self, mock_setup_test, mock_get_num_editors, mock_thread):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_get_num_editors.return_value = 2
        mock_test_spec_list = [mock.MagicMock(), mock.MagicMock()]
        mock_test_data = mock.MagicMock()

        mock_test_suite._run_parallel_tests(mock.MagicMock(), mock.MagicMock(), mock.MagicMock(), mock_test_data,
                                            mock_test_spec_list, [])

        assert mock_setup_test.called
        assert mock_test_data.results.update.call_count == len(mock_test_spec_list)
        assert mock_thread.call_count == len(mock_test_spec_list)

    @mock.patch('threading.Thread')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_number_parallel_editors')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._setup_editor_test')
    def test_RunParallelTests_TenTestsAndTwoEditors_TenThreads(self, mock_setup_test, mock_get_num_editors, mock_thread):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_get_num_editors.return_value = 2
        mock_test_spec_list = []
        for i in range(10):
            mock_test_spec_list.append(mock.MagicMock())
        mock_test_data = mock.MagicMock()

        mock_test_suite._run_parallel_tests(mock.MagicMock(), mock.MagicMock(), mock.MagicMock(), mock_test_data,
                                            mock_test_spec_list, [])

        assert mock_setup_test.called
        assert mock_test_data.results.update.call_count == len(mock_test_spec_list)
        assert mock_thread.call_count == len(mock_test_spec_list)

    @mock.patch('threading.Thread')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_number_parallel_editors')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._setup_editor_test')
    def test_RunParallelTests_TenTestsAndThreeEditors_TenThreads(self, mock_setup_test, mock_get_num_editors,
                                                                 mock_thread):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_get_num_editors.return_value = 3
        mock_test_spec_list = []
        for i in range(10):
            mock_test_spec_list.append(mock.MagicMock())
        mock_test_data = mock.MagicMock()

        mock_test_suite._run_parallel_tests(mock.MagicMock(), mock.MagicMock(), mock.MagicMock(), mock_test_data,
                                            mock_test_spec_list, [])

        assert mock_setup_test.called
        assert mock_test_data.results.update.call_count == len(mock_test_spec_list)
        assert mock_thread.call_count == len(mock_test_spec_list)

    @mock.patch('threading.Thread')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_number_parallel_editors')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._setup_editor_test')
    def test_RunParallelBatchedTests_TwoTestsAndEditors_TwoThreads(self, mock_setup_test, mock_get_num_editors,
                                                                   mock_thread):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_get_num_editors.return_value = 2
        mock_test_spec_list = [mock.MagicMock(), mock.MagicMock()]
        mock_test_data = mock.MagicMock()

        mock_test_suite._run_parallel_batched_tests(mock.MagicMock(), mock.MagicMock(), mock.MagicMock(),
                                                    mock_test_data, mock_test_spec_list, [])

        assert mock_setup_test.called
        assert mock_test_data.results.update.call_count == len(mock_test_spec_list)
        assert mock_thread.call_count == len(mock_test_spec_list)

    @mock.patch('threading.Thread')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_number_parallel_editors')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._setup_editor_test')
    def test_RunParallelBatchedTests_TenTestsAndTwoEditors_2Threads(self, mock_setup_test, mock_get_num_editors,
                                                                      mock_thread):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_get_num_editors.return_value = 2
        mock_test_spec_list = []
        for i in range(10):
            mock_test_spec_list.append(mock.MagicMock())
        mock_test_data = mock.MagicMock()

        mock_test_suite._run_parallel_batched_tests(mock.MagicMock(), mock.MagicMock(), mock.MagicMock(),
                                                    mock_test_data, mock_test_spec_list, [])

        assert mock_setup_test.called
        assert mock_test_data.results.update.call_count == 2
        assert mock_thread.call_count == 2

    @mock.patch('threading.Thread')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._get_number_parallel_editors')
    @mock.patch('ly_test_tools.o3de.editor_test.EditorTestSuite._setup_editor_test')
    def test_RunParallelBatchedTests_TenTestsAndThreeEditors_ThreeThreads(self, mock_setup_test, mock_get_num_editors,
                                                                        mock_thread):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_get_num_editors.return_value = 3
        mock_test_spec_list = []
        for i in range(10):
            mock_test_spec_list.append(mock.MagicMock())
        mock_test_data = mock.MagicMock()

        mock_test_suite._run_parallel_batched_tests(mock.MagicMock(), mock.MagicMock(), mock.MagicMock(),
                                                    mock_test_data, mock_test_spec_list, [])

        assert mock_setup_test.called
        assert mock_test_data.results.update.call_count == 3
        assert mock_thread.call_count == 3

    def test_GetNumberParallelEditors_ConfigExists_ReturnsConfig(self):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_request = mock.MagicMock()
        mock_request.config.getoption.return_value = 1

        num_of_editors = mock_test_suite._get_number_parallel_editors(mock_request)
        assert num_of_editors == 1

    def test_GetNumberParallelEditors_ConfigNotExists_ReturnsDefault(self):
        mock_test_suite = ly_test_tools.o3de.editor_test.EditorTestSuite()
        mock_request = mock.MagicMock()
        mock_request.config.getoption.return_value = None

        num_of_editors = mock_test_suite._get_number_parallel_editors(mock_request)
        assert num_of_editors == mock_test_suite.get_number_parallel_editors()

@mock.patch('_pytest.python.Class.collect')
class TestEditorTestClass(unittest.TestCase):

    def setUp(self):
        mock_name = mock.MagicMock()
        mock_collector = mock.MagicMock()
        self.mock_test_class = ly_test_tools.o3de.editor_test.EditorTestSuite.EditorTestClass(mock_name, mock_collector)
        self.mock_test_class.obj = mock.MagicMock()
        single_1 = mock.MagicMock()
        single_1.__name__ = 'single_1_name'
        single_2 = mock.MagicMock()
        single_2.__name__ = 'single_2_name'
        self.mock_test_class.obj.get_single_tests.return_value = [single_1, single_2]
        batch_1 = mock.MagicMock()
        batch_1.__name__ = 'batch_1_name'
        batch_2 = mock.MagicMock()
        batch_2.__name__ = 'batch_2_name'
        parallel_1 = mock.MagicMock()
        parallel_1.__name__ = 'parallel_1_name'
        parallel_2 = mock.MagicMock()
        parallel_2.__name__ = 'parallel_2_name'
        both_1 = mock.MagicMock()
        both_1.__name__ = 'both_1_name'
        both_2 = mock.MagicMock()
        both_2.__name__ = 'both_2_name'
        self.mock_test_class.obj.filter_shared_tests.side_effect = [ [batch_1, batch_2],
                                                                     [parallel_1, parallel_2],
                                                                     [both_1, both_2] ]

    def test_Collect_NoParallelNoBatched_RunsAsSingleTests(self, mock_collect):
        self.mock_test_class.config.getoption.return_value = True
        self.mock_test_class.collect()
        assert self.mock_test_class.obj.single_1_name.__name__ == 'single_run'
        assert self.mock_test_class.obj.single_2_name.__name__ == 'single_run'
        assert self.mock_test_class.obj.batch_1_name.__name__ == 'single_run'
        assert self.mock_test_class.obj.batch_2_name.__name__ == 'single_run'
        assert self.mock_test_class.obj.parallel_1_name.__name__ == 'single_run'
        assert self.mock_test_class.obj.parallel_2_name.__name__ == 'single_run'
        assert self.mock_test_class.obj.both_1_name.__name__ == 'single_run'
        assert self.mock_test_class.obj.both_2_name.__name__ == 'single_run'

    def test_Collect_AllValidTests_RunsAsInteded(self, mock_collect):
        self.mock_test_class.config.getoption.return_value = False
        self.mock_test_class.collect()
        assert self.mock_test_class.obj.single_1_name.__name__ == 'single_run'
        assert self.mock_test_class.obj.single_2_name.__name__ == 'single_run'
        assert self.mock_test_class.obj.batch_1_name.__name__ == 'result'
        assert self.mock_test_class.obj.batch_2_name.__name__ == 'result'
        assert self.mock_test_class.obj.parallel_1_name.__name__ == 'result'
        assert self.mock_test_class.obj.parallel_2_name.__name__ == 'result'
        assert self.mock_test_class.obj.both_1_name.__name__ == 'result'
        assert self.mock_test_class.obj.both_2_name.__name__ == 'result'

    def test_Collect_AllValidTests_CallsCollect(self, mock_collect):
        self.mock_test_class.collect()
        assert mock_collect.called

    def test_Collect_NormalCollection_ReturnsFilteredRuns(self, mock_collect):
        mock_run = mock.MagicMock()
        mock_run.obj.marks = {"run_type": 'run_shared'}
        mock_run_2 = mock.MagicMock()
        mock_run_2.obj.marks = {"run_type": 'result'}
        mock_instance = mock.MagicMock()
        mock_instance.collect.return_value = [mock_run, mock_run_2]
        mock_collect.return_value = [mock_instance]

        collection = self.mock_test_class.collect()
        assert collection == [mock_run_2]

    def test_Collect_NormalRun_ReturnsRunners(self, mock_collect):
        self.mock_test_class.collect()
        runners =  self.mock_test_class.obj._runners

        assert runners[0].name == 'run_batched_tests'
        assert runners[1].name == 'run_parallel_tests'
        assert runners[2].name == 'run_parallel_batched_tests'

    def test_Collect_NormalCollection_StoresRunners(self, mock_collect):
        mock_runner = mock.MagicMock()
        mock_run = mock.MagicMock()
        mock_run.obj.marks = {"run_type": 'run_shared'}
        mock_run.function.marks = {"runner": mock_runner}
        mock_runner_2 = mock.MagicMock()
        mock_runner_2.result_pytestfuncs = []
        mock_run_2 = mock.MagicMock()
        mock_run_2.obj.marks = {"run_type": 'result'}
        mock_run_2.function.marks = {"runner": mock_runner_2}
        mock_instance = mock.MagicMock()
        mock_instance.collect.return_value = [mock_run, mock_run_2]
        mock_collect.return_value = [mock_instance]

        self.mock_test_class.collect()

        assert mock_runner.run_pytestfunc == mock_run
        assert mock_run_2 in mock_runner_2.result_pytestfuncs
