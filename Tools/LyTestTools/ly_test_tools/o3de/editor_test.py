"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Simplified O3DE Editor test-writing utilities.

Test writers should subclass a test suite from TestSuite for easy specification of python test scripts for
the editor to run. Tests can be parallelized (run in multiple editor instances at once) and/or batched (multiple tests
run in the same editor instance), with collated results and crash detection.

Usage example:
   class MyTestSuite(TestSuite):

       class MyFirstTest(EditorSingleTest):
           from . import script_to_be_run_by_editor as test_module

       class MyTestInParallel_1(EditorParallelTest):
           from . import another_script_to_be_run_by_editor as test_module

       class MyTestInParallel_2(EditorParallelTest):
           from . import yet_another_script_to_be_run_by_editor as test_module
"""
from __future__ import annotations
import logging
import math
import os
import threading

import pytest

import ly_test_tools.o3de.editor_test_utils as editor_utils

from ly_test_tools.o3de.test_suite_base import Result, TestBase, TestSuite, SingleTest

# This file contains ready-to-use test functions which are not actual tests, avoid pytest collection
__test__ = False

logger = logging.getLogger(__name__)


class EditorSingleTest(SingleTest):
    """
    Test that will be run alone in one editor, with no parallel editors
    """
    def __init__(self):
        super(EditorSingleTest, self).__init__()
        # Extra cmdline arguments to supply to the editor for the test
        self.extra_cmdline_args = []
        # Whether to use null renderer, this will override use_null_renderer for the Suite if not None
        self.use_null_renderer = None

    @staticmethod
    def setup(instance, request, workspace, editor, editor_test_results, launcher_platform):
        """
        User-overrideable setup function, which will run before the test
        """
        pass

    @staticmethod
    def wrap_run(instance, request, workspace, editor, editor_test_results, launcher_platform):
        """
        User-overrideable wrapper function, which will run before and after test.
        Any code before the 'yield' statement will run before the test. With code after yield run after the test.
        """
        yield

    @staticmethod
    def teardown(instance, request, workspace, editor, editor_test_results, launcher_platform):
        """
        User-overrideable teardown function, which will run after the test
        """
        pass


@pytest.mark.parametrize("crash_log_watchdog", [("raise_on_crash", False)])
class EditorTestSuite(TestSuite):
    pass

    def _exec_editor_test(self,
                          request: _pytest.fixtures.FixtureRequest,
                          workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                          editor: ly_test_tools.launchers.platforms.base.Launcher,
                          run_id: int, log_name:str,
                          test_spec: TestBase,
                          cmdline_args: list[str] = None) -> dict[str, Result]:
        """
        Starts the editor with the given test and returns an result dict with a single element specifying the result
        :request: The pytest request
        :workspace: The LyTestTools Workspace object
        :editor: The LyTestTools Editor object
        :run_id: The unique run id
        :log_name: The name of the editor log to retrieve
        :test_spec: The type of TestBase
        :cmdline_args: Any additional command line args
        :return: a dictionary of Result objects
        """
        if cmdline_args is None:
            cmdline_args = []
        test_cmdline_args = self.global_extra_cmdline_args + cmdline_args
        test_cmdline_args += [
            "--regset=/Amazon/Preferences/EnablePrefabSystem=true",
            f"--regset-file={os.path.join(workspace.paths.engine_root(), 'Registry', 'prefab.test.setreg')}"]

        test_spec_uses_null_renderer = getattr(test_spec, "use_null_renderer", None)
        if test_spec_uses_null_renderer or (test_spec_uses_null_renderer is None and self.use_null_renderer):
            test_cmdline_args += ["-rhi=null"]
        if test_spec.attach_debugger:
            test_cmdline_args += ["--attach-debugger"]
        if test_spec.wait_for_debugger:
            test_cmdline_args += ["--wait-for-debugger"]

        # Cycle any old crash report in case it wasn't cycled properly
        editor_utils.cycle_crash_report(run_id, workspace)

        test_result = None
        results = {}
        test_filename = editor_utils.get_testcase_module_filepath(test_spec.test_module)
        cmdline = [
                      "--runpythontest", test_filename,
                      "-logfile", f"@log@/{log_name}",
                      "-project-log-path", editor_utils.retrieve_log_path(run_id, workspace)] + test_cmdline_args
        editor.args.extend(cmdline)
        editor.start(backupFiles=False, launch_ap=False, configure_settings=False)

        try:
            editor.wait(test_spec.timeout)
            output = editor.get_output()
            return_code = editor.get_returncode()
            editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)
            # Save the editor log
            workspace.artifact_manager.save_artifact(
                os.path.join(editor_utils.retrieve_log_path(run_id, workspace), log_name),
                f'({run_id}){log_name}')
            if return_code == 0:
                test_result = Result.Pass(test_spec, output, editor_log_content)
            else:
                has_crashed = return_code != TestSuite._TEST_FAIL_RETCODE
                if has_crashed:
                    crash_output = editor_utils.retrieve_crash_output(run_id, workspace, self._TIMEOUT_CRASH_LOG)
                    test_result = Result.Crash(test_spec, output, return_code, crash_output, None)
                    # Save the .dmp file which is generated on Windows only
                    dmp_file_name = os.path.join(editor_utils.retrieve_log_path(run_id, workspace),
                                                 'error.dmp')
                    if os.path.exists(dmp_file_name):
                        workspace.artifact_manager.save_artifact(dmp_file_name)
                    # Save the crash log
                    crash_file_name = os.path.join(editor_utils.retrieve_log_path(run_id, workspace),
                                                   os.path.basename(workspace.paths.crash_log()))
                    if os.path.exists(crash_file_name):
                        workspace.artifact_manager.save_artifact(crash_file_name)
                        editor_utils.cycle_crash_report(run_id, workspace)
                    else:
                        logger.warning(f"Crash occurred, but could not find log {crash_file_name}")
                else:
                    test_result = Result.Fail(test_spec, output, editor_log_content)
        except WaitTimeoutError:
            output = editor.get_output()
            editor.stop()
            editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)
            test_result = Result.Timeout(test_spec, output, test_spec.timeout, editor_log_content)

        editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)
        results = self._get_results_using_output([test_spec], output, editor_log_content)
        results[test_spec.__name__] = test_result
        return results

    def _exec_editor_multitest(self, request: _pytest.fixtures.FixtureRequest,
                               workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                               editor: ly_test_tools.launchers.platforms.base.Launcher, run_id: int, log_name: str,
                               test_spec_list: list[TestBase],
                               cmdline_args: list[str] = None) -> dict[str, Result]:
        """
        Starts an editor executable with a list of tests and returns a dict of the result of every test ran within that
        editor instance. In case of failure this function also parses the editor output to find out what specific tests
        failed.
        :request: The pytest request
        :workspace: The LyTestTools Workspace object
        :editor: The LyTestTools Editor object
        :run_id: The unique run id
        :log_name: The name of the editor log to retrieve
        :test_spec_list: A list of TestBase tests to run in the same editor instance
        :cmdline_args: Any additional command line args
        :return: A dict of Result objects
        """
        if cmdline_args is None:
            cmdline_args = []
        test_cmdline_args = self.global_extra_cmdline_args + cmdline_args
        test_cmdline_args += [
            "--regset=/Amazon/Preferences/EnablePrefabSystem=true",
            f"--regset-file={os.path.join(workspace.paths.engine_root(), 'Registry', 'prefab.test.setreg')}"]

        if self.use_null_renderer:
            test_cmdline_args += ["-rhi=null"]
        if any([t.attach_debugger for t in test_spec_list]):
            test_cmdline_args += ["--attach-debugger"]
        if any([t.wait_for_debugger for t in test_spec_list]):
            test_cmdline_args += ["--wait-for-debugger"]

        # Cycle any old crash report in case it wasn't cycled properly
        editor_utils.cycle_crash_report(run_id, workspace)

        results = {}
        test_filenames_str = ";".join(
            editor_utils.get_testcase_module_filepath(test_spec.test_module) for test_spec in test_spec_list)
        cmdline = [
                      "--runpythontest", test_filenames_str,
                      "-logfile", f"@log@/{log_name}",
                      "-project-log-path", editor_utils.retrieve_log_path(run_id, workspace)] + test_cmdline_args

        editor.args.extend(cmdline)
        editor.start(backupFiles=False, launch_ap=False, configure_settings=False)

        output = ""
        editor_log_content = ""
        try:
            editor.wait(self.shared_test_timeout)
            output = editor.get_output()
            return_code = editor.get_returncode()
            editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)
            # Save the editor log
            try:
                workspace.artifact_manager.save_artifact(
                    os.path.join(editor_utils.retrieve_log_path(run_id, workspace), log_name), f'({run_id}){log_name}')
            except FileNotFoundError:
                # Error logging is already performed and we don't want this to fail the test
                pass
            if return_code == 0:
                # No need to scrape the output, as all the tests have passed
                for test_spec in test_spec_list:
                    results[test_spec.__name__] = Result.Pass(test_spec, output, editor_log_content)
            else:
                # Scrape the output to attempt to find out which tests failed.
                # This function should always populate the result list, if it didn't find it, it will have "Unknown" type of result
                results = self._get_results_using_output(test_spec_list, output, editor_log_content)
                assert len(results) == len(
                    test_spec_list), "bug in _get_results_using_output(), the number of results don't match the tests ran"

                # If the editor crashed, find out in which test it happened and update the results
                has_crashed = return_code != TestSuite._TEST_FAIL_RETCODE
                if has_crashed:
                    crashed_result = None
                    for test_spec_name, result in results.items():
                        if isinstance(result, Result.Unknown):
                            if not crashed_result:
                                # The first test with "Unknown" result (no data in output) is likely the one that crashed
                                crash_error = editor_utils.retrieve_crash_output(run_id, workspace,
                                                                                 self._TIMEOUT_CRASH_LOG)
                                # Save the .dmp file which is generated on Windows only
                                dmp_file_name = os.path.join(editor_utils.retrieve_log_path(run_id, workspace),
                                                             'error.dmp')
                                if os.path.exists(dmp_file_name):
                                    workspace.artifact_manager.save_artifact(dmp_file_name)
                                # Save the crash log
                                crash_file_name = os.path.join(editor_utils.retrieve_log_path(run_id, workspace),
                                                               os.path.basename(workspace.paths.crash_log()))
                                if os.path.exists(crash_file_name):
                                    workspace.artifact_manager.save_artifact(crash_file_name)
                                    editor_utils.cycle_crash_report(run_id, workspace)
                                else:
                                    logger.warning(f"Crash occurred, but could not find log {crash_file_name}")
                                results[test_spec_name] = Result.Crash(result.test_spec, output, return_code,
                                                                       crash_error, result.editor_log)
                                crashed_result = result
                            else:
                                # If there are remaning "Unknown" results, these couldn't execute because of the crash,
                                # update with info about the offender
                                results[test_spec_name].extra_info = f"This test has unknown result," \
                                                                     f"test '{crashed_result.test_spec.__name__}'" \
                                                                     f"crashed before this test could be executed"
                    # if all the tests ran, the one that has caused the crash is the last test
                    if not crashed_result:
                        crash_error = editor_utils.retrieve_crash_output(run_id, workspace, self._TIMEOUT_CRASH_LOG)
                        editor_utils.cycle_crash_report(run_id, workspace)
                        results[test_spec_name] = Result.Crash(crashed_result.test_spec, output, return_code,
                                                               crash_error, crashed_result.editor_log)
        except WaitTimeoutError:
            editor.stop()
            output = editor.get_output()
            editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)

            # The editor timed out when running the tests, get the data from the output to find out which ones ran
            results = self._get_results_using_output(test_spec_list, output, editor_log_content)
            assert len(results) == len(
                test_spec_list), "bug in _get_results_using_output(), the number of results don't match the tests ran"
            # Similar logic here as crashes, the first test that has no result is the one that timed out
            timed_out_result = None
            for test_spec_name, result in results.items():
                if isinstance(result, Result.Unknown):
                    if not timed_out_result:
                        results[test_spec_name] = Result.Timeout(result.test_spec, result.output,
                                                                 self.shared_test_timeout,
                                                                 result.editor_log)
                        timed_out_result = result
                    else:
                        # If there are remaning "Unknown" results, these couldn't execute because of the timeout,
                        # update with info about the offender
                        results[test_spec_name].extra_info = f"This test has unknown result, test " \
                                                             f"'{timed_out_result.test_spec.__name__}' timed out " \
                                                             f"before this test could be executed"
            # if all the tests ran, the one that has caused the timeout is the last test, as it didn't close the editor
            if not timed_out_result:
                results[test_spec_name] = Result.Timeout(timed_out_result.test_spec,
                                                         results[test_spec_name].output,
                                                         self.shared_test_timeout, result.editor_log)
        return results

    def _run_single_test(self, request: _pytest.fixtures.FixtureRequest,
                         workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                         editor: ly_test_tools.launchers.platforms.base.Launcher,
                         test_data: TestData, test_spec: EditorSingleTest) -> None:
        """
        Runs a single test (one editor, one test) with the given specs
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :editor: The LyTestTools Editor object
        :test_data: The TestData from calling test_data()
        :test_spec: The test class that should be a subclass of EditorSingleTest
        :return: None
        """
        self._setup_editor_test(editor, workspace, test_data)
        extra_cmdline_args = []
        if hasattr(test_spec, "extra_cmdline_args"):
            extra_cmdline_args = test_spec.extra_cmdline_args

        result = self._exec_editor_test(request, workspace, editor, 1, "editor_test.log", test_spec, extra_cmdline_args)
        if result is None:
            logger.error(f"Unexpectedly found no test run in the editor log during {test_spec}")
            result = {"Unknown":
                Result.Unknown(
                    test_spec=test_spec,
                    extra_info="Unexpectedly found no test run information on stdout in the editor log")}
        test_data.results.update(result)
        test_name, test_result = next(iter(result.items()))
        self._report_result(test_name, test_result)
        # If test did not pass, save assets with errors and warnings
        if not isinstance(test_result, Result.Pass):
            editor_utils.save_failed_asset_joblogs(workspace)

    def _run_batched_tests(self, request: _pytest.fixtures.FixtureRequest,
                           workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                           editor: ly_test_tools.launchers.platforms.base.Launcher, test_data: TestData,
                           test_spec_list: list[EditorSharedTest], extra_cmdline_args: list[str] = None) -> None:
        """
        Runs a batch of tests in one single editor with the given spec list (one editor, multiple tests)
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :editor: The LyTestTools Editor object
        :test_data: The TestData from calling test_data()
        :test_spec_list: A list of EditorSharedTest tests to run
        :extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if extra_cmdline_args is None:
            extra_cmdline_args = []

        if not test_spec_list:
            return

        self._setup_editor_test(editor, workspace, test_data)
        results = self._exec_editor_multitest(request, workspace, editor, 1, "editor_test.log", test_spec_list,
                                              extra_cmdline_args)
        test_data.results.update(results)
        # If at least one test did not pass, save assets with errors and warnings
        for result in results:
            if result is None:
                logger.error("Unexpectedly found no test run in the editor log during EditorBatchedTest")
                logger.debug(f"Results from EditorBatchedTest:\n{results}")
            if not isinstance(result, Result.Pass):
                editor_utils.save_failed_asset_joblogs(workspace)
                return  # exit early on first batch failure

    def _run_parallel_tests(self, request: _pytest.fixtures.FixtureRequest,
                            workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                            editor: ly_test_tools.launchers.platforms.base.Launcher, test_data: TestData,
                            test_spec_list: list[EditorSharedTest], extra_cmdline_args: list[str] = None) -> None:
        """
        Runs multiple editors with one test on each editor (multiple editor, one test each)
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :editor: The LyTestTools Editor object
        :test_data: The TestData from calling test_data()
        :test_spec_list: A list of EditorSharedTest tests to run
        :extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if extra_cmdline_args is None:
            extra_cmdline_args = []

        if not test_spec_list:
            return

        self._setup_editor_test(editor, workspace, test_data)
        parallel_editors = self._get_number_parallel_instances(request, "--editors-parallel")
        assert parallel_editors > 0, "Must have at least one editor"

        # If there are more tests than max parallel editors, we will split them into multiple consecutive runs
        num_iterations = int(math.ceil(len(test_spec_list) / parallel_editors))
        for iteration in range(num_iterations):
            tests_for_iteration = test_spec_list[iteration * parallel_editors:(iteration + 1) * parallel_editors]
            total_threads = len(tests_for_iteration)
            threads = []
            results_per_thread = [None] * total_threads
            for i in range(total_threads):
                def make_func(test_spec, index, my_editor):
                    def run(request, workspace, extra_cmdline_args):
                        results = self._exec_editor_test(request, workspace, my_editor, index + 1, f"editor_test.log",
                                                         test_spec, extra_cmdline_args)
                        assert results is not None
                        results_per_thread[index] = results

                    return run

                # Duplicate the editor using the one coming from the fixture
                cur_editor = editor.__class__(workspace, editor.args.copy())
                f = make_func(tests_for_iteration[i], i, cur_editor)
                t = threading.Thread(target=f, args=(request, workspace, extra_cmdline_args))
                t.start()
                threads.append(t)

            for t in threads:
                t.join()

            save_asset_logs = False

            for result in results_per_thread:
                if result is None:
                    logger.error("Unexpectedly found no test run in the editor log during EditorParallelTest")
                    logger.debug(f"Results from EditorParallelTest thread:\n{results_per_thread}")
                    result = {"Unknown":
                        Result.Unknown(
                            test_spec=EditorParallelTest,
                            extra_info="Unexpectedly found no test run information on stdout in the editor log")}
                test_data.results.update(result)
                if not isinstance(result, Result.Pass):
                    save_asset_logs = True
            # If at least one test did not pass, save assets with errors and warnings
            if save_asset_logs:
                editor_utils.save_failed_asset_joblogs(workspace)

    def _run_parallel_batched_tests(self, request: _pytest.fixtures.FixtureRequest,
                                    workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                                    editor: ly_test_tools.launchers.platforms.base.Launcher, test_data: TestData,
                                    test_spec_list: list[EditorSharedTest],
                                    extra_cmdline_args: list[str] = None) -> None:
        """
        Runs multiple editors with a batch of tests for each editor (multiple editor, multiple tests each)
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :editor: The LyTestTools Editor object
        :test_data: The TestData from calling test_data()
        :test_spec_list: A list of EditorSharedTest tests to run
        :extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if extra_cmdline_args is None:
            extra_cmdline_args = []

        if not test_spec_list:
            return

        self._setup_editor_test(editor, workspace, test_data)
        total_threads = self._get_number_parallel_instances(request, "--editors-parallel")
        assert total_threads > 0, "Must have at least one editor"
        threads = []
        tests_per_editor = int(math.ceil(len(test_spec_list) / total_threads))
        results_per_thread = [None] * total_threads
        for i in range(total_threads):
            tests_for_thread = test_spec_list[i * tests_per_editor:(i + 1) * tests_per_editor]

            def make_func(test_spec_list_for_editor, index, my_editor):
                def run(request, workspace, extra_cmdline_args):
                    results = None
                    if len(test_spec_list_for_editor) > 0:
                        results = self._exec_editor_multitest(request, workspace, my_editor, index + 1,
                                                              f"editor_test.log", test_spec_list_for_editor,
                                                              extra_cmdline_args)
                        assert results is not None
                    else:
                        results = {}
                    results_per_thread[index] = results

                return run

            # Duplicate the editor using the one coming from the fixture
            cur_editor = editor.__class__(workspace, editor.args.copy())
            f = make_func(tests_for_thread, i, cur_editor)
            t = threading.Thread(target=f, args=(request, workspace, extra_cmdline_args))
            t.start()
            threads.append(t)

        for t in threads:
            t.join()

        save_asset_logs = False
        for result in results_per_thread:
            if result is None:
                logger.error("Unexpectedly found no test run in the editor log during EditorSharedTest")
                logger.debug(f"Results from EditorSharedTest thread:\n{results_per_thread}")
                result = {"Unknown":
                    Result.Unknown(
                        test_spec=EditorSharedTest,
                        extra_info="Unexpectedly found no test run information on stdout in the editor log")}
            test_data.results.update(result)
            if not isinstance(result, Result.Pass):
                save_asset_logs = True
        # If at least one test did not pass, save assets with errors and warnings
        if save_asset_logs:
            editor_utils.save_failed_asset_joblogs(workspace)
