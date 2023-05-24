"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import unittest

import pytest
import unittest.mock as mock

import ly_test_tools
import ly_test_tools.launchers.exceptions
import ly_test_tools.o3de.multi_test_framework as multi_test_framework
from ly_test_tools.o3de.editor_test_utils import prepare_asset_processor
from ly_test_tools.launchers.platforms.linux.launcher import LinuxAtomToolsLauncher, LinuxEditor
from ly_test_tools.launchers.platforms.win.launcher import WinAtomToolsLauncher, WinEditor


pytestmark = pytest.mark.SUITE_smoke


class TestResultType(unittest.TestCase):

    class DummySubclass(multi_test_framework.Result.ResultType):
        def __str__(self):
            return None

    def setUp(self):
        self.mock_result = TestResultType.DummySubclass()

    def test_GetOutputStr_HasOutput_ReturnsCorrectly(self):
        self.mock_result.output = 'expected output'
        assert self.mock_result.get_output_str() == 'expected output'

    def test_GetOutputStr_NoOutput_ReturnsCorrectly(self):
        self.mock_result.output = None
        assert self.mock_result.get_output_str() == '-- No output --'

    def test_GetLogStr_HasOutput_ReturnsCorrectly(self):
        self.mock_result.log_output = 'expected log output'
        assert self.mock_result.get_log_output_str() == 'expected log output'

    def test_GetLogStr_NoOutput_ReturnsCorrectly(self):
        self.mock_result.log_output = None
        assert self.mock_result.get_log_output_str() == '-- No log found --'


