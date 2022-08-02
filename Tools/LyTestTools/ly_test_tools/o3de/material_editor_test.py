"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# Test-writing utilities that simplify creating O3DE MaterialEditor tests in Python.
#
# Test writers should subclass a test suite from MaterialEditorSuite to hold the specification of python test scripts
# for the MaterialEditor to load and run.
# Tests can be parallelized (run in multiple MaterialEditor instances at once) and/or
# batched (multiple tests run in the same MaterialEditor instance), with collated results and crash detection.
# Tests retain the ability to be run as a single test in a single MaterialEditor instance as well.
#
# Usage example:
#    class MyTestSuite(MaterialEditorSuite):
#
#        class MyFirstTest(MaterialEditorSingleTest):
#            from . import script_to_be_run_by_material_editor as test_module
#
#        class MyTestInParallel_1(MaterialEditorBatchedTest):
#            from . import another_script_to_be_run_by_material_editor as test_module
#
#        class MyTestInParallel_2(MaterialEditorParallelTest):
#            from . import yet_another_script_to_be_run_by_material_editor as test_module

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
from ly_test_tools.launchers.exceptions import WaitTimeoutError
from ly_test_tools.launchers import launcher_helper
from ly_test_tools.o3de.multi_test_framework import AbstractTestBase, AbstractTestSuite, Result

logger = logging.getLogger(__name__)

LOG_NAME = "material_editor_test.log"


class MaterialEditorSingleTest(AbstractTestBase):
    """
    Test that will run alone in one MaterialEditor with no parallel MaterialEditors,
    limiting environmental side-effects at the expense of redundant isolated work
    """

    def __init__(self):
        # Extra cmdline arguments to supply to the MaterialEditor for the test
        self.extra_cmdline_args = []
        # Whether to use null renderer, this will override use_null_renderer for the Suite if not None
        self.use_null_renderer = None

    @staticmethod
    def setup(instance: MaterialEditorTestSuite.AbstractTestClass,
              request: _pytest.fixtures.FixtureRequest,
              workspace: AbstractWorkspaceManager,
              material_editor_test_results: AbstractTestSuite.TestData,
              launcher_platform: str) -> None:
        """
        User-overrideable setup function, which will run before the test.
        :param instance: Parent MaterialEditorTestSuite.AbstractTestClass instance executing the test
        :param request: PyTest request object
        :param workspace: LyTestTools workspace manager
        :param material_editor_test_results: Currently recorded MaterialEditor test results
        :param launcher_platform: user-parameterized string for LyTestTools
        """
        pass

    @staticmethod
    def wrap_run(instance: MaterialEditorTestSuite.AbstractTestClass,
                 request: _pytest.fixtures.FixtureRequest,
                 workspace: AbstractWorkspaceManager,
                 material_editor_test_results: AbstractTestSuite.TestData,
                 launcher_platform: str) -> None:
        """
        User-overrideable wrapper function, which will run both before and after test.
        Any code before the 'yield' statement will run before the test. With code after yield run after the test.
        Setup will run before wrap_run starts. Teardown will run after it completes.
        :param instance: Parent MaterialEditorTestSuite.AbstractTestClass instance executing the test
        :param request: PyTest request object
        :param workspace: LyTestTools workspace manager
        :param material_editor_test_results: Currently recorded MaterialEditor test results
        :param launcher_platform: user-parameterized string for LyTestTools
        """
        yield

    @staticmethod
    def teardown(instance: MaterialEditorTestSuite.AbstractTestClass,
                 request: _pytest.fixtures.FixtureRequest,
                 workspace: AbstractWorkspaceManager,
                 material_editor_test_results: AbstractTestSuite.TestData,
                 launcher_platform: str) -> None:
        """
        User-overrideable teardown function, which will run after the test
        :param instance: Parent MaterialEditorTestSuite.AbstractTestClass instance executing the test
        :param request: PyTest request object
        :param workspace: LyTestTools workspace manager
        :param material_editor_test_results: Currently recorded MaterialEditor test results
        :param launcher_platform: user-parameterized string for LyTestTools
        """
        pass


