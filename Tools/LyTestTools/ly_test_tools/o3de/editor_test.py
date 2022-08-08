"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test-writing utilities that simplify creating O3DE Editor tests in Python.

Test writers should subclass a test suite from EditorTestSuite to hold the specification of python test scripts for
the editor to load and run. Tests can be parallelized (run in multiple editor instances at once) and/or batched
(multiple tests run in the same editor instance), with collated results and crash detection.

Usage example:
   class MyTestSuite(EditorTestSuite):

       class MyFirstTest(EditorSingleTest):
           from . import script_to_be_run_by_editor as test_module

       class MyTestInParallel_1(EditorBatchedTest):
           from . import another_script_to_be_run_by_editor as test_module

       class MyTestInParallel_2(EditorParallelTest):
           from . import yet_another_script_to_be_run_by_editor as test_module
"""

from __future__ import annotations
__test__ = False  # Avoid pytest collection & warnings since this module is for test functions, but not a test itself.

import logging
import math
import os
import tempfile
import threading

import pytest
import _pytest.python
import _pytest.outcomes

import ly_test_tools.o3de.editor_test_utils as editor_utils
from ly_test_tools._internal.managers.workspace import AbstractWorkspaceManager
from ly_test_tools.launchers import launcher_helper
from ly_test_tools.launchers.exceptions import WaitTimeoutError
from ly_test_tools.o3de.multi_test_framework import AbstractTestBase, AbstractTestSuite, Result

logger = logging.getLogger(__name__)


LOG_NAME = "editor_test.log"


class EditorSingleTest(AbstractTestBase):
    """
    Test that will run alone in one editor with no parallel editors, limiting environmental side-effects at the
    expense of redundant isolated work
    """

    def __init__(self):
        # Extra cmdline arguments to supply to the editor for the test
        self.extra_cmdline_args = []
        # Whether to use null renderer, this will override use_null_renderer for the Suite if not None
        self.use_null_renderer = None

    @staticmethod
    def setup(instance: EditorTestSuite.AbstractTestClass,
              request: _pytest.fixtures.FixtureRequest,
              workspace: AbstractWorkspaceManager,
              editor_test_results: EditorTestSuite.TestData,
              launcher_platform: str) -> None:
        """
        User-overrideable setup function, which will run before the test.
        :param instance: Parent EditorTestSuite.AbstractTestClass instance executing the test
        :param request: PyTest request object
        :param workspace: LyTestTools workspace manager
        :param editor_test_results: Currently recorded editor test results
        :param launcher_platform: user-parameterized string for LyTestTools
        """
        pass

    @staticmethod
    def wrap_run(instance: EditorTestSuite.AbstractTestClass,
                 request: _pytest.fixtures.FixtureRequest,
                 workspace: AbstractWorkspaceManager,
                 editor_test_results: EditorTestSuite.TestData,
                 launcher_platform: str) -> None:
        """
        User-overrideable wrapper function, which will run both before and after test.
        Any code before the 'yield' statement will run before the test. With code after yield run after the test.
        Setup will run before wrap_run starts. Teardown will run after it completes.
        :param instance: Parent EditorTestSuite.AbstractTestClass instance executing the test
        :param request: PyTest request object
        :param workspace: LyTestTools workspace manager
        :param editor_test_results: Currently recorded EditorTest results
        :param launcher_platform: user-parameterized string for LyTestTools
        """
        yield

    @staticmethod
    def teardown(instance: EditorTestSuite.AbstractTestClass,
                 request: _pytest.fixtures.FixtureRequest,
                 workspace: AbstractWorkspaceManager,
                 editor_test_results: EditorTestSuite.TestData,
                 launcher_platform: str) -> None:
        """
        User-overrideable teardown function, which will run after the test
        :param instance: Parent EditorTestSuite.AbstractTestClass instance executing the test
        :param request: PyTest request object
        :param workspace: LyTestTools workspace manager
        :param editor_test_results: Currently recorded editor test results
        :param launcher_platform: user-parameterized string for LyTestTools
        """
        pass


class EditorSharedTest(AbstractTestBase):
    """
    Test that will run in parallel with tests in different editor instances, as well as serially batching with other
    tests in each editor instance. Minimizes total test run duration.

    Does not support per test setup/teardown to avoid creating race conditions
    """
    # Specifies if the test can be batched in the same editor
    is_batchable = True
    # Specifies if the test can be run in multiple editors in parallel
    is_parallelizable = True


class EditorParallelTest(EditorSharedTest):
    """
    Test that will run in parallel with tests in different editor instances, though not serially batched with other
    tests in each editor instance. Reduces total test run duration, while limiting side-effects between tests.

    Does not support per test setup/teardown to avoid creating race conditions
    """
    is_batchable = False
    is_parallelizable = True


class EditorBatchedTest(EditorSharedTest):
    """
    Test that will run serially batched with the tests in the same editor instance, though not executed in parallel with
    other editor instances. Reduces overhead from starting the Editor, while limiting side-effects between editors.

    Does not support per test setup/teardown to avoid creating race conditions
    """
    is_batchable = True
    is_parallelizable = False


class EditorTestSuite(AbstractTestSuite):
    # Extra cmdline arguments to supply for every editor instance for this test suite
    global_extra_cmdline_args = ["-BatchMode", "-autotest_mode"]
    # Tests usually run with no renderer, however some tests require a renderer and will disable this
    use_null_renderer = True
    # Maximum time in seconds for a single editor to stay open across the set of shared tests
    timeout_editor_shared_test = 300
    # Maximum time (seconds) for waiting for a crash file to finish being dumped to disk
    _TIMEOUT_CRASH_LOG = 20
    # Return code for test failure
    _TEST_FAIL_RETCODE = 0xF
    # Test class to use for single test collection
    _single_test_class = EditorSingleTest
    # Test class to use for shared test collection
    _shared_test_class = EditorSharedTest
    # Log attribute value to use to collect Editor logs
    _log_attribute = "editor_log"

    @pytest.mark.parametrize("crash_log_watchdog", [("raise_on_crash", False)])
    def pytest_multitest_makeitem(
            collector: _pytest.python.Module, name: str, obj: object) -> EditorTestSuite.AbstractTestClass:
        """
        Enables ly_test_tools._internal.pytest_plugin.editor_test.pytest_pycollect_makeitem to collect the tests
        defined by this suite.
        This is required for any test suite that inherits from the AbstractTestSuite class else the tests won't be
        collected for that suite.
        :param collector: Module that serves as the pytest test class collector
        :param name: Name of the parent test class
        :param obj: Module of the test to be run
        :return: AbstractTestClass
        """
        return EditorTestSuite.AbstractTestClass(name, collector)

    def _exec_editor_test(self,
                          request: _pytest.fixtures.FixtureRequest,
                          workspace: AbstractWorkspaceManager,
                          run_id: int,
                          log_name: str,
                          test_spec: AbstractTestBase,
                          cmdline_args: list[str] = None) -> dict[str, Result.ResultType]:
        """
        Starts the editor with the given test and returns a result dict with a single element specifying the result
        :request: The pytest request
        :workspace: The LyTestTools Workspace object
        :run_id: The unique run id
        :log_name: The name of the editor log to retrieve
        :test_spec: The type of test class
        :cmdline_args: Any additional command line args
        :return: a dictionary of Result objects (should be only one) with a given Result.ResultType (i.e. Result.Pass).
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
        test_case_name = editor_utils.compile_test_case_name(request, test_spec)
        cmdline = [
            "--runpythontest", test_filename,
            f"-pythontestcase={test_case_name}",
            "-logfile", f"@log@/{log_name}",
            "-project-log-path", editor_utils.retrieve_log_path(run_id, workspace)] + test_cmdline_args
        editor = launcher_helper.create_editor(workspace)
        editor.args.extend(cmdline)
        editor.start(backupFiles=False, launch_ap=False, configure_settings=False)

        try:
            editor.wait(test_spec.timeout)
            output = editor.get_output()
            return_code = editor.get_returncode()
            editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)
            # Save the editor log
            workspace.artifact_manager.save_artifact(
                os.path.join(editor_utils.retrieve_log_path(run_id, workspace), log_name), f'({run_id}){log_name}')
            if return_code == 0:
                test_result = Result.Pass(EditorTestSuite._log_attribute, test_spec, output, editor_log_content)
            else:
                has_crashed = return_code != EditorTestSuite._TEST_FAIL_RETCODE
                if has_crashed:
                    crash_output = editor_utils.retrieve_crash_output(run_id, workspace, self._TIMEOUT_CRASH_LOG)
                    test_result = Result.Crash(EditorTestSuite._log_attribute, test_spec, output, return_code, crash_output, None)
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
                    test_result = Result.Fail(EditorTestSuite._log_attribute, test_spec, output, editor_log_content)
        except WaitTimeoutError:
            output = editor.get_output()
            editor.stop()
            editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)
            test_result = Result.Timeout(EditorTestSuite._log_attribute, test_spec, output, test_spec.timeout, editor_log_content)
    
        editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)
        results = self._get_results_using_output(EditorTestSuite._log_attribute, [test_spec], output, editor_log_content)
        results[test_spec.__name__] = test_result
        return results

    def _exec_editor_multitest(self,
                               request: _pytest.fixtures.FixtureRequest,
                               workspace: AbstractWorkspaceManager,
                               run_id: int,
                               log_name: str,
                               test_spec_list: list[AbstractTestBase],
                               cmdline_args: list[str] = None) -> dict[str, Result.ResultType]:
        """
        Starts an editor executable with a list of tests and returns a dict of the result of every test ran within that
        editor instance. In case of failure this function also parses the editor output to find out what specific tests
        failed.
        :request: The pytest request
        :workspace: The LyTestTools Workspace object
        :run_id: The unique run id
        :log_name: The name of the editor log to retrieve
        :test_spec_list: A list of test classes to run in the same editor instance
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

        # We create a files containing a semicolon separated scipts and test cases for the Editor to read
        test_script_list = ""
        test_case_list = ""

        for test_spec in test_spec_list:
            # Test script
            test_script_list += editor_utils.get_testcase_module_filepath(test_spec.test_module) + ';'

            # Test case
            test_case_name = editor_utils.compile_test_case_name(request, test_spec)
            test_case_list += f"{test_case_name};"

        # Remove the trailing semicolon from the last entry
        test_script_list = test_script_list[:-1]
        test_script_list = test_script_list.replace('\\', '/')
        test_case_list = test_case_list[:-1]

        temp_batched_script_file = tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.txt')
        temp_batched_script_file.write(test_script_list.replace('\\', '\\\\'))
        temp_batched_script_file.flush()
        temp_batched_script_file.close()

        temp_batched_case_file = tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.txt')
        temp_batched_case_file.write(test_case_list)
        temp_batched_case_file.flush()
        temp_batched_case_file.close()

        cmdline = [
            "-runpythontest", temp_batched_script_file.name,
            "-pythontestcase", temp_batched_case_file.name,
            "-logfile", f"@log@/{log_name}",
            "-project-log-path", editor_utils.retrieve_log_path(run_id, workspace)] + test_cmdline_args
        editor = launcher_helper.create_editor(workspace)
        editor.args.extend(cmdline)
        editor.start(backupFiles = False, launch_ap = False, configure_settings=False)

        output = ""
        editor_log_content = ""
        try:
            editor.wait(self.timeout_editor_shared_test)
            output = editor.get_output()
            return_code = editor.get_returncode()
            editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)
            # Save the editor log
            try:
                path_to_artifact = os.path.join(editor_utils.retrieve_log_path(run_id, workspace), log_name)
                full_log_name = f'({run_id}){log_name}'
                destination_path = workspace.artifact_manager.save_artifact(path_to_artifact, full_log_name)
                editor_utils.split_batched_editor_log_file(workspace, path_to_artifact, destination_path)
            except FileNotFoundError:
                # Error logging is already performed and we don't want this to fail the test
                pass
            if return_code == 0:
                # No need to scrape the output, as all the tests have passed
                for test_spec in test_spec_list:
                    results[test_spec.__name__] = Result.Pass(EditorTestSuite._log_attribute, test_spec, output, editor_log_content)
            else:
                # Scrape the output to attempt to find out which tests failed.
                # This function should always populate the result list, if it didn't find it, it will have "Unknown" type of result
                results = self._get_results_using_output(test_spec_list, output, editor_log_content)
                assert len(results) == len(test_spec_list), "bug in get_results_using_output(), the number of results don't match the tests ran"

                # If the editor crashed, find out in which test it happened and update the results
                has_crashed = return_code != EditorTestSuite._TEST_FAIL_RETCODE
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
                                results[test_spec_name] = Result.Crash(
                                    'editor_log', result.test_spec, output, return_code, crash_error, result.editor_log)
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
                        results[test_spec_name] = Result.Crash(
                            'editor_log', crashed_result.test_spec, output, return_code, crash_error,
                            crashed_result.editor_log)
        except WaitTimeoutError:            
            editor.stop()
            output = editor.get_output()
            editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)

            # The editor timed out when running the tests, get the data from the output to find out which ones ran
            results = self._get_results_using_output(test_spec_list, output, editor_log_content)
            assert len(results) == len(test_spec_list), "bug in _get_results_using_output(), the number of results don't match the tests ran"
            # Similar logic here as crashes, the first test that has no result is the one that timed out
            timed_out_result = None
            for test_spec_name, result in results.items():
                if isinstance(result, Result.Unknown):
                    if not timed_out_result:
                        results[test_spec_name] = Result.Timeout(EditorTestSuite._log_attribute,
                                                                 result.test_spec,
                                                                 result.output,
                                                                 self.timeout_editor_shared_test,
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
                results[test_spec_name] = Result.Timeout(EditorTestSuite._log_attribute,
                                                         timed_out_result.test_spec,
                                                         results[test_spec_name].output,
                                                         self.timeout_editor_shared_test,
                                                         result.editor_log)
        finally:
            if temp_batched_script_file:
                os.unlink(temp_batched_script_file.name)
            if temp_batched_case_file:
                os.unlink(temp_batched_case_file.name)
        return results

    def _run_single_test(self,
                         request: _pytest.fixtures.FixtureRequest,
                         workspace: AbstractWorkspaceManager,
                         collected_test_data: AbstractTestSuite.TestData,
                         test_spec: EditorSingleTest) -> None:
        """
        Runs a single test (one editor, one test) with the given specs
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :collected_test_data: The TestData from calling collected_test_data()
        :test_spec: The test class that should be a subclass of EditorSingleTest
        :return: None
        """
        self._setup_test_run(workspace, collected_test_data)
        editor = launcher_helper.create_editor(workspace)
        editor.configure_settings()
        extra_cmdline_args = []
        if hasattr(test_spec, "extra_cmdline_args"):
            extra_cmdline_args = test_spec.extra_cmdline_args

        result = self._exec_editor_test(request, workspace, 1, f"{LOG_NAME}", test_spec, extra_cmdline_args)
        if result is None:
            logger.error(f"Unexpectedly found no test run in the {LOG_NAME} during {test_spec}")
            result = {"Unknown":
                      Result.Unknown(
                          log_attribute='editor_log',
                          test_spec=test_spec,
                          extra_info=f"Unexpectedly found no test run information on stdout in the {LOG_NAME}")}
        collected_test_data.results.update(result)
        test_name, test_result = next(iter(result.items()))
        self._report_result(test_name, test_result)
        # If test did not pass, save assets with errors and warnings
        if not isinstance(test_result, Result.Pass):
            editor_utils.save_failed_asset_joblogs(workspace)

    def _run_batched_tests(self,
                           request: _pytest.fixtures.FixtureRequest,
                           workspace: AbstractWorkspaceManager,
                           collected_test_data: AbstractTestSuite.TestData,
                           test_spec_list: list[EditorSharedTest],
                           extra_cmdline_args: list[str] = None) -> None:
        """
        Runs a batch of tests in one single editor with the given spec list (one editor, multiple tests)
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :collected_test_data: The TestData from calling collected_test_data()
        :test_spec_list: A list of EditorSharedTest tests to run
        :extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if extra_cmdline_args is None:
            extra_cmdline_args = []

        if not test_spec_list:
            return

        self._setup_test_run(workspace, collected_test_data)
        editor = launcher_helper.create_editor(workspace)
        editor.configure_settings()
        results = self._exec_editor_multitest(
            request, workspace, 1, "editor_test.log", test_spec_list, extra_cmdline_args)
        collected_test_data.results.update(results)
        # If at least one test did not pass, save assets with errors and warnings
        for result in results:
            if result is None:
                logger.error("Unexpectedly found no test run in the editor log during EditorBatchedTest")
                logger.debug(f"Results from EditorBatchedTest:\n{results}")
            if not isinstance(result, Result.Pass):
                editor_utils.save_failed_asset_joblogs(workspace)
                return  # exit early on first batch failure

    def _run_parallel_tests(self,
                            request: _pytest.fixtures.FixtureRequest,
                            workspace: AbstractWorkspaceManager,
                            collected_test_data: AbstractTestSuite.TestData,
                            test_spec_list: list[EditorSharedTest],
                            extra_cmdline_args: list[str] = None) -> None:
        """
        Runs multiple editors with one test on each editor (multiple editor, one test each)
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :collected_test_data: The TestData from calling collected_test_data()
        :test_spec_list: A list of EditorSharedTest tests to run
        :extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if extra_cmdline_args is None:
            extra_cmdline_args = []

        if not test_spec_list:
            return

        self._setup_test_run(workspace, collected_test_data)
        editor = launcher_helper.create_editor(workspace)
        editor.configure_settings()
        parallel_editors = self._get_number_parallel_instances(request)
        assert parallel_editors > 0, "Must have at least one editor"

        # If there are more tests than max parallel editors, we will split them into multiple consecutive runs
        num_iterations = int(math.ceil(len(test_spec_list) / parallel_editors))
        for iteration in range(num_iterations):
            tests_for_iteration = test_spec_list[iteration*parallel_editors:(iteration+1)*parallel_editors]
            total_threads = len(tests_for_iteration)
            threads = []
            results_per_thread = [None] * total_threads
            for i in range(total_threads):
                def make_func(test_spec, index):
                    def run(request, workspace, extra_cmdline_args):
                        results = self._exec_editor_test(
                            request, workspace, index + 1, f"editor_test.log", test_spec, extra_cmdline_args)
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
                                  log_attribute='editor_log',
                                  test_spec=EditorParallelTest,
                                  extra_info="Unexpectedly found no test run information on stdout in the editor log")}
                collected_test_data.results.update(result)
                if not isinstance(result, Result.Pass):
                    save_asset_logs = True
            # If at least one test did not pass, save assets with errors and warnings
            if save_asset_logs:
                editor_utils.save_failed_asset_joblogs(workspace)

    def _run_parallel_batched_tests(self,
                                    request: _pytest.fixtures.FixtureRequest,
                                    workspace: AbstractWorkspaceManager,
                                    collected_test_data: AbstractTestSuite.TestData,
                                    test_spec_list: list[EditorSharedTest],
                                    extra_cmdline_args: list[str] = None) -> None:
        """
        Runs multiple editors with a batch of tests for each editor (multiple editor, multiple tests each)
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :collected_test_data: The TestData from calling collected_test_data()
        :test_spec_list: A list of EditorSharedTest tests to run
        :extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if extra_cmdline_args is None:
            extra_cmdline_args = []

        if not test_spec_list:
            return

        self._setup_test_run(workspace, collected_test_data)
        editor = launcher_helper.create_editor(workspace)
        editor.configure_settings()
        total_threads = self._get_number_parallel_instances(request)
        assert total_threads > 0, "Must have at least one editor"
        threads = []
        tests_per_editor = int(math.ceil(len(test_spec_list) / total_threads))
        results_per_thread = [None] * total_threads
        for i in range(total_threads):
            tests_for_thread = test_spec_list[i*tests_per_editor:(i+1)*tests_per_editor]

            def make_func(test_spec_list_for_editor, index):
                def run(request, workspace, extra_cmdline_args):
                    results = None
                    if len(test_spec_list_for_editor) > 0:
                        results = self._exec_editor_multitest(
                            request, workspace, index + 1, f"editor_test.log", test_spec_list_for_editor,
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
                              log_attribute='editor_log',
                              test_spec=EditorSharedTest,
                              extra_info="Unexpectedly found no test run information on stdout in the editor log")}
            collected_test_data.results.update(result)
            if not isinstance(result, Result.Pass):
                save_asset_logs = True
        # If at least one test did not pass, save assets with errors and warnings
        if save_asset_logs:
            editor_utils.save_failed_asset_joblogs(workspace)