class TestResultPass(unittest.TestCase):

    def test_Create_ValidArgs_CorrectAttributes(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_log_output = 'mock_log_output'

        under_test = multi_test_framework.Result.Pass(mock_test_spec, mock_output, mock_log_output)
        assert mock_test_spec == under_test.test_spec
        assert mock_output == under_test.output
        assert mock_log_output == under_test.log_output

    def test_Str_ValidPassString_ReturnsOutput(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_log_output = 'mock_log_output'

        under_test = multi_test_framework.Result.Pass(mock_test_spec, mock_output, mock_log_output)
        assert mock_output in str(under_test)


class TestResultFail(unittest.TestCase):

    def test_Create_ValidArgs_CorrectAttributes(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_log_output = 'mock_log_output'

        under_test = multi_test_framework.Result.Fail(mock_test_spec, mock_output, mock_log_output)
        assert mock_test_spec == under_test.test_spec
        assert mock_output == under_test.output
        assert mock_log_output == under_test.log_output

    def test_Str_ValidFailString_ReturnsOutput(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_log_output = 'mock_log_output'

        under_test = multi_test_framework.Result.Fail(mock_test_spec, mock_output, mock_log_output)
        assert mock_output in str(under_test)
        assert mock_log_output in str(under_test)


class TestResultCrash(unittest.TestCase):

    def test_Create_ValidArgs_CorrectAttributes(self):
        mock_test_spec = mock.MagicMock()
        mock_output = mock.MagicMock()
        mock_return_code = mock.MagicMock()
        mock_stacktrace = mock.MagicMock()
        mock_log_output = 'mock_log_output'

        under_test = multi_test_framework.Result.Crash(
            mock_test_spec, mock_output, mock_return_code, mock_stacktrace, mock_log_output)

        assert under_test.test_spec == mock_test_spec
        assert under_test.output == mock_output
        assert under_test.log_output == mock_log_output
        assert under_test.ret_code == mock_return_code
        assert under_test.stacktrace == mock_stacktrace

    def test_Str_ValidCrashString_ReturnsOutput(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_log_output = 'mock_log_output'
        mock_return_code = 0
        mock_stacktrace = 'mock stacktrace'

        under_test = multi_test_framework.Result.Crash(
            mock_test_spec, mock_output, mock_return_code, mock_stacktrace, mock_log_output)

        assert mock_stacktrace in str(under_test)
        assert mock_output in str(under_test)
        assert str(mock_return_code) in str(under_test)

    def test_Str_MissingStackTrace_ReturnsCorrectly(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_log_output = 'mock_log_output'
        mock_return_code = 0
        mock_stacktrace = None

        under_test = multi_test_framework.Result.Crash(
            mock_test_spec, mock_output, mock_return_code, mock_stacktrace, mock_log_output)

        assert mock_output in str(under_test)
        assert str(mock_return_code) in str(under_test)


class TestResultTimeout(unittest.TestCase):

    def test_Create_ValidArgs_CorrectAttributes(self):
        mock_test_spec = mock.MagicMock()
        mock_output = mock.MagicMock()
        mock_log_output = mock.MagicMock()
        mock_timeout = mock.MagicMock()

        under_test = multi_test_framework.Result.Timeout(mock_test_spec, mock_output, mock_timeout, mock_log_output)
        assert under_test.test_spec == mock_test_spec
        assert under_test.output == mock_output
        assert under_test.log_output == mock_log_output
        assert under_test.time_secs == mock_timeout

    def test_Str_ValidTimeoutString_ReturnsOutput(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_log_output = 'mock_log_output'
        mock_timeout = 0

        under_test = multi_test_framework.Result.Timeout(mock_test_spec, mock_output, mock_timeout, mock_log_output)
        assert mock_output in str(under_test)
        assert str(mock_timeout) in str(under_test)


class TestResultUnknown(unittest.TestCase):

    def test_Create_ValidArgs_CorrectAttributes(self):
        mock_test_spec = mock.MagicMock()
        mock_output = mock.MagicMock()
        mock_log_output = mock.MagicMock()
        mock_extra_info = mock.MagicMock()

        under_test = multi_test_framework.Result.Unknown(mock_test_spec, mock_output, mock_extra_info, mock_log_output)

        assert under_test.test_spec == mock_test_spec
        assert under_test.output == mock_output
        assert under_test.log_output == mock_log_output
        assert under_test.extra_info == mock_extra_info

    def test_Str_ValidUnknownString_ReturnsOutput(self):
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_extra_info = 'mock extra info'
        mock_log_output = 'mock_log_output'

        under_test = multi_test_framework.Result.Unknown(mock_test_spec, mock_output, mock_extra_info, mock_log_output)

        assert mock_output in str(under_test)
        assert mock_extra_info in str(under_test)


class TestMultiTestSuite(unittest.TestCase):

    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_TestData_ValidAP_TeardownAPOnce(self, mock_kill_processes):
        mock_abstract_test_suite = multi_test_framework.MultiTestSuite()
        mock_test_data_generator = mock_abstract_test_suite._collected_test_data(mock.MagicMock())
        mock_asset_processor = mock.MagicMock()
        for test_data in mock_test_data_generator:
            test_data.asset_processor = mock_asset_processor
        mock_asset_processor.teardown.assert_called()
        assert test_data.asset_processor is None
        mock_kill_processes.assert_called_once_with(include_asset_processor=True)

    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_TestData_NoAP_NoTeardownAP(self, mock_kill_processes):
        mock_abstract_test_suite = multi_test_framework.MultiTestSuite()
        mock_test_data_generator = mock_abstract_test_suite._collected_test_data(mock.MagicMock())
        for test_data in mock_test_data_generator:
            test_data.asset_processor = None
        mock_kill_processes.assert_called_once_with(include_asset_processor=False)

    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite.filter_session_shared_tests')
    def test_PytestCustomModifyItems_FunctionsMatch_AddsRunners(self, mock_filter_tests):
        class MockTestSuite(multi_test_framework.MultiTestSuite):
            pass
        mock_func_1 = mock.MagicMock()
        mock_test = mock.MagicMock()
        runner_1 = multi_test_framework.MultiTestSuite.Runner('mock_runner_1', mock_func_1, [mock_test])
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
        class MockTestSuite(multi_test_framework.MultiTestSuite):
            pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_single_tests()
        assert len(tests) == 0

    def test_GetSingleTests_OneSingleTests_ReturnsOne(self):
        class MockTestSuite(multi_test_framework.MultiTestSuite):
            class MockSingleTest(multi_test_framework.SingleTest):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_single_tests()
        assert len(tests) == 1
        assert tests[0].__name__ == "MockSingleTest"
        assert issubclass(tests[0], multi_test_framework.SingleTest)

    def test_GetSingleTests_AllTests_ReturnsOnlySingles(self):
        class MockTestSuite(multi_test_framework.MultiTestSuite):
            class MockSingleTest(multi_test_framework.SingleTest):
                pass
            class MockAnotherSingleTest(multi_test_framework.SingleTest):
                pass
            class MockNotSingleTest(multi_test_framework.SharedTest):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_single_tests()
        assert len(tests) == 2
        for test in tests:
            assert issubclass(test, multi_test_framework.SingleTest)

    def test_GetSharedTests_NoSharedTests_EmptyList(self):
        class MockTestSuite(multi_test_framework.MultiTestSuite):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_shared_tests()
        assert len(tests) == 0

    def test_GetSharedTests_OneSharedTests_ReturnsOne(self):
        class MockTestSuite(multi_test_framework.MultiTestSuite):
            class MockSharedTest(multi_test_framework.SharedTest):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_shared_tests()
        assert len(tests) == 1
        assert tests[0].__name__ == 'MockSharedTest'
        assert issubclass(tests[0], multi_test_framework.SharedTest)

    def test_GetSharedTests_AllTests_ReturnsOnlyShared(self):
        class MockTestSuite(multi_test_framework.MultiTestSuite):
            class MockSharedTest(multi_test_framework.SharedTest):
                pass
            class MockAnotherSharedTest(multi_test_framework.SharedTest):
                pass
            class MockNotSharedTest(multi_test_framework.SingleTest):
                pass
        mock_test_suite = MockTestSuite()
        tests = mock_test_suite.get_shared_tests()
        assert len(tests) == 2
        for test in tests:
            assert issubclass(test, multi_test_framework.SharedTest)

    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite.filter_session_shared_tests')
    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite.get_shared_tests')
    def test_GetSessionSharedTests_Valid_CallsCorrectly(self, mock_get_shared_tests, mock_filter_session):
        multi_test_framework.MultiTestSuite.get_session_shared_tests(mock.MagicMock())
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

        selected_tests = multi_test_framework.MultiTestSuite.filter_session_shared_tests(mock_session_items, mock_shared_tests)
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

        selected_tests = multi_test_framework.MultiTestSuite.filter_session_shared_tests(mock_session_items, mock_shared_tests)
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

        selected_tests = multi_test_framework.MultiTestSuite.filter_session_shared_tests(mock_session_items, mock_shared_tests)
        assert selected_tests == [mock_test]

    @mock.patch('ly_test_tools.o3de.multi_test_framework.skip_pytest_runtest_setup', mock.MagicMock(side_effect=Exception))
    def test_FilterSessionSharedTests_ExceptionDuringSkipSetup_SkipsAddingTest(self):
        def mock_test():
            pass
        mock_test.originalname = 'mock_test'
        mock_test.__name__ = mock_test.originalname
        mock_session_items = [mock_test]
        mock_shared_tests = [mock_test]

        selected_tests = multi_test_framework.MultiTestSuite.filter_session_shared_tests(mock_session_items, mock_shared_tests)
        assert len(selected_tests) == 0

    def test_FilterSharedTests_TrueParams_ReturnsTrueTests(self):
        mock_test = mock.MagicMock()
        mock_test.is_batchable = True
        mock_test.is_parallelizable = True
        mock_test_2 = mock.MagicMock()
        mock_test_2.is_batchable = False
        mock_test_2.is_parallelizable = False
        mock_shared_tests = [mock_test, mock_test_2]

        filtered_tests = multi_test_framework.MultiTestSuite.filter_shared_tests(
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

        filtered_tests = multi_test_framework.MultiTestSuite.filter_shared_tests(
            mock_shared_tests, is_batchable=False, is_parallelizable=False)
        assert filtered_tests == [mock_test_2]


class TestUtils(unittest.TestCase):

    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_PrepareAssetProcessor_APExists_StartsAP(self, mock_kill_processes):
        mock_workspace = mock.MagicMock()
        mock_collected_test_data = mock.MagicMock()
        mock_ap = mock.MagicMock()
        mock_collected_test_data.asset_processor = mock_ap

        prepare_asset_processor(mock_workspace, mock_collected_test_data)
        assert mock_collected_test_data.asset_processor.start.called
        assert not mock_kill_processes.called

    @mock.patch('ly_test_tools.o3de.asset_processor.AssetProcessor.start')
    @mock.patch('ly_test_tools.environment.process_utils.process_exists')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_PrepareAssetProcessor_NoAP_KillAndCreateAP(self, mock_kill_processes, mock_proc_exists, mock_start):
        mock_workspace = mock.MagicMock()
        mock_collected_test_data = mock.MagicMock()
        mock_collected_test_data.asset_processor = None
        mock_proc_exists.return_value = False

        prepare_asset_processor(mock_workspace, mock_collected_test_data)
        mock_kill_processes.assert_called_with(include_asset_processor=True)
        assert isinstance(mock_collected_test_data.asset_processor, ly_test_tools.o3de.asset_processor.AssetProcessor)
        assert mock_start.called

    @mock.patch('ly_test_tools.o3de.asset_processor.AssetProcessor.start')
    @mock.patch('ly_test_tools.environment.process_utils.process_exists')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_PrepareAssetProcessor_NoAPButProcExists_NoKill(self, mock_kill_processes, mock_proc_exists, mock_start):
        mock_workspace = mock.MagicMock()
        mock_collected_test_data = mock.MagicMock()
        mock_collected_test_data.asset_processor = None
        mock_proc_exists.return_value = True

        prepare_asset_processor(mock_workspace, mock_collected_test_data)
        mock_kill_processes.assert_called_with(include_asset_processor=False)
        assert not mock_start.called
        assert mock_collected_test_data.asset_processor is None

    @mock.patch('ly_test_tools.o3de.asset_processor.AssetProcessor.start')
    @mock.patch('ly_test_tools.environment.process_utils.process_exists')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    def test_PrepareAssetProcessor_NoAPButProcExists_NoKill(self, mock_kill_processes, mock_proc_exists, mock_start):
        mock_workspace = mock.MagicMock()
        mock_collected_test_data = mock.MagicMock()
        mock_collected_test_data.asset_processor = None
        mock_proc_exists.return_value = True

        prepare_asset_processor(mock_workspace, mock_collected_test_data)
        mock_kill_processes.assert_called_with(include_asset_processor=False)
        assert not mock_start.called
        assert mock_collected_test_data.asset_processor is None

    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_module_filename')
    def test_GetResultsUsingOutput_ValidJsonSuccess_CreatesPassResult(self, mock_get_module):
        mock_get_module.return_value = 'mock_module_name'
        mock_test_suite = multi_test_framework.MultiTestSuite()
        mock_test = mock.MagicMock()
        mock_test.__name__ = 'mock_test_name'
        mock_test_list = [mock_test]
        mock_output = 'JSON_START(' \
                      '{"name": "mock_module_name", "output": "mock_std_out", "success": "mock_success_data"}' \
                      ')JSON_END'

        results = mock_test_suite._get_results_using_output(mock_test_list, mock_output, '')
        assert len(results) == 1
        assert 'mock_test_name' in results.keys()
        assert isinstance(results['mock_test_name'], multi_test_framework.Result.Pass)

    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_module_filename')
    def test_GetResultsUsingOutput_ValidJsonFail_CreatesFailResult(self, mock_get_module):
        mock_get_module.return_value = 'mock_module_name'
        mock_test_suite = multi_test_framework.MultiTestSuite()
        mock_test = mock.MagicMock()
        mock_test.__name__ = 'mock_test_name'
        mock_test_list = [mock_test]
        mock_output = 'JSON_START(' \
                      '{"name": "mock_module_name", "output": "mock_std_out", "failed": "mock_fail_data", "success": ""}' \
                      ')JSON_END'

        results = mock_test_suite._get_results_using_output(mock_test_list, mock_output, '')
        assert len(results) == 1
        assert 'mock_test_name' in results.keys()
        assert isinstance(results['mock_test_name'], multi_test_framework.Result.Fail)

    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_module_filename')
    def test_GetResultsUsingOutput_ModuleNotInLog_CreatesUnknownResult(self, mock_get_module):
        mock_get_module.return_value = 'different_module_name'
        mock_test_suite = multi_test_framework.MultiTestSuite()
        mock_test = mock.MagicMock()
        mock_test.__name__ = 'mock_test_name'
        mock_test_list = [mock_test]
        mock_output = 'JSON_START(' \
                      '{"name": "mock_module_name", "output": "mock_std_out", "failed": "mock_fail_data"}' \
                      ')JSON_END'

        results = mock_test_suite._get_results_using_output(mock_test_list, mock_output, '')
        assert len(results) == 1
        assert 'mock_test_name' in results.keys()
        assert isinstance(results['mock_test_name'], multi_test_framework.Result.Unknown)

    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_module_filename')
    def test_GetResultsUsingOutput_MultipleTests_CreatesCorrectResults(self, mock_get_module):
        mock_get_module.side_effect = ['mock_module_name_pass', 'mock_module_name_fail', 'different_module_name']
        mock_test_suite = multi_test_framework.MultiTestSuite()
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
        mock_log_output = 'JSON_START(' \
                          '{"name": "mock_module_name_pass"}' \
                          ')JSON_END' \
                          'JSON_START(' \
                          '{"name": "mock_module_name_fail"}' \
                          ')JSON_END'

        results = mock_test_suite._get_results_using_output(mock_test_list, mock_output, mock_log_output)
        assert len(results) == 3
        assert 'mock_test_name_pass' in results.keys()
        assert 'mock_test_name_fail' in results.keys()
        assert 'mock_test_name_unknown' in results.keys()
        assert isinstance(results['mock_test_name_pass'], multi_test_framework.Result.Pass)
        assert isinstance(results['mock_test_name_fail'], multi_test_framework.Result.Fail)
        assert isinstance(results['mock_test_name_unknown'], multi_test_framework.Result.Unknown)

    @mock.patch('builtins.print')
    def test_ReportResult_TestPassed_ReportsCorrectly(self, mock_print):
        mock_test_name = 'mock name'
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_log_output = 'mock_log_output'
        mock_pass = ly_test_tools.o3de.multi_test_framework.Result.Pass(mock_test_spec, mock_output, mock_log_output)
        ly_test_tools.o3de.multi_test_framework.MultiTestSuite._report_result(mock_test_name, mock_pass)
        mock_print.assert_called_with(
            f'Test {mock_test_name}:\nTest Passed\n------------\n'
            f'|  Output  |\n------------\n{mock_output}\n')

    @mock.patch('pytest.fail')
    def test_ReportResult_TestFailed_FailsCorrectly(self, mock_pytest_fail):
        mock_test_name = 'mock name'
        mock_test_spec = mock.MagicMock()
        mock_output = 'mock_output'
        mock_log_output = 'mock_log_output'
        mock_fail = ly_test_tools.o3de.multi_test_framework.Result.Fail(mock_test_spec, mock_output, mock_log_output)

        ly_test_tools.o3de.multi_test_framework.MultiTestSuite._report_result(mock_test_name, mock_fail)
        mock_pytest_fail.assert_called_with(
            f"Test {mock_test_name}:\n"
            "Test FAILED\n"
            "------------\n"
            f"|  Output  |\n"
            "------------\n"
            f"{mock_output}\n"
            "----------------------------------------------------\n"
            f"| Application log  |\n"
            "----------------------------------------------------\n"
            f"{mock_log_output}\n")


class TestRunningTests(unittest.TestCase):

    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    @mock.patch('os.path.join', mock.MagicMock())
    def test_ExecSingleTest_TestSucceeds_ReturnsPass(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                     mock_retrieve_log, mock_retrieve_editor_log,
                                                     mock_get_output_results):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.engine_root.return_value = ""
        mock_executable = mock.MagicMock()
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_executable.get_returncode.return_value = 0
        mock_get_output_results.return_value = {}

        results = mock_test_suite._exec_single_test(
            mock.MagicMock(), mock_workspace, mock_executable, 0, 'mock_log_name', mock_test_spec, [])
        assert isinstance(results[mock_test_spec.__name__], multi_test_framework.Result.Pass)
        assert mock_cycle_crash.called
        assert mock_executable.start.called

    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    @mock.patch('os.path.join', mock.MagicMock())
    def test_ExecSingleTest_TestFails_ReturnsFail(self, mock_cycle_crash, mock_get_testcase_filepath, mock_retrieve_log,
                                                  mock_retrieve_editor_log, mock_get_output_results):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.engine_root.return_value = ""
        mock_executable = mock.MagicMock()
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_executable.get_returncode.return_value = 15
        mock_get_output_results.return_value = {}

        results = mock_test_suite._exec_single_test(
            mock.MagicMock(), mock_workspace, mock_executable, 0, 'mock_log_name', mock_test_spec, [])

        assert isinstance(results[mock_test_spec.__name__], multi_test_framework.Result.Fail)
        assert mock_cycle_crash.called
        assert mock_executable.start.called

    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_crash_output')
    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    @mock.patch('os.path.join', mock.MagicMock())
    @mock.patch('os.path.basename', mock.MagicMock())
    def test_ExecSingleTest_TestCrashes_ReturnsCrash(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                     mock_retrieve_log, mock_retrieve_editor_log,
                                                     mock_get_output_results, mock_retrieve_crash):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.engine_root.return_value = ""
        mock_executable = mock.MagicMock()
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_executable.get_returncode.return_value = 1
        mock_get_output_results.return_value = {}

        results = mock_test_suite._exec_single_test(
            mock.MagicMock(), mock_workspace, mock_executable, 0, 'mock_log_name', mock_test_spec, [])
        assert mock_cycle_crash.call_count == 2
        assert isinstance(results[mock_test_spec.__name__], multi_test_framework.Result.Crash)
        assert mock_executable.start.called
        assert mock_retrieve_crash.called
        # Save executable log output, crash log, and crash dmp
        assert mock_workspace.artifact_manager.save_artifact.call_count == 3

    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    def test_ExecSingleTest_TestTimeout_ReturnsTimeout(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                       mock_retrieve_log, mock_retrieve_editor_log,
                                                       mock_get_output_results):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.engine_root.return_value = ""
        mock_executable = mock.MagicMock()
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_executable.wait.side_effect = ly_test_tools.launchers.exceptions.WaitTimeoutError()
        mock_get_output_results.return_value = {}

        results = mock_test_suite._exec_single_test(
            mock.MagicMock(), mock_workspace, mock_executable, 0, 'mock_log_name', mock_test_spec, [])

        assert isinstance(results[mock_test_spec.__name__], multi_test_framework.Result.Timeout)
        assert mock_cycle_crash.called
        assert mock_executable.start.called
        assert mock_executable.stop.called

    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    @mock.patch('os.unlink', mock.MagicMock())
    @mock.patch('tempfile.NamedTemporaryFile', mock.MagicMock())
    @mock.patch('os.path.join', mock.MagicMock())
    @mock.patch('os.path.splitext', mock.MagicMock())
    @mock.patch('os.path.dirname', mock.MagicMock())
    def test_ExecMultitest_AllTestsPass_ReturnsPasses(self, mock_cycle_crash):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_workspace = mock.MagicMock()
        mock_artifact_manager = mock.MagicMock()
        mock_workspace.artifact_manager = mock_artifact_manager
        mock_workspace.paths.engine_root.return_value = ""
        mock_executable = mock.MagicMock()
        mock_executable.args = []
        mock_executable.get_returncode.return_value = 0
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_test_spec_2 = mock.MagicMock()
        mock_test_spec_2.__name__ = 'mock_test_name_2'
        mock_test_spec_list = [mock_test_spec, mock_test_spec_2]
        mock_path_exists = mock.MagicMock()
        mock_path_exists.return_value = True

        with mock.patch('builtins.open', mock.mock_open(read_data="")) as mock_file:
            results = mock_test_suite._exec_multitest(
                mock.MagicMock(), mock_workspace, mock_executable, 0, 'mock_log_name', mock_test_spec_list, [])

        assert len(results) == 2
        assert isinstance(results['mock_test_name'], multi_test_framework.Result.Pass)
        assert isinstance(results['mock_test_name_2'], multi_test_framework.Result.Pass)
        assert mock_cycle_crash.called

    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    @mock.patch('os.unlink', mock.MagicMock())
    @mock.patch('tempfile.NamedTemporaryFile', mock.MagicMock())
    @mock.patch('os.path.join', mock.MagicMock())
    @mock.patch('os.path.splitext', mock.MagicMock())
    @mock.patch('os.path.dirname', mock.MagicMock())
    def test_ExecMultitest_OneFailure_CallsCorrectFunc(
            self, mock_cycle_crash, mock_get_testcase_filepath, mock_get_results):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.engine_root.return_value = ""
        mock_executable = mock.MagicMock()
        mock_executable.get_returncode.return_value = 15
        mock_test_spec = mock.MagicMock()
        mock_test_spec_2 = mock.MagicMock()
        mock_test_spec_list = [mock_test_spec, mock_test_spec_2]
        mock_get_testcase_filepath.side_effect = ['mock_path', 'mock_path_2']
        mock_get_results.return_value = {'mock_test_name': mock.MagicMock(), 'mock_test_name_2': mock.MagicMock()}

        with mock.patch('builtins.open', mock.mock_open(read_data="")) as mock_file:
            results = mock_test_suite._exec_multitest(
                mock.MagicMock(), mock_workspace, mock_executable, 0, 'mock_log_name', mock_test_spec_list, [])

        assert len(results) == 2
        assert mock_cycle_crash.called
        assert mock_get_results.called

    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_crash_output')
    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    @mock.patch('os.unlink', mock.MagicMock())
    @mock.patch('tempfile.NamedTemporaryFile', mock.MagicMock())
    @mock.patch('os.path.join', mock.MagicMock())
    @mock.patch('os.path.basename', mock.MagicMock())
    @mock.patch('os.path.dirname', mock.MagicMock())
    def test_ExecMultitest_OneCrash_ReportsOnUnknownResult(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                           mock_retrieve_log, mock_retrieve_editor_log,
                                                           mock_get_results, mock_retrieve_crash):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.engine_root.return_value = ""
        mock_executable = mock.MagicMock()
        mock_executable.get_returncode.return_value = 1
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_test_spec_2 = mock.MagicMock()
        mock_test_spec_2.__name__ = 'mock_test_name_2'
        mock_test_spec_list = [mock_test_spec, mock_test_spec_2]
        mock_unknown_result = ly_test_tools.o3de.multi_test_framework.Result.Unknown(test_spec=None)
        mock_unknown_result.test_spec = mock.MagicMock()
        mock_unknown_result.editor_log = mock.MagicMock()
        mock_get_testcase_filepath.side_effect = ['mock_path', 'mock_path_2']
        mock_get_results.return_value = {mock_test_spec.__name__: mock_unknown_result,
                                         mock_test_spec_2.__name__: mock.MagicMock()}

        results = mock_test_suite._exec_multitest(
            mock.MagicMock(), mock_workspace, mock_executable, 0, 'mock_log_name', mock_test_spec_list, [])
        assert mock_cycle_crash.call_count == 2
        assert mock_get_results.called
        assert isinstance(results[mock_test_spec.__name__], multi_test_framework.Result.Crash)
        # Save executable log output, crash log, and crash dmp
        assert mock_workspace.artifact_manager.save_artifact.call_count == 3

    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_crash_output')
    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    @mock.patch('os.unlink', mock.MagicMock())
    @mock.patch('tempfile.NamedTemporaryFile', mock.MagicMock())
    @mock.patch('os.path.join', mock.MagicMock())
    @mock.patch('os.path.basename', mock.MagicMock())
    @mock.patch('os.path.dirname', mock.MagicMock())
    def test_ExecMultitest_ManyUnknown_ReportsUnknownResults(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                             mock_retrieve_log, mock_retrieve_editor_log,
                                                             mock_get_results, mock_retrieve_crash):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.engine_root.return_value = ""
        mock_executable = mock.MagicMock()
        mock_executable.get_returncode.return_value = 1
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_test_spec_2 = mock.MagicMock()
        mock_test_spec_2.__name__ = 'mock_test_name_2'
        mock_test_spec_list = [mock_test_spec, mock_test_spec_2]
        mock_unknown_result = ly_test_tools.o3de.multi_test_framework.Result.Unknown(test_spec=None)
        mock_unknown_result.__name__ = 'mock_test_name'
        mock_unknown_result.test_spec = mock.MagicMock()
        mock_unknown_result.test_spec.__name__ = 'mock_test_spec_name'
        mock_unknown_result.editor_log = mock.MagicMock()
        mock_get_testcase_filepath.side_effect = ['mock_path', 'mock_path_2']
        mock_get_results.return_value = {mock_test_spec.__name__: mock_unknown_result,
                                         mock_test_spec_2.__name__: mock_unknown_result}

        results = mock_test_suite._exec_multitest(
            mock.MagicMock(), mock_workspace, mock_executable, 0, 'mock_log_name', mock_test_spec_list, [])
        assert mock_cycle_crash.call_count == 2
        assert mock_get_results.called
        assert isinstance(results[mock_test_spec.__name__], multi_test_framework.Result.Crash)
        assert isinstance(results[mock_test_spec_2.__name__], multi_test_framework.Result.Unknown)
        assert results[mock_test_spec_2.__name__].extra_info, "Extra info missing from Unknown failure"

    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_results_using_output')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_editor_log_content')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.retrieve_log_path')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.get_testcase_module_filepath')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.cycle_crash_report')
    def test_ExecMultitest_EditorTimeout_ReportsCorrectly(self, mock_cycle_crash, mock_get_testcase_filepath,
                                                          mock_retrieve_log, mock_retrieve_editor_log, mock_get_results):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_workspace = mock.MagicMock()
        mock_workspace.paths.engine_root.return_value = ""
        mock_executable = mock.MagicMock()
        mock_executable.wait.side_effect = ly_test_tools.launchers.exceptions.WaitTimeoutError()
        mock_test_spec = mock.MagicMock()
        mock_test_spec.__name__ = 'mock_test_name'
        mock_test_spec_2 = mock.MagicMock()
        mock_test_spec_2.__name__ = 'mock_test_name_2'
        mock_test_spec_list = [mock_test_spec, mock_test_spec_2]
        mock_unknown_result = ly_test_tools.o3de.multi_test_framework.Result.Unknown(test_spec=None)
        mock_unknown_result.test_spec = mock.MagicMock()
        mock_unknown_result.test_spec.__name__ = 'mock_test_spec_name'
        mock_unknown_result.output = mock.MagicMock()
        mock_unknown_result.editor_log = mock.MagicMock()
        mock_get_testcase_filepath.side_effect = ['mock_path', 'mock_path_2']
        mock_get_results.return_value = {mock_test_spec.__name__: mock_unknown_result,
                                         mock_test_spec_2.__name__: mock_unknown_result}

        results = mock_test_suite._exec_multitest(
            mock.MagicMock(), mock_workspace, mock_executable, 0, 'mock_log_name', mock_test_spec_list, [])
        assert mock_cycle_crash.called
        assert mock_get_results.called
        assert isinstance(results[mock_test_spec.__name__], multi_test_framework.Result.Timeout)
        assert isinstance(results[mock_test_spec_2.__name__], multi_test_framework.Result.Unknown)
        assert results[mock_test_spec_2.__name__].extra_info, "Extra info missing from Unknown failure"

    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._report_result')
    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._exec_single_test')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.save_failed_asset_joblogs', mock.MagicMock())
    def test_RunSingleTest_ValidTest_ReportsResults(self, mock_exec_single_test, mock_report_result):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_request = mock.MagicMock()
        mock_workspace = mock.MagicMock()
        mock_collected_test_data = mock.MagicMock()
        mock_test_spec = mock.MagicMock()
        mock_result = mock.MagicMock()
        mock_test_name = 'mock_test_result'
        mock_exec_single_test.return_value = {mock_test_name: mock_result}

        mock_test_suite._run_single_test(
            mock_request, mock_workspace, mock_collected_test_data, mock_test_spec)

        assert mock_exec_single_test.called
        assert mock_collected_test_data.results.update.called
        mock_report_result.assert_called_with(mock_test_name, mock_result)

    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._test_reporting')
    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._exec_multitest')
    def test_RunBatchedTests_ValidTests_CallsCorrectly(self, mock_exec_multitest, mock_reporting):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_test_suite.atom_tools_executable_name = "dummy_executable"
        mock_request = mock.MagicMock()
        mock_workspace = mock.MagicMock()
        mock_collected_test_data = mock.MagicMock()
        mock_test_spec_list = mock.MagicMock()
        mock_extra_cmdline_args = [""]

        mock_test_suite._run_batched_tests(
            mock_request, mock_workspace, mock_collected_test_data, mock_test_spec_list, mock_extra_cmdline_args)

        assert mock_exec_multitest.called
        assert mock_reporting.called
        assert type(mock_test_suite.executable) in [WinAtomToolsLauncher, LinuxAtomToolsLauncher]
        assert mock_test_suite.executable.workspace == mock_workspace

    @mock.patch('threading.Thread')
    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_number_parallel_executables')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.save_failed_asset_joblogs', mock.MagicMock())
    def test_RunParallelTests_TwoTestsAndExecutables_TwoThreads(self, mock_get_number_executables, mock_thread):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_request = mock.MagicMock()
        mock_workspace = mock.MagicMock()
        mock_collected_test_data = mock.MagicMock()
        mock_extra_cmdline_args = [""]
        mock_get_number_executables.return_value = 2
        mock_test_spec_list = [mock.MagicMock(), mock.MagicMock()]

        mock_test_suite._run_parallel_tests(
            mock_request, mock_workspace, mock_collected_test_data, mock_test_spec_list, mock_extra_cmdline_args)

        assert mock_collected_test_data.results.update.call_count == len(mock_test_spec_list)
        assert mock_thread.call_count == len(mock_test_spec_list)

    @mock.patch('threading.Thread')
    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_number_parallel_executables')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.save_failed_asset_joblogs', mock.MagicMock())
    def test_RunParallelTests_TenTestsAndTwoExecutables_TenThreads(self, mock_get_number_executables, mock_thread):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_request = mock.MagicMock()
        mock_workspace = mock.MagicMock()
        mock_collected_test_data = mock.MagicMock()
        mock_extra_cmdline_args = [""]
        mock_get_number_executables.return_value = 2
        mock_test_spec_list = []
        for i in range(10):
            mock_test_spec_list.append(mock.MagicMock())

        mock_test_suite._run_parallel_tests(
            mock_request, mock_workspace, mock_collected_test_data, mock_test_spec_list, mock_extra_cmdline_args)

        assert mock_collected_test_data.results.update.call_count == len(mock_test_spec_list)
        assert mock_thread.call_count == len(mock_test_spec_list)

    @mock.patch('threading.Thread')
    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_number_parallel_executables')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.save_failed_asset_joblogs', mock.MagicMock())
    def test_RunParallelTests_TenTestsAndThreeEditors_TenThreads(self, mock_get_number_executables, mock_thread):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_request = mock.MagicMock()
        mock_workspace = mock.MagicMock()
        mock_collected_test_data = mock.MagicMock()
        mock_extra_cmdline_args = [""]
        mock_get_number_executables.return_value = 3
        mock_test_spec_list = []
        for i in range(10):
            mock_test_spec_list.append(mock.MagicMock())

        mock_test_suite._run_parallel_tests(
            mock_request, mock_workspace, mock_collected_test_data, mock_test_spec_list, mock_extra_cmdline_args)

        assert mock_collected_test_data.results.update.call_count == len(mock_test_spec_list)
        assert mock_thread.call_count == len(mock_test_spec_list)

    @mock.patch('threading.Thread')
    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_number_parallel_executables')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.save_failed_asset_joblogs', mock.MagicMock())
    def test_RunParallelBatchedTests_TwoTestsAndEditors_TwoThreads(self, mock_get_number_executables, mock_thread):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_request = mock.MagicMock()
        mock_workspace = mock.MagicMock()
        mock_collected_test_data = mock.MagicMock()
        mock_extra_cmdline_args = [""]
        mock_get_number_executables.return_value = 2
        mock_test_spec_list = [mock.MagicMock(), mock.MagicMock()]

        mock_test_suite._run_parallel_batched_tests(
            mock_request, mock_workspace, mock_collected_test_data, mock_test_spec_list, mock_extra_cmdline_args)

        assert mock_collected_test_data.results.update.call_count == len(mock_test_spec_list)
        assert mock_thread.call_count == len(mock_test_spec_list)

    @mock.patch('threading.Thread')
    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_number_parallel_executables')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.save_failed_asset_joblogs', mock.MagicMock())
    def test_RunParallelBatchedTests_TenTestsAndTwoEditors_2Threads(self, mock_get_number_executables, mock_thread):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_request = mock.MagicMock()
        mock_workspace = mock.MagicMock()
        mock_collected_test_data = mock.MagicMock()
        mock_extra_cmdline_args = [""]
        mock_get_number_executables.return_value = 2
        mock_test_spec_list = []
        for i in range(10):
            mock_test_spec_list.append(mock.MagicMock())

        mock_test_suite._run_parallel_batched_tests(
            mock_request, mock_workspace, mock_collected_test_data, mock_test_spec_list, mock_extra_cmdline_args)

        assert mock_collected_test_data.results.update.call_count == 2
        assert mock_thread.call_count == 2

    @mock.patch('threading.Thread')
    @mock.patch('ly_test_tools.o3de.multi_test_framework.MultiTestSuite._get_number_parallel_executables')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.save_failed_asset_joblogs', mock.MagicMock())
    def test_RunParallelBatchedTests_TenTestsAndThreeEditors_ThreeThreads(
            self, mock_get_number_executables, mock_thread):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_request = mock.MagicMock()
        mock_workspace = mock.MagicMock()
        mock_collected_test_data = mock.MagicMock()
        mock_extra_cmdline_args = [""]
        mock_get_number_executables.return_value = 3
        mock_test_spec_list = []
        for i in range(10):
            mock_test_spec_list.append(mock.MagicMock())

        mock_test_suite._run_parallel_batched_tests(
            mock_request, mock_workspace, mock_collected_test_data, mock_test_spec_list, mock_extra_cmdline_args)

        assert mock_collected_test_data.results.update.call_count == 3
        assert mock_thread.call_count == 3

    def test_GetNumberParallelEditors_ConfigExists_ReturnsConfig(self):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_request = mock.MagicMock()
        mock_request.config.getoption.return_value = 1

        number_of_executables = mock_test_suite._get_number_parallel_executables(mock_request)
        assert number_of_executables == 1

    def test_GetNumberParallelEditors_ConfigNotExists_ReturnsDefault(self):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_request = mock.MagicMock()
        mock_request.config.getoption.return_value = None

        number_of_executables = mock_test_suite._get_number_parallel_executables(mock_request)
        assert number_of_executables == mock_test_suite.get_number_parallel_executables()

    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.prepare_asset_processor')
    @mock.patch('ly_test_tools.launchers.launcher_helper.create_editor')
    @mock.patch('ly_test_tools.launchers.launcher_helper.create_atom_tools_launcher')
    def test_SetupTest_AtomExe_SetsExecutable(self, mock_create_atom, mock_create_editor, mock_prepare_ap, 
                                              mock_kill_processes):
        mock_workspace = mock.MagicMock()
        mock_exe = 'mock_atom_exe'
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_test_suite.atom_tools_executable_name = mock_exe
        mock_test_suite.executable = mock.MagicMock()

        mock_test_suite._setup_test(mock_workspace, mock.MagicMock())
        mock_create_atom.assert_called_once_with(mock_workspace, mock_exe)
        assert not mock_create_editor.called
        assert mock_prepare_ap.called
        assert mock_kill_processes.called
        assert mock_test_suite.executable.configure_settings.called

    @mock.patch('ly_test_tools.o3de.editor_test_utils.kill_all_ly_processes')
    @mock.patch('ly_test_tools.o3de.editor_test_utils.prepare_asset_processor')
    @mock.patch('ly_test_tools.launchers.launcher_helper.create_editor')
    @mock.patch('ly_test_tools.launchers.launcher_helper.create_atom_tools_launcher')
    def test_SetupTest_EditorExe_SetsExecutable(self, mock_create_atom, mock_create_editor, mock_prepare_ap, 
                                                mock_kill_processes):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_test_suite.atom_tools_executable_name = None
        mock_test_suite.executable = mock.MagicMock()

        mock_test_suite._setup_test(mock.MagicMock(), mock.MagicMock())
        assert not mock_create_atom.called
        assert mock_create_editor.called
        assert mock_prepare_ap.called
        assert mock_kill_processes.called
        assert mock_test_suite.executable.configure_settings.called

    @mock.patch('ly_test_tools.o3de.editor_test_utils.save_failed_asset_joblogs')
    def test_TestReporting_AllPassResults_NoSaveLogs(self, mock_save_logs):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_collected_test_data = mock.MagicMock()
        mock_pass = multi_test_framework.Result.Pass(mock.MagicMock(), mock.MagicMock(), mock.MagicMock())
        mock_results = [mock_pass]

        mock_test_suite._test_reporting(mock_collected_test_data, mock_results, mock.MagicMock(), mock.MagicMock())
        assert not mock_save_logs.called
        assert mock_collected_test_data.results.update.called

    @mock.patch('ly_test_tools.o3de.editor_test_utils.save_failed_asset_joblogs')
    def test_TestReporting_UnknownResults_SaveLogs(self, mock_save_logs):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_collected_test_data = mock.MagicMock()
        mock_results = [None]

        mock_test_suite._test_reporting(mock_collected_test_data, mock_results, mock.MagicMock(), mock.MagicMock())
        assert mock_save_logs.called
        assert mock_collected_test_data.results.update.called

    @mock.patch('ly_test_tools.o3de.editor_test_utils.save_failed_asset_joblogs')
    def test_TestReporting_BothResults_SaveLogs(self, mock_save_logs):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_collected_test_data = mock.MagicMock()
        mock_pass = multi_test_framework.Result.Pass(mock.MagicMock(), mock.MagicMock(), mock.MagicMock())
        mock_results = [mock_pass, None]

        mock_test_suite._test_reporting(mock_collected_test_data, mock_results, mock.MagicMock(), mock.MagicMock())
        assert mock_save_logs.called
        assert mock_collected_test_data.results.update.called
        
    @mock.patch('ly_test_tools.o3de.editor_test_utils.save_failed_asset_joblogs')
    def test_TestReporting_ManyResults_SaveLogs(self, mock_save_logs):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_collected_test_data = mock.MagicMock()
        mock_pass = multi_test_framework.Result.Pass(mock.MagicMock(), mock.MagicMock(), mock.MagicMock())
        mock_fail = multi_test_framework.Result.Fail(mock.MagicMock(), mock.MagicMock(), mock.MagicMock())
        mock_results = [mock_pass, mock_fail, None]

        mock_test_suite._test_reporting(mock_collected_test_data, mock_results, mock.MagicMock(), mock.MagicMock())
        assert mock_save_logs.called
        assert mock_collected_test_data.results.update.called
 
    def test_SetupCmdlineArgs_EditorExe_EnablesPrefab(self):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_executable = WinEditor(mock.MagicMock(), [])
        mock_test_spec_list = mock.MagicMock()

        under_test = mock_test_suite._setup_cmdline_args(None, mock_executable, mock_test_spec_list, mock.MagicMock())
        assert "--regset=/Amazon/Preferences/EnablePrefabSystem=true" in under_test
        assert "-rhi=null" in under_test
        assert "--attach-debugger" not in under_test
        assert "--wait-for-debugger" not in under_test

    def test_SetupCmdlineArgs_AtomExe_NotEnablesPrefab(self):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_executable = WinAtomToolsLauncher(mock.MagicMock(), [])
        mock_test_spec_list = mock.MagicMock()

        under_test = mock_test_suite._setup_cmdline_args(None, mock_executable, mock_test_spec_list, mock.MagicMock())
        assert "--regset=/Amazon/Preferences/EnablePrefabSystem=true" not in under_test
        assert "-rhi=null" in under_test
        assert "--attach-debugger" not in under_test
        assert "--wait-for-debugger" not in under_test

    def test_SetupCmdlineArgs_Debugger_EnablesDebugger(self):
        mock_test_suite = ly_test_tools.o3de.multi_test_framework.MultiTestSuite()
        mock_executable = WinAtomToolsLauncher(mock.MagicMock(), [])
        mock_test_spec = mock.MagicMock()
        mock_test_spec.attach_debugger = True
        mock_test_spec.wait_for_debugger = True
        mock_test_spec_list = [mock_test_spec]

        under_test = mock_test_suite._setup_cmdline_args(None, mock_executable, mock_test_spec_list, mock.MagicMock())
        assert "--regset=/Amazon/Preferences/EnablePrefabSystem=true" not in under_test
        assert "-rhi=null" in under_test
        assert "--attach-debugger" in under_test
        assert "--wait-for-debugger" in under_test