class MaterialEditorSharedTest(AbstractTestBase):
    """
    Test that will run in parallel with tests in different MaterialEditor instances, as well as serially batching
    with other tests in each MaterialEditor instance. Minimizes total test run duration.

    Does not support per test setup/teardown to avoid creating race conditions
    """
    # Specifies if the test can be batched in the same MaterialEditor
    is_batchable = True
    # Specifies if the test can be run in multiple MaterialEditors in parallel
    is_parallelizable = True


class MaterialEditorParallelTest(MaterialEditorSharedTest):
    """
    Test that will run in parallel with tests in different MaterialEditor instances,
    though not serially batched with other tests in each MaterialEditor instance.
    Reduces total test run duration, while limiting side-effects between tests.

    Does not support per test setup/teardown to avoid creating race conditions
    """
    is_batchable = False
    is_parallelizable = True


class MaterialEditorBatchedTest(MaterialEditorSharedTest):
    """
    Test that will run serially batched with the tests in the same MaterialEditor instance,
    though not executed in parallel with other MaterialEditor instances.
    Reduces overhead from starting the MaterialEditor, while limiting side-effects between MaterialEditors.

    Does not support per test setup/teardown to avoid creating race conditions
    """
    is_batchable = True
    is_parallelizable = False


class MaterialEditorResult(Result):
    """Used to set the log_attribute value for MaterialEditor result logs."""
    Result.log_attribute = "material_editor_log"


