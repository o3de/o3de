"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
This suite contains the tests for editor_test utilities.
"""

import pytest
import os
import sys
import importlib
import re

import ly_test_tools
from ly_test_tools import LAUNCHERS

sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorTestSuite, Result
from ly_test_tools.o3de.asset_processor import AssetProcessor
import ly_test_tools.environment.process_utils as process_utils

import argparse, sys

def get_editor_launcher_platform():
    if ly_test_tools.WINDOWS:
        return "windows_editor"
    elif ly_test_tools.LINUX:
        return "linux_editor"
    else:
        return None

@pytest.mark.parametrize("launcher_platform", [get_editor_launcher_platform()])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestEditorTest:
    
    args = None 
    path = None
    @classmethod
    def setup_class(cls):
        TestEditorTest.args = sys.argv.copy()
        build_dir_arg_index = -1
        try:
            build_dir_arg_index = TestEditorTest.args.index("--build-directory")
        except ValueError as ex:
            raise ValueError("Must pass --build-directory argument in order to run this test")

        TestEditorTest.args[build_dir_arg_index+1] = os.path.abspath(TestEditorTest.args[build_dir_arg_index+1])
        TestEditorTest.args.append("-s")
        TestEditorTest.path = os.path.dirname(os.path.abspath(__file__))
        cls._asset_processor = None

    def teardown_class(cls):
        if cls._asset_processor:
            cls._asset_processor.stop(1)
            cls._asset_processor.teardown()

    # Test runs #
    @classmethod
    def _run_single_test(cls, testdir, workspace, module_name):
        if cls._asset_processor is None:
            if not process_utils.process_exists("AssetProcessor", ignore_extensions=True):
                cls._asset_processor = AssetProcessor(workspace)
                cls._asset_processor.start()

        testdir.makepyfile(
            f"""
            import pytest
            import os
            import sys

            from ly_test_tools import LAUNCHERS
            from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorTestSuite

            @pytest.mark.SUITE_main
            @pytest.mark.parametrize("launcher_platform", [{get_editor_launcher_platform()}])
            @pytest.mark.parametrize("project", ["AutomatedTesting"])
            class TestAutomation(EditorTestSuite):
                class test_single(EditorSingleTest):
                    import {module_name} as test_module

            """)
        result = testdir.runpytest(*TestEditorTest.args[2:])

        def get_class(module_name):
            class test_single(EditorSingleTest):
                test_module = importlib.import_module(module_name)
            return test_single

        output = "".join(result.outlines)
        extracted_results = EditorTestSuite._get_results_using_output([get_class(module_name)], output, output)
        extracted_result = next(iter(extracted_results.items()))
        return (extracted_result[1], result)
    
    def test_single_passing_test(self, request, workspace, launcher_platform, testdir):
        (extracted_result, result) = TestEditorTest._run_single_test(testdir, workspace, "EditorTest_That_Passes")
        result.assert_outcomes(passed=1)
        assert isinstance(extracted_result, Result.Pass)

    def test_single_failing_test(self, request, workspace, launcher_platform, testdir):
        (extracted_result, result) = TestEditorTest._run_single_test(testdir, workspace, "EditorTest_That_Fails")        
        result.assert_outcomes(failed=1)
        assert isinstance(extracted_result, Result.Fail)

    def test_single_crashing_test(self, request, workspace, launcher_platform, testdir):
        (extracted_result, result) = TestEditorTest._run_single_test(testdir, workspace, "EditorTest_That_Crashes")
        result.assert_outcomes(failed=1)
        assert isinstance(extracted_result, Result.Unknown)
    
    @classmethod
    def _run_shared_test(cls, testdir, module_class_code, extra_cmd_line=None):
        if not extra_cmd_line:
            extra_cmd_line = []

        if cls._asset_processor is None:
            if not process_utils.process_exists("AssetProcessor", ignore_extensions=True):
                cls._asset_processor = AssetProcessor(workspace)
                cls._asset_processor.start()

        testdir.makepyfile(
            f"""
            import pytest
            import os
            import sys

            from ly_test_tools import LAUNCHERS
            from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorTestSuite

            @pytest.mark.SUITE_main
            @pytest.mark.parametrize("launcher_platform", [{get_editor_launcher_platform()}])
            @pytest.mark.parametrize("project", ["AutomatedTesting"])
            class TestAutomation(EditorTestSuite):
            {module_class_code}
            """)
        result = testdir.runpytest(*TestEditorTest.args[2:] + extra_cmd_line)
        return result

    def test_batched_two_passing(self, request, workspace, launcher_platform, testdir):
        result = self._run_shared_test(testdir,
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

    def test_batched_one_pass_one_fail(self, request, workspace, launcher_platform, testdir):
        result = self._run_shared_test(testdir,
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
    
    def test_batched_one_pass_one_fail_one_crash(self, request, workspace, launcher_platform, testdir):
        result = self._run_shared_test(testdir,
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

    def test_parallel_two_passing(self, request, workspace, launcher_platform, testdir):
        result = self._run_shared_test(testdir,
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
    
    def test_parallel_one_passing_one_failing_one_crashing(self, request, workspace, launcher_platform, testdir):
        result = self._run_shared_test(testdir,
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

    def test_parallel_batched_two_passing(self, request, workspace, launcher_platform, testdir):
        result = self._run_shared_test(testdir,
            """
                class test_pass_1(EditorSharedTest):
                    import EditorTest_That_Passes as test_module
                
                class test_pass_2(EditorSharedTest):
                    import EditorTest_That_PassesToo as test_module
            """
        )
        # 2 Passes +1(batched+parallel runner)
        result.assert_outcomes(passed=3)
    
    def test_parallel_batched_one_passing_one_failing_one_crashing(self, request, workspace, launcher_platform, testdir):
        result = self._run_shared_test(testdir,
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

    def test_selection_2_deselected_1_selected(self, request, workspace, launcher_platform, testdir):
        result = self._run_shared_test(testdir,
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
