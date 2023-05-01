"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

This suite contains the tests for editor_test utilities.
"""
import pytest
import os
import sys
import importlib
import unittest.mock as mock

import ly_test_tools
import ly_test_tools.environment.process_utils as process_utils
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorTestSuite
from ly_test_tools.o3de.multi_test_framework import Result
from ly_test_tools.o3de.asset_processor import AssetProcessor


sys.path.append(os.path.dirname(os.path.abspath(__file__)))


def get_editor_launcher_platform():
    if ly_test_tools.WINDOWS:
        return "windows_editor"
    elif ly_test_tools.LINUX:
        return "linux_editor"
    else:
        return None


# Other plugins can create cross-object reference issues due these tests executing nonstandard pytest-within-pytest
@mock.patch('ly_test_tools._internal.pytest_plugin.terminal_report._add_ownership', mock.MagicMock())
@pytest.mark.parametrize("launcher_platform", [get_editor_launcher_platform()])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestEditorTest:

    args = []
    path = None

    @classmethod
    def setup_class(cls):
        # Copy all args except for the python interpreter and module file
        for arg in sys.argv:
            if not arg.endswith(".py"):
                TestEditorTest.args.append(arg)

        if "--build-directory" in TestEditorTest.args:
            # passed as two args, flag and value
            build_dir_arg_index = TestEditorTest.args.index("--build-directory")
            TestEditorTest.args[build_dir_arg_index + 1] = os.path.abspath(
                TestEditorTest.args[build_dir_arg_index + 1])
        else:
            # may instead be passed as one arg which includes equals-sign between flag and value
            build_dir_arg_index = 0
            for arg in TestEditorTest.args:
                if arg.startswith("--build-directory"):
                    first, second = arg.split("=", maxsplit=1)
                    TestEditorTest.args[build_dir_arg_index] = f'{first}={os.path.abspath(second)}'
                    break
                build_dir_arg_index += 1
        if build_dir_arg_index == len(TestEditorTest.args):
            raise ValueError(f"Must pass --build-directory argument in order to run this test. Found args: {TestEditorTest.args}")

        TestEditorTest.args.append("-s")
        TestEditorTest.path = os.path.dirname(os.path.abspath(__file__))
        cls._asset_processor = None

    # Custom cleanup
    def teardown_class(cls):
        if cls._asset_processor:
            cls._asset_processor.stop(1)
            cls._asset_processor.teardown()

    # Test runs #
    @classmethod
    def _run_single_test(cls, pytester, workspace, module_name):
        # Keep the AP open for all tests
        if cls._asset_processor is None:
            if not process_utils.process_exists("AssetProcessor", ignore_extensions=True):
                cls._asset_processor = AssetProcessor(workspace)
                cls._asset_processor.start()

        pytester.makepyfile(
            f"""
            import pytest
            import os
            import sys

            from ly_test_tools import LAUNCHERS
            from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorTestSuite

            @pytest.mark.SUITE_main
            @pytest.mark.parametrize("launcher_platform", ['{get_editor_launcher_platform()}'])
            @pytest.mark.parametrize("project", ["AutomatedTesting"])
            class TestAutomation(EditorTestSuite):
                class test_single(EditorSingleTest):
                    import {module_name} as test_module

            """)
        result = pytester.runpytest(*TestEditorTest.args)

        def get_class(module_name):
            class test_single(EditorSingleTest):
                test_module = importlib.import_module(module_name)
            return test_single

        output = "".join(result.outlines)
        extracted_results = EditorTestSuite._get_results_using_output([get_class(module_name)], output, output)
        extracted_result = next(iter(extracted_results.items()))
        return extracted_result[1], result

    @pytest.mark.skip(reason="Skipped for test efficiency, but keeping for reference.")
    def test_single_pass_test(self, request, workspace, launcher_platform, pytester):
        (extracted_result, result) = TestEditorTest._run_single_test(pytester, workspace, "EditorTest_That_Passes")
        result.assert_outcomes(passed=1)
        assert isinstance(extracted_result, Result.Pass)

    def test_single_fail_test(self, request, workspace, launcher_platform, pytester):
        (extracted_result, result) = TestEditorTest._run_single_test(pytester, workspace, "EditorTest_That_Fails")
        result.assert_outcomes(failed=1)
        assert isinstance(extracted_result, Result.Fail)

    def test_single_crash_test(self, request, workspace, launcher_platform, pytester):
        (extracted_result, result) = TestEditorTest._run_single_test(pytester, workspace, "EditorTest_That_Crashes")
        result.assert_outcomes(failed=1)
        # TODO: For the python 3.10.5 update on windows, a crashed test results in a fail, but on linux it results in an Unknown
        #       We will need to investigate the appropriate assertion here
        assert isinstance(extracted_result, Result.Unknown) or isinstance(extracted_result, Result.Fail)

    @classmethod
    def _run_shared_test(cls, pytester, workspace, module_class_code, extra_cmd_line=None):
        if not extra_cmd_line:
            extra_cmd_line = []

        # Keep the AP open for all tests
        if cls._asset_processor is None:
            if not process_utils.process_exists("AssetProcessor", ignore_extensions=True):
                cls._asset_processor = AssetProcessor(workspace)
                cls._asset_processor.start()

        pytester.makepyfile(
            f"""
            import pytest
            import os
            import sys

            from ly_test_tools import LAUNCHERS
            from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorTestSuite

            @pytest.mark.SUITE_main
            @pytest.mark.parametrize("launcher_platform", ['{get_editor_launcher_platform()}'])
            @pytest.mark.parametrize("project", ["AutomatedTesting"])
            class TestAutomation(EditorTestSuite):
            {module_class_code}
            """)
        result = pytester.runpytest(*TestEditorTest.args + extra_cmd_line)
        return result

# Here and throughout- the batch/parallel runner counts towards pytest's Passes, so we must include it in the asserts

    @pytest.mark.skip(reason="Skipped for test efficiency, but keeping for reference.")
    def test_batched_2_pass(self, request, workspace, launcher_platform, pytester):
        result = self._run_shared_test(pytester, workspace,
            """
                class test_pass(EditorSharedTest):
                    import EditorTest_That_Passes as test_module
                    is_parallelizable = False

                class test_2(EditorSharedTest):
                    import EditorTest_That_PassesToo as test_module
                    is_parallelizable = False
            """
        )
        # 2 Passes +1(batch runner)
        result.assert_outcomes(passed=3)

    @pytest.mark.skip(reason="Skipped for test efficiency, but keeping for reference.")
    def test_batched_1_pass_1_fail(self, request, workspace, launcher_platform, pytester):
        result = self._run_shared_test(pytester, workspace,
            """
                class test_pass(EditorSharedTest):
                    import EditorTest_That_Passes as test_module
                    is_parallelizable = False

                class test_fail(EditorSharedTest):
                    import EditorTest_That_Fails as test_module
                    is_parallelizable = False
            """
        )
        # 1 Fail, 1 Passes +1(batch runner)
        result.assert_outcomes(passed=2, failed=1)

    def test_batched_1_pass_1_fail_1_crash(self, request, workspace, launcher_platform, pytester):
        result = self._run_shared_test(pytester, workspace,
            """
                class test_pass(EditorSharedTest):
                    import EditorTest_That_Passes as test_module
                    is_parallelizable = False
                
                class test_fail(EditorSharedTest):
                    import EditorTest_That_Fails as test_module
                    is_parallelizable = False                
                
                class test_crash(EditorSharedTest):
                    import EditorTest_That_Crashes as test_module
                    is_parallelizable = False   
            """
        )
        # 2 Fail, 1 Passes + 1(batch runner)
        result.assert_outcomes(passed=2, failed=2)

    @pytest.mark.skip(reason="Skipped for test efficiency, but keeping for reference.")
    def test_parallel_2_pass(self, request, workspace, launcher_platform, pytester):
        result = self._run_shared_test(pytester, workspace,
            """
                class test_pass_1(EditorSharedTest):
                    import EditorTest_That_Passes as test_module
                    is_batchable = False

                class test_pass_2(EditorSharedTest):
                    import EditorTest_That_PassesToo as test_module
                    is_batchable = False
            """
        )
        # 2 Passes +1(parallel runner)
        result.assert_outcomes(passed=3)

    def test_parallel_1_pass_1_fail_1_crash(self, request, workspace, launcher_platform, pytester):
        result = self._run_shared_test(pytester, workspace,
            """
                class test_pass(EditorSharedTest):
                    import EditorTest_That_Passes as test_module
                    is_batchable = False
                
                class test_fail(EditorSharedTest):
                    import EditorTest_That_Fails as test_module
                    is_batchable = False                
                
                class test_crash(EditorSharedTest):
                    import EditorTest_That_Crashes as test_module
                    is_batchable = False   
            """
        )
        # 2 Fail, 1 Passes + 1(parallel runner)
        result.assert_outcomes(passed=2, failed=2)

    @pytest.mark.skip(reason="Skipped for test efficiency, but keeping for reference.")
    def test_parallel_batched_2_pass(self, request, workspace, launcher_platform, pytester):
        result = self._run_shared_test(pytester, workspace,
            """
                class test_pass_1(EditorSharedTest):
                    import EditorTest_That_Passes as test_module

                class test_pass_2(EditorSharedTest):
                    import EditorTest_That_PassesToo as test_module
            """
        )
        # 2 Passes +1(batched+parallel runner)
        result.assert_outcomes(passed=3)

    def test_parallel_batched_1_pass_1_fail_1_crash(self, request, workspace, launcher_platform, pytester):
        result = self._run_shared_test(pytester, workspace,
            """
                class test_pass(EditorSharedTest):
                    import EditorTest_That_Passes as test_module
                
                class test_fail(EditorSharedTest):
                    import EditorTest_That_Fails as test_module
                
                class test_crash(EditorSharedTest):
                    import EditorTest_That_Crashes as test_module
            """
        )
        # 2 Fail, 1 Passes + 1(batched+parallel runner)
        result.assert_outcomes(passed=2, failed=2)

    def test_selection_2_deselected_1_selected(self, request, workspace, launcher_platform, pytester):
        result = self._run_shared_test(pytester, workspace,
            """
                class test_pass(EditorSharedTest):
                    import EditorTest_That_Passes as test_module
                
                class test_fail(EditorSharedTest):
                    import EditorTest_That_Fails as test_module
                
                class test_crash(EditorSharedTest):
                    import EditorTest_That_Crashes as test_module
            """, extra_cmd_line=["-k", "fail"]
        )
        # 1 Fail + 1 Success(parallel runner)
        result.assert_outcomes(failed=1, passed=1)
        outcomes = result.parseoutcomes()
        deselected = outcomes.get("deselected")
        assert deselected == 2