class MaterialEditorTestSuite(AbstractTestSuite):
    # Extra cmdline arguments to supply for every MaterialEditor instance for this test suite
    global_extra_cmdline_args = ["-BatchMode", "-autotest_mode"]
    # Tests usually run with no renderer, however some tests require a renderer and will disable this
    use_null_renderer = True
    # Maximum time in seconds for a single MaterialEditor to stay open across the set of shared tests
    timeout_material_editor_shared_test = 300
    # Maximum time (seconds) for waiting for a crash file to finish being dumped to disk
    _TIMEOUT_CRASH_LOG = 20
    # Return code for test failure
    _TEST_FAIL_RETCODE = 0xF
    # Test class to use for single test collection
    single_test_class = MaterialEditorSingleTest
    # Test class to use for shared test collection
    shared_test_class = MaterialEditorSharedTest

    @pytest.mark.parametrize("crash_log_watchdog", [("raise_on_crash", False)])
    def pytest_multitest_makeitem(
            collector: _pytest.python.Module, name: str, obj: object) -> MaterialEditorTestSuite.AbstractTestClass:
        """
        Enables ly_test_tools._internal.pytest_plugin.editor_test.pytest_pycollect_makeitem to collect the tests
        defined by this suite.
        This is required for any test suite that inherits from the AbstractTestSuite class else the tests won't be
        collected for that suite when using the ly_test_tools.o3de.multi_test_framework module.
        :param collector: Module that serves as the pytest test class collector
        :param name: Name of the parent test class
        :param obj: Module of the test to be run
        :return: MaterialEditorTestSuite.AbstractTestClass
        """
        return MaterialEditorTestSuite.AbstractTestClass(name, collector)

    def _get_number_parallel_instances(self, request: _pytest.fixtures.FixtureRequest) -> int:
        """
        Retrieves the number of parallel instances preference based on cmdline overrides or class overrides.
        Defaults to self.get_number_parallel_instances() from inherited AbstractTestSuite class.
        :request: The Pytest Request object
        :return: The number of parallel MaterialEditors to use
        """
        parallel_material_editors_value = request.config.getoption("--parallel-executables", None)
        if parallel_material_editors_value:
            return int(parallel_material_editors_value)

        return self.get_number_parallel_instances()

    def _exec_material_editor_test(self,
                                   request: _pytest.fixtures.FixtureRequest,
                                   workspace: AbstractWorkspaceManager,
                                   run_id: int,
                                   log_name: str,
                                   test_spec: AbstractTestBase,
                                   cmdline_args: list[str] = None) -> dict[str, MaterialEditorResult.ResultType]:
        """
        Starts the MaterialEditor with the given test & returns a result dict with a single element specifying the result
        :request: The pytest request
        :workspace: The LyTestTools Workspace object
        :run_id: The unique run id
        :log_name: The name of the MaterialEditor log to retrieve
        :test_spec: The type of test class
        :cmdline_args: Any additional command line args
        :return: a dictionary of Result objects (should be only one)
        """
        if cmdline_args is None:
            cmdline_args = []
        test_cmdline_args = self.global_extra_cmdline_args + cmdline_args

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
                      "-logfile", os.path.join(editor_utils.retrieve_material_editor_log_path(run_id, workspace),
                                               log_name)] + test_cmdline_args
        material_editor = launcher_helper.create_material_editor(workspace)
        material_editor.args.extend(cmdline)
        material_editor.start(backupFiles=False, launch_ap=False, configure_settings=False)

        try:
            material_editor.wait(test_spec.timeout)
            output = material_editor.get_output()
            return_code = material_editor.get_returncode()
            material_editor_log_content = editor_utils.retrieve_material_editor_log_content(run_id, log_name, workspace)
            # Save the MaterialEditor log
            workspace.artifact_manager.save_artifact(
                os.path.join(editor_utils.retrieve_material_editor_log_path(run_id, workspace), log_name))
            if return_code == 0:
                test_result = MaterialEditorResult.Pass(test_spec, output, material_editor_log_content)
            else:
                has_crashed = return_code != MaterialEditorTestSuite._TEST_FAIL_RETCODE
                if has_crashed:
                    crash_output = editor_utils.retrieve_crash_output(run_id, workspace, self._TIMEOUT_CRASH_LOG)
                    test_result = MaterialEditorResult.Crash(test_spec, output, return_code, crash_output, None)
                    # Save the .dmp file which is generated on Windows only
                    dmp_file_name = os.path.join(editor_utils.retrieve_material_editor_log_path(run_id, workspace),
                                                 'error.dmp')
                    if os.path.exists(dmp_file_name):
                        workspace.artifact_manager.save_artifact(dmp_file_name)
                    # Save the crash log
                    crash_file_name = os.path.join(editor_utils.retrieve_material_editor_log_path(run_id, workspace),
                                                   os.path.basename(workspace.paths.crash_log()))
                    if os.path.exists(crash_file_name):
                        workspace.artifact_manager.save_artifact(crash_file_name)
                        editor_utils.cycle_crash_report(run_id, workspace)
                    else:
                        logger.warning(f"Crash occurred, but could not find log {crash_file_name}")
                else:
                    test_result = MaterialEditorResult.Fail(test_spec, output, material_editor_log_content)
        except WaitTimeoutError:
            output = material_editor.get_output()
            material_editor.stop()
            material_editor_log_content = editor_utils.retrieve_material_editor_log_content(run_id, log_name, workspace)
            test_result = MaterialEditorResult.Timeout(test_spec, output, test_spec.timeout, material_editor_log_content)

        material_editor_log_content = editor_utils.retrieve_material_editor_log_content(run_id, log_name, workspace)
        results = self._get_results_using_output([test_spec], output, material_editor_log_content)
        results[test_spec.__name__] = test_result
        return results

    def _exec_material_editor_multitest(self,
                                        request: _pytest.fixtures.FixtureRequest,
                                        workspace: AbstractWorkspaceManager,
                                        run_id: int,
                                        log_name: str,
                                        test_spec_list: list[AbstractTestBase],
                                        cmdline_args: list[str] = None) -> dict[str, MaterialEditorResult.ResultType]:
        """
        Starts a MaterialEditor executable with a list of tests and returns a dict of the result of every test ran
        within that MaterialEditor instance.
        In case of failure this function also parses the MaterialEditor output to find out what specific tests failed.
        :request: The pytest request
        :workspace: The LyTestTools Workspace object
        :run_id: The unique run id
        :log_name: The name of the MaterialEditor log to retrieve
        :test_spec_list: A list of test classes to run in the same MaterialEditor instance
        :cmdline_args: Any additional command line args
        :return: A dict of Result objects
        """
        if cmdline_args is None:
            cmdline_args = []
        test_cmdline_args = self.global_extra_cmdline_args + cmdline_args

        if self.use_null_renderer:
            test_cmdline_args += ["-rhi=null"]
        if any([t.attach_debugger for t in test_spec_list]):
            test_cmdline_args += ["--attach-debugger"]
        if any([t.wait_for_debugger for t in test_spec_list]):
            test_cmdline_args += ["--wait-for-debugger"]

        # Cycle any old crash report in case it wasn't cycled properly
        editor_utils.cycle_crash_report(run_id, workspace)

        results = {}

        # We create a file containing a semicolon separated list for the MaterialEditor to read
        temp_batched_file = tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.txt')
        for test_spec in test_spec_list[:-1]:
            temp_batched_file.write(editor_utils.get_testcase_module_filepath(test_spec.test_module)
                                    .replace('\\', '\\\\') + ';')
        # The last entry does not have a semicolon
        temp_batched_file.write(editor_utils.get_testcase_module_filepath(test_spec_list[-1].test_module)
                                .replace('\\', '\\\\'))
        temp_batched_file.flush()
        temp_batched_file.close()

        cmdline = [
                      "--runpythontest", temp_batched_file.name,
                      "-logfile", os.path.join(editor_utils.retrieve_material_editor_log_path(run_id, workspace),
                                               log_name)] + test_cmdline_args
        material_editor = launcher_helper.create_material_editor(workspace)
        material_editor.args.extend(cmdline)
        material_editor.start(backupFiles=False, launch_ap=False, configure_settings=False)

        output = ""
        material_editor_log_content = ""
        try:
            material_editor.wait(self.timeout_material_editor_shared_test)
            output = material_editor.get_output()
            return_code = material_editor.get_returncode()
            material_editor_log_content = editor_utils.retrieve_material_editor_log_content(run_id, log_name, workspace)
            # Save the MaterialEditor log
            try:
                path_to_artifact = os.path.join(
                    editor_utils.retrieve_material_editor_log_path(run_id, workspace), log_name)
                destination_path = workspace.artifact_manager.save_artifact(path_to_artifact)
                editor_utils.split_batched_editor_log_file(workspace, path_to_artifact, destination_path)
            except FileNotFoundError:
                # Error logging is already performed and we don't want this to fail the test
                pass
            if return_code == 0:
                # No need to scrape the output, as all the tests have passed
                for test_spec in test_spec_list:
                    results[test_spec.__name__] = MaterialEditorResult.Pass(test_spec, output, material_editor_log_content)
            else:
                # Scrape the output to attempt to find out which tests failed.
                # This function should always populate the result list,
                # if it didn't find it, it will have "Unknown" type of result
                results = self._get_results_using_output(test_spec_list, output, material_editor_log_content)
                assert len(results) == len(
                    test_spec_list), "bug in get_results_using_output(), the number of results don't match the tests ran"

                # If the MaterialEditor crashed, find out in which test it happened and update the results
                has_crashed = return_code != MaterialEditorTestSuite._TEST_FAIL_RETCODE
                if has_crashed:
                    crashed_result = None
                    for test_spec_name, result in results.items():
                        if isinstance(result, MaterialEditorResult.Unknown):
                            if not crashed_result:
                                # The first test with "Unknown" result (no output data) is likely the one that crashed
                                crash_error = editor_utils.retrieve_crash_output(
                                    run_id, workspace, self._TIMEOUT_CRASH_LOG)
                                # Save the .dmp file which is generated on Windows only
                                dmp_file_name = os.path.join(
                                    editor_utils.retrieve_material_editor_log_path(run_id, workspace), 'error.dmp')
                                if os.path.exists(dmp_file_name):
                                    workspace.artifact_manager.save_artifact(dmp_file_name)
                                # Save the crash log
                                crash_file_name = os.path.join(
                                    editor_utils.retrieve_material_editor_log_path(run_id, workspace),
                                    os.path.basename(workspace.paths.crash_log()))
                                if os.path.exists(crash_file_name):
                                    workspace.artifact_manager.save_artifact(crash_file_name)
                                    editor_utils.cycle_crash_report(run_id, workspace)
                                else:
                                    logger.warning(f"Crash occurred, but could not find log {crash_file_name}")
                                results[test_spec_name] = MaterialEditorResult.Crash(
                                    result.test_spec, output, return_code, crash_error, result.material_editor_log)
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
                        results[test_spec_name] = MaterialEditorResult.Crash(
                            crashed_result.test_spec, output, return_code, crash_error,
                            crashed_result.material_editor_log)
        except WaitTimeoutError:
            material_editor.stop()
            output = material_editor.get_output()
            material_editor_log_content = editor_utils.retrieve_material_editor_log_content(run_id, log_name, workspace)

            # The MaterialEditor timed out when running the tests,
            # get the data from the output to find out which ones ran
            results = self._get_results_using_output(test_spec_list, output, material_editor_log_content)
            assert len(results) == len(
                test_spec_list), "bug in _get_results_using_output(), the number of results don't match the tests ran"
            # Similar logic here as crashes, the first test that has no result is the one that timed out
            timed_out_result = None
            for test_spec_name, result in results.items():
                if isinstance(result, MaterialEditorResult.Unknown):
                    if not timed_out_result:
                        results[test_spec_name] = MaterialEditorResult.Timeout(result.test_spec, result.output,
                                                                 self.timeout_material_editor_shared_test,
                                                                 result.material_editor_log)
                        timed_out_result = result
                    else:
                        # If there are remaning "Unknown" results, these couldn't execute because of the timeout,
                        # update with info about the offender
                        results[test_spec_name].extra_info = f"This test has unknown result, test " \
                                                             f"'{timed_out_result.test_spec.__name__}' timed out " \
                                                             f"before this test could be executed"
            # if all the tests ran, the one that has caused the timeout is the last test,
            # as it didn't close the MaterialEditor
            if not timed_out_result:
                results[test_spec_name] = MaterialEditorResult.Timeout(timed_out_result.test_spec,
                                                         results[test_spec_name].output,
                                                         self.timeout_material_editor_shared_test,
                                                         result.material_editor_log)
        finally:
            if temp_batched_file:
                os.unlink(temp_batched_file.name)
        return results

    def _run_single_test(self,
                         request: _pytest.fixtures.FixtureRequest,
                         workspace: AbstractWorkspaceManager,
                         collected_test_data: AbstractTestSuite.TestData,
                         test_spec: MaterialEditorSingleTest) -> None:
        """
        Runs a single test (one MaterialEditor, one test) with the given specs
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :collected_test_data: The TestData from calling collected_test_data()
        :test_spec: The test class that should be a subclass of MaterialEditorSingleTest
        :return: None
        """
        self._setup_test_run(workspace, collected_test_data)
        material_editor = launcher_helper.create_material_editor(workspace)
        material_editor.configure_settings()
        extra_cmdline_args = []
        if hasattr(test_spec, "extra_cmdline_args"):
            extra_cmdline_args = test_spec.extra_cmdline_args

        result = self._exec_material_editor_test(request, workspace, 1, f"{LOG_NAME}", test_spec, extra_cmdline_args)
        if result is None:
            logger.error(f"Unexpectedly found no test run in the {LOG_NAME} during {test_spec}")
            result = {"Unknown":
                      MaterialEditorResult.Unknown(
                          test_spec=test_spec,
                          extra_info=f"Unexpectedly found no test run information on stdout in the {LOG_NAME}")}
        collected_test_data.results.update(result)
        test_name, test_result = next(iter(result.items()))
        self._report_result(test_name, test_result)
        # If test did not pass, save assets with errors and warnings
        if not isinstance(test_result, MaterialEditorResult.Pass):
            editor_utils.save_failed_asset_joblogs(workspace)

    def _run_batched_tests(self, request: _pytest.fixtures.FixtureRequest,
                           workspace: AbstractWorkspaceManager,
                           collected_test_data: AbstractTestSuite.TestData,
                           test_spec_list: list[MaterialEditorSharedTest],
                           extra_cmdline_args: list[str] = None) -> None:
        """
        Runs a batch of tests in one single MaterialEditor with the given spec list (one MaterialEditor, multiple tests)
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :collected_test_data: The TestData from calling collected_test_data()
        :test_spec_list: A list of MaterialEditorSharedTest tests to run
        :extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if extra_cmdline_args is None:
            extra_cmdline_args = []

        if not test_spec_list:
            return

        self._setup_test_run(workspace, collected_test_data)
        material_editor = launcher_helper.create_material_editor(workspace)
        material_editor.configure_settings()
        results = self._exec_material_editor_multitest(
            request, workspace, 1, f"{LOG_NAME}", test_spec_list, extra_cmdline_args)
        collected_test_data.results.update(results)
        # If at least one test did not pass, save assets with errors and warnings
        for result in results:
            if result is None:
                logger.error(f"Unexpectedly found no test run in the {LOG_NAME} during MaterialEditorBatchedTest")
                logger.debug(f"Results from MaterialEditorBatchedTest:\n{results}")
            if not isinstance(result, MaterialEditorResult.Pass):
                editor_utils.save_failed_asset_joblogs(workspace)
                return  # exit early on first batch failure

    def _run_parallel_tests(self,
                            request: _pytest.fixtures.FixtureRequest,
                            workspace: AbstractWorkspaceManager,
                            material_editor: ly_test_tools.launchers.platforms.base.Launcher,
                            collected_test_data: AbstractTestSuite.TestData,
                            test_spec_list: list[MaterialEditorSharedTest],
                            extra_cmdline_args: list[str] = None) -> None:
        """
        Runs multiple MaterialEditors with one test on each MaterialEditor (multiple MaterialEditors, one test each)
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :material_editor: The LyTestTools MaterialEditor object
        :collected_test_data: The TestData from calling collected_test_data()
        :test_spec_list: A list of MaterialEditorSharedTest tests to run
        :extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if extra_cmdline_args is None:
            extra_cmdline_args = []

        if not test_spec_list:
            return

        self._setup_test_run(workspace, collected_test_data)
        material_editor.configure_settings()
        parallel_material_editors = self._get_number_parallel_instances(request)
        assert parallel_material_editors > 0, "Must have at least one MaterialEditor"

        # If there are more tests than max parallel MaterialEditors, we will split them into multiple consecutive runs
        num_iterations = int(math.ceil(len(test_spec_list) / parallel_material_editors))
        for iteration in range(num_iterations):
            tests_for_iteration = test_spec_list[
                                  iteration * parallel_material_editors:(iteration + 1) * parallel_material_editors]
            total_threads = len(tests_for_iteration)
            threads = []
            results_per_thread = [None] * total_threads
            for i in range(total_threads):
                def make_func(test_spec, index):
                    def run(request, workspace, extra_cmdline_args):
                        results = self._exec_material_editor_test(
                            request, workspace, index + 1, f"{LOG_NAME}", test_spec, extra_cmdline_args)
                        assert results is not None
                        results_per_thread[index] = results

                    return run

                # Duplicate the MaterialEditors using the one coming from the fixture
                cur_material_editor = material_editor.__class__(workspace, material_editor.args.copy())
                parallel_run_test = make_func(tests_for_iteration[i], i, cur_material_editor)
                parallel_run_thread = threading.Thread(
                    target=parallel_run_test, args=(request, workspace, extra_cmdline_args))
                parallel_run_thread.start()
                threads.append(parallel_run_thread)

            for t in threads:
                t.join()

            save_asset_logs = False

            for result in results_per_thread:
                if result is None:
                    logger.error(f"Unexpectedly found no test run in the {LOG_NAME} during MaterialEditorParallelTest")
                    logger.debug(f"Results from MaterialEditorParallelTest thread:\n{results_per_thread}")
                    result = {"Unknown":
                              MaterialEditorResult.Unknown(
                                  test_spec=MaterialEditorParallelTest,
                                  extra_info=f"Unexpectedly found no test run information on stdout in the {LOG_NAME}")}
                collected_test_data.results.update(result)
                if not isinstance(result, MaterialEditorResult.Pass):
                    save_asset_logs = True
            # If at least one test did not pass, save assets with errors and warnings
            if save_asset_logs:
                editor_utils.save_failed_asset_joblogs(workspace)

    def _run_parallel_batched_tests(self,
                                    request: _pytest.fixtures.FixtureRequest,
                                    workspace: AbstractWorkspaceManager,
                                    collected_test_data: AbstractTestSuite.TestData,
                                    test_spec_list: list[MaterialEditorSharedTest],
                                    extra_cmdline_args: list[str] = None) -> None:
        """
        Runs multiple editors with a batch of tests for each MaterialEditor (multiple instances, multiple tests each)
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :collected_test_data: The TestData from calling collected_test_data()
        :test_spec_list: A list of MaterialEditorSharedTest tests to run
        :extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if extra_cmdline_args is None:
            extra_cmdline_args = []

        if not test_spec_list:
            return

        self._setup_test_run(workspace, collected_test_data)
        material_editor = launcher_helper.create_material_editor(workspace)
        material_editor.configure_settings()
        total_threads = self._get_number_parallel_instances(request)
        assert total_threads > 0, "Must have at least one MaterialEditor"
        threads = []
        tests_per_material_editor = int(math.ceil(len(test_spec_list) / total_threads))
        results_per_thread = [None] * total_threads
        for i in range(total_threads):
            tests_for_thread = test_spec_list[i * tests_per_material_editor:(i + 1) * tests_per_material_editor]

            def make_func(test_spec_list_for_material_editor, index):
                def run(request, workspace, extra_cmdline_args):
                    results = None
                    if len(test_spec_list_for_material_editor) > 0:
                        results = self._exec_material_editor_multitest(
                            request, workspace, index + 1, f"{LOG_NAME}", test_spec_list_for_material_editor,
                            extra_cmdline_args)
                        assert results is not None
                    else:
                        results = {}
                    results_per_thread[index] = results

                return run

            # Duplicate the MaterialEditor using the one coming from the fixture
            cur_material_editor = material_editor.__class__(workspace, material_editor.args.copy())
            f = make_func(tests_for_thread, i, cur_material_editor)
            t = threading.Thread(target=f, args=(request, workspace, extra_cmdline_args))
            t.start()
            threads.append(t)

        for t in threads:
            t.join()

        save_asset_logs = False
        for result in results_per_thread:
            if result is None:
                logger.error(f"Unexpectedly found no test run in the {LOG_NAME} during MaterialEditorSharedTest")
                logger.debug(f"Results from MaterialEditorSharedTest thread:\n{results_per_thread}")
                result = {"Unknown":
                          MaterialEditorResult.Unknown(
                              test_spec=MaterialEditorSharedTest,
                              extra_info=f"Unexpectedly found no test run information on stdout in the {LOG_NAME}")}
            collected_test_data.results.update(result)
            if not isinstance(result, MaterialEditorResult.Pass):
                save_asset_logs = True
        # If at least one test did not pass, save assets with errors and warnings
        if save_asset_logs:
            editor_utils.save_failed_asset_joblogs(workspace)