@mock.patch('_pytest.python.Class.collect')
class TestMultiTestCollector(unittest.TestCase):

    def setUp(self):
        mock_name = mock.MagicMock()
        mock_collector = mock.MagicMock()
        self.mock_test_class = ly_test_tools.o3de.multi_test_framework.MultiTestSuite.MultiTestCollector.from_parent(
            name=mock_name, parent=mock_collector)
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
        mock_collect.return_value = [mock_run_2]

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
        mock_collect.return_value = [mock_run_2]

        self.mock_test_class.collect()

        assert mock_run_2 in mock_runner_2.result_pytestfuncs

    @mock.patch('ly_test_tools.o3de.multi_test_framework.isinstance', mock.MagicMock())
    @mock.patch('ly_test_tools.o3de.multi_test_framework.issubclass', mock.MagicMock())
    def test_MakeSingleRun_SingleRun_SetupTeardown(self, mock_collect):
        mock_inner_test_spec = mock.MagicMock()
        mock_self = mock.MagicMock()

        mock_single_run_func = multi_test_framework.MultiTestSuite.MultiTestCollector._make_single_run(
            mock_inner_test_spec)
        mock_single_run_func(mock_self, mock.MagicMock(), mock.MagicMock(), mock.MagicMock(), mock.MagicMock())

        mock_inner_test_spec.setup.assert_called_once()
        mock_inner_test_spec.teardown.assert_called_once()
        mock_self._run_single_test.assert_called_once()

    @mock.patch('ly_test_tools.o3de.multi_test_framework.isinstance', mock.MagicMock(return_value=False))
    @mock.patch('ly_test_tools.o3de.multi_test_framework.issubclass', mock.MagicMock(return_value=False))
    def test_MakeSingleRun_NoSingleRun_NoSetupTeardown(self, mock_collect):
        mock_inner_test_spec = mock.MagicMock()
        mock_self = mock.MagicMock()

        mock_single_run_func = multi_test_framework.MultiTestSuite.MultiTestCollector._make_single_run(
            mock_inner_test_spec)
        mock_single_run_func(mock_self, mock.MagicMock(), mock.MagicMock(), mock.MagicMock(), mock.MagicMock())

        assert not mock_inner_test_spec.setup.called
        assert not mock_inner_test_spec.teardown.called
        mock_self._run_single_test.assert_called_once()

    def test_CreateRunner_HappyPath_SetsRunnersProperly(self, mock_collect):
        mock_runner_name = 'mock_runner_name'
        mock_function = mock.MagicMock()
        mock_shared_test_spec_1 = mock.MagicMock()
        mock_shared_test_spec_1.__name__ = 'mock_shared_test_spec_1'
        mock_shared_test_spec_2 = mock.MagicMock()
        mock_shared_test_spec_2.__name__ = 'mock_shared_test_spec_2'
        mock_tests = [mock_shared_test_spec_1, mock_shared_test_spec_2]
        mock_obj = mock.MagicMock()

        multi_test_framework.MultiTestSuite.MultiTestCollector._create_runner(mock_runner_name, mock_function,
                                                                              mock_tests, mock_obj)

        assert hasattr(mock_obj, mock_runner_name)
        assert hasattr(mock_obj, mock_shared_test_spec_1.__name__)
        assert hasattr(mock_obj, mock_shared_test_spec_2.__name__)

    def test_CreateRunner_MakeSharedRun_CallsAttributeFunc(self, mock_collect):
        mock_runner_name = 'mock_runner_name'
        mock_obj = mock.MagicMock()
        mock_function = mock.MagicMock()
        mock_function.__name__ = 'mock_func_name'
        mock_tests = []
        mock_request = mock.MagicMock()
        mock_workspace = mock.MagicMock()
        mock_collected_test_data = mock.MagicMock()

        multi_test_framework.MultiTestSuite.MultiTestCollector._create_runner(mock_runner_name, mock_function,
                                                                              mock_tests, mock_obj)
        getattr(mock_obj, mock_runner_name)(mock_obj, mock_request, mock_workspace, mock_collected_test_data,
                                            mock.MagicMock())

        mock_obj.mock_func_name.assert_called_once_with(mock_request, mock_workspace, mock_collected_test_data,
                                                        mock_tests)

    def test_CreateRunner_MakeResultsRun_CallsAttributeFunc(self, mock_collect):
        mock_runner_name = 'mock_runner_name'
        mock_obj = mock.MagicMock()
        mock_function = mock.MagicMock()
        mock_shared_test_spec_1 = mock.MagicMock()
        mock_shared_test_spec_1.__name__ = 'mock_shared_test_spec_name_1'
        mock_shared_test_spec_2 = mock.MagicMock()
        mock_shared_test_spec_2.__name__ = 'mock_shared_test_spec_name_2'
        mock_tests = [mock_shared_test_spec_1, mock_shared_test_spec_2]
        mock_collected_test_data = mock.MagicMock()
        mock_result_1 = mock.MagicMock()
        mock_result_2 = mock.MagicMock()
        mock_collected_test_data.results = {mock_shared_test_spec_1.__name__: mock_result_1,
                                            mock_shared_test_spec_2.__name__: mock_result_2}

        multi_test_framework.MultiTestSuite.MultiTestCollector._create_runner(mock_runner_name, mock_function,
                                                                              mock_tests, mock_obj)
        getattr(mock_obj, mock_shared_test_spec_1.__name__)(mock_obj, mock.MagicMock(), mock.MagicMock(),
                                                            mock_collected_test_data, mock.MagicMock())
        mock_obj._report_result.assert_called_with(mock_shared_test_spec_1.__name__, mock_result_1)
        getattr(mock_obj, mock_shared_test_spec_2.__name__)(mock_obj, mock.MagicMock(), mock.MagicMock(),
                                                            mock_collected_test_data, mock.MagicMock())
        mock_obj._report_result.assert_called_with(mock_shared_test_spec_2.__name__, mock_result_2)
