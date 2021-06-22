from base import TestAutomationBase
import pytest
import logging
import inspect
from typing import List
from abc import ABC
import os, sys
import threading
import math
import json

import ly_test_tools.environment.file_system as file_system
import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.waiter as waiter

from ly_test_tools.o3de.asset_processor import AssetProcessor
from ly_test_tools.launchers.exceptions import WaitTimeoutError
from ly_test_tools.log.log_monitor import LogMonitor, LogMonitorException

class EditorUtils():

    @staticmethod
    def kill_all_ly_processes(include_asset_processor=True):
        LY_PROCESSES = [
            'Editor', 'Profiler', 'RemoteConsole',
        ]
        AP_PROCESSES = [
            'AssetProcessor', 'AssetProcessorBatch', 'AssetBuilder', 'CrySCompileServer',
            'rc'  # Resource Compiler
        ]
        
        if include_asset_processor:
            process_utils.kill_processes_named(LY_PROCESSES+AP_PROCESSES, ignore_extensions=True)
        else:
            process_utils.kill_processes_named(LY_PROCESSES, ignore_extensions=True)

    @staticmethod
    def get_testcase_module_filepath(testcase_module):
        # type: (Module) -> str
        """
        return the full path of the test module
        :param testcase_module: The testcase python module being tested
        :return str: The full path to the testcase module
        """
        return os.path.splitext(testcase_module.__file__)[0] + ".py"

    @staticmethod
    def get_testcase_module_name(testcase_module):
        # type: (Module) -> str
        """
        return the full path of the test module
        :param testcase_module: The testcase python module being tested
        :return str: The full path to the testcase module
        """
        return os.path.basename(os.path.splitext(testcase_module.__file__)[0])

    @staticmethod
    def retrieve_user_path(run_id : int, workspace):
        return os.path.join(workspace.paths.project(), f"user_test_{run_id}")
    
    @staticmethod
    def retrieve_log_path(run_id : int, workspace):
        return os.path.join(EditorUtils.retrieve_user_path(run_id, workspace), "log")
    
    @staticmethod
    def retrieve_crash_output(run_id : int, workspace, timeout):
        crash_info = "-- No crash log available --"
        crash_log = os.path.join(EditorUtils.retrieve_log_path(run_id, workspace), 'error.log')
        try:
            waiter.wait_for(lambda: os.path.exists(crash_log), timeout=timeout)
        except AssertionError:                    
            pass
            
        try:
            with open(crash_log) as f:
                crash_info = f.read()
        except Exception as ex:
            crash_info += f"\n{str(ex)}"
        return crash_info

    @staticmethod
    def delete_crash_report(run_id : int, workspace):
        crash_log = os.path.join(EditorUtils.retrieve_log_path(run_id, workspace), 'error.log')
        if os.path.exists(crash_log):
            os.remove(crash_log)

    @staticmethod
    def retrieve_editor_log_content(run_id : int, log_name : str, workspace, timeout=10):
        editor_info = "-- No editor log available --"
        editor_log = os.path.join(EditorUtils.retrieve_log_path(run_id, workspace), log_name)
        try:
            waiter.wait_for(lambda: os.path.exists(editor_log), timeout=timeout)
        except AssertionError:              
            pass
            
        try:
            with open(editor_log) as f:
                editor_info = ""
                for line in f:
                    editor_info += f"[editor.log]  {line}"
        except Exception as ex:
            editor_info = f"-- Error reading editor.log: {str(ex)} --"
        return editor_info

    @staticmethod
    def retrieve_last_run_test_index_from_output(test_spec_list, output : str):
        index = -1
        find_pos = 0
        for test_spec in test_spec_list:
            find_pos = output.find(test_spec.__name__, find_pos)
            if find_pos == -1:
                index = max(index, 0) # <- if we didn't even find the first test, assume its been the first one that crashed
                return index
            else:
                index += 1
        return index

class EditorTestBase(ABC):
    # Test file that this test will run
    test_module = None
    # Maximum time for run, in seconds
    timeout = 180

# Test that will be run alone in one editor
class EditorSingleTest(EditorTestBase):
    # Extra cmdline arguments to supply to the editor for the test
    extra_cmdline_args = []

# Test that will be run in parallel and/or batched with other in a single editor.
# Does not support per test setup/teardown for avoiding race conditions
class EditorSharedTest(EditorTestBase):
    # Specifies if the test can be run in multiple editors in parallel
    is_parallelizable = True
    # Specifies if the test can be batched in the same editor
    is_batchable = True

@pytest.mark.parametrize("crash_log_watchdog", [("raise_on_crash", False)])
class EditorTestSuite():
    #- Configurable params -#
    # Extra cmdline arguments to supply for every editor instance for this test suite
    global_extra_cmdline_args = ["-BatchMode", "-autotest_mode", "-rhi=null"]

    # Function to calculate number of editors to run in parallel, this can be overriden by the user
    @staticmethod
    def get_number_parallel_editors():
        return 8

    ## Internal ##
    TIMEOUT_CRASH_LOG = 20 # Maximum time for waiting for a crash log, in secondss

    _TEST_FAIL_RETCODE = 0xF # Return code for test failure 
    _asset_processor = None
    _results = {}

    # Specifies a Test result
    class Result:
        class Base:
            def get_output_str(self):
                if hasattr(self, "output") and self.output is not None:
                    return self.output
                else:
                    return "-- No output --"
            
            def get_editor_log_str(self):
                if hasattr(self, "editor_log") and self.editor_log is not None:
                    return self.editor_log
                else:
                    return "-- No editor log found --"

        class Pass(Base):
            @classmethod
            def create(cls, output : str, editor_log : str):
                r = cls()
                r.output = output
                r.editor_log = editor_log
                return r

            def __str__(self):
                output = (
                    f"Test Passed\n"
                    f"------------\n"
                    f"|  Output  |\n"
                    f"------------\n"
                    f"{self.get_output_str()}\n"
                )
                return output

        class Fail(Base):       
            @classmethod
            def create(cls, output, editor_log : str):
                r = cls()
                r.output = output
                r.editor_log = editor_log
                return r
            
            def __str__(self):
                output = (
                    f"Test FAILED\n"
                    f"------------\n"
                    f"|  Output  |\n"
                    f"------------\n"
                    f"{self.get_output_str()}\n"
                    f"--------------\n"
                    f"| Editor log |\n"
                    f"--------------\n"
                    f"{self.get_editor_log_str()}\n"
                )
                return output

        class Crash(Base):
            @classmethod
            def create(cls, output : str, ret_code : int, stacktrace : str, editor_log : str):
                r = cls()
                r.output = output
                r.ret_code = ret_code
                r.stacktrace = stacktrace
                r.editor_log = editor_log
                return r
            
            def __str__(self):
                stacktrace_str = "-- No stacktrace data found --" if not self.stacktrace else self.stacktrace
                output = (
                    f"Test CRASHED, return code {hex(self.ret_code)}\n"
                    f"---------------\n"
                    f"|  Stacktrace |\n"
                    f"---------------\n"
                    f"{stacktrace_str}"
                    f"------------\n"
                    f"|  Output  |\n"
                    f"------------\n"
                    f"{self.get_output_str()}\n"
                    f"--------------\n"
                    f"| Editor log |\n"
                    f"--------------\n"
                    f"{self.get_editor_log_str()}\n"
                )
                crash_str = "-- No crash information found --"
                return output

        class Timeout(Base):
            @classmethod
            def create(cls, output : str, time_secs : float, editor_log : str):
                r = cls()
                r.output = output
                r.time_secs = time_secs
                r.editor_log = editor_log
                return r
            
            def __str__(self):
                output = (
                    f"Test TIMED OUT after {self.time_secs} seconds\n"
                    f"------------\n"
                    f"|  Output  |\n"
                    f"------------\n"
                    f"{self.get_output_str()}\n"
                    f"--------------\n"
                    f"| Editor log |\n"
                    f"--------------\n"
                    f"{self.get_editor_log_str()}\n"
                )
                return output

        class Unknown(Base):
            @classmethod
            def create(cls, output : str, extra_info : str, editor_log : str):
                r = cls()
                r.output = output
                r.editor_log = editor_log
                r.extra_info = extra_info
                return r
            
            def __str__(self):
                output = (
                    f"Unknown test result, possible cause: {self.extra_info}\n"
                    f"------------\n"
                    f"|  Output  |\n"
                    f"------------\n"
                    f"{self.get_output_str()}\n"
                    f"--------------\n"
                    f"| Editor log |\n"
                    f"--------------\n"
                    f"{self.get_editor_log_str()}\n"
                )
                return output

    # Custom collector class. This collector is where the magic happens, it programatically add the test functions
    # to the class based on the test specifications.
    class Collector(pytest.Class):
        def __init__(self, name, collector):
            super().__init__(name, collector)

        def collect(self):
            from inspect import getmembers, isclass

            assert sys.version_info >= (3, 7), "requires Python 3.7 newer"

            opt_batch_tests = self.config.getoption("--no-editor-batch", default=True)
            opt_parallel_tests = self.config.getoption("--no-editor-parallel", default=True)

            # Procedurally create single test functions using the specifications
            single_tests = [c for c in self.obj.__dict__.items() if isclass(c[1]) and issubclass(c[1], EditorSingleTest)]
            for name, test_spec in single_tests:
                def make_func(test_spec):
                    def test_run(self, request, workspace, editor, launcher_platform):
                        self._run_single_test(request, workspace, editor, test_spec)
                    return test_run
                    
                setattr(self.obj, name, make_func(test_spec))
            
            # Prepare the batched tests
            if opt_batch_tests:
                batched_tests = self.obj._get_shared_tests({'is_parallelizable' : False, 'is_batchable' : True})
                if len(batched_tests) > 0:
                    def test_all_batched(self, request, workspace, editor, launcher_platform):
                        self._run_batched_tests(request, workspace, editor, batched_tests)
                    setattr(self.obj, "test_all_batched", test_all_batched)
            
            # Prepare the parallel tests
            if opt_parallel_tests:
                parallel_tests = self.obj._get_shared_tests({'is_parallelizable' : True, 'is_batchable' : False})
                if len(parallel_tests) > 0:
                    def test_all_parallel(self, request, workspace, editor, launcher_platform):
                        self._run_parallel_tests(request, workspace, editor, parallel_tests)
                    setattr(self.obj, f"test_all_parallel", test_all_parallel)
                    
            # Prepare the parallel + batched tests 
            if opt_batch_tests and opt_parallel_tests:
                parallel_and_batchable_tests = self.obj._get_shared_tests({'is_parallelizable' : True, 'is_batchable' : True})
                if len(parallel_and_batchable_tests) > 0:
                    def test_all_parallel_and_batchable(self, request, workspace, editor, launcher_platform):
                        self._run_parallel_batched_tests(request, workspace, editor, parallel_and_batchable_tests)
                    setattr(self.obj, "test_all_parallel_and_batchable", test_all_parallel_and_batchable)

            # Now add the multi editor tests, these won't run the tests, but report their specific success/fail of the batched run
            # These tests must always run after the previous ones, which works since dictionaries are waranteed to be insertion ordered
            # from python 3.7
            multi_tests = [c for c in self.cls.__dict__.items() if isclass(c[1]) and issubclass(c[1], EditorSharedTest)]
            for name, test_spec in multi_tests:
                def make_func(name, test_spec):
                    def test_run(self, request, workspace, editor, launcher_platform):
                        if not hasattr(EditorTestSuite, "_results") or name not in EditorTestSuite._results:
                            # Run the test normally only if the matching test_all_* have not been collected for the run.
                            # This can happen if:
                            # 1) Specific test via: python -m pytest MyTests.py::MyTests::test_mytest
                            # 2) Forcing single tests via cmdline arg --no-editor-batch or --no-editor-parallel
                            func_names = [item.originalname for item in request.session.items]
                            run_test = False
                            if test_spec.is_parallelizable and test_spec.is_batchable:
                                run_test = "test_all_parallel_and_batchable" not in func_names
                            elif test_spec.is_batchable:
                                run_test = "test_all_batchable" not in func_names
                            elif test_spec.is_parallelizable:
                                run_test = "test_all_parallel" not in func_names
                            
                            if run_test:
                                self._run_single_test(request, workspace, editor, test_spec)
                            else:
                                pytest.fail(f"No run data for test: {test_spec.__name__}")
                        else:
                            EditorTestSuite._report_result(name, EditorTestSuite._results[name])
                    return test_run
            
                setattr(self.obj, name, make_func(name, test_spec))

            collection = super().collect()
            return collection


    @staticmethod
    def pytest_custom_makeitem(collector, name, obj):
        return EditorTestSuite.Collector(name, collector)

    def setup_class(cls):
        cls._results = {}
    
    def teardown_class(cls):
        if cls._asset_processor:
            cls._asset_processor.stop(1)
            cls._asset_processor.teardown()
            cls._asset_processor = None
            EditorUtils.kill_all_ly_processes(include_asset_processor=True)
        else:
            EditorUtils.kill_all_ly_processes(include_asset_processor=False)

    ### Utils ###
    def _prepare_asset_processor(self, workspace):
        try:
            # Start-up an asset processor if we are not running one
            # If another AP process exist, don't kill it, as we don't own it
            if EditorTestSuite._asset_processor is None:
                if not process_utils.process_exists("AssetProcessor", ignore_extensions=True):
                    EditorUtils.kill_all_ly_processes()
                    EditorTestSuite._asset_processor = AssetProcessor(workspace)
                    EditorTestSuite._asset_processor.start()
                else:
                    EditorUtils.kill_all_ly_processes(include_asset_processor=False)
            else:
                # Make sure the asset processor from before wasn't closed by accident
                EditorTestSuite._asset_processor.start()
        except Exception as ex:
            EditorTestSuite._asset_processor = None
            raise ex

    def _setup_editor_test(self, editor, workspace):
        self._prepare_asset_processor(workspace)
        EditorUtils.kill_all_ly_processes(include_asset_processor=False)
        editor.configure_settings()

    # Utility function for parsing the output information from the editor.
    # It deserializes the JSON content printed in the output for every test and returns that information
    @staticmethod
    def _get_results_using_output(test_spec_list, editor_log_content, output):
        Result = EditorTestSuite.Result
        START_TOKEN = "JSON_START("
        END_TOKEN = ")JSON_END"
        results = {}
        found_jsons = {}
        find_start = 0
        while find_start >= 0:
            find_start = output.find(START_TOKEN, find_start+1)
            if find_start >= 0:
                find_end = output.find(END_TOKEN, find_start)
                if find_end:
                    json_str = output[find_start+len(START_TOKEN):find_end]
                    try:
                        test_json = json.loads(json_str)
                        found_jsons[test_json["name"]] = test_json
                    except Exception as ex:
                        continue

        for test_spec in test_spec_list:
            name = EditorUtils.get_testcase_module_name(test_spec.test_module)
            if name not in found_jsons.keys():
                results[test_spec.__name__] = Result.Unknown.create(output, "??", editor_log_content)
            else:
                result = None
                json_result = found_jsons[name]
                json_output = json_result["output"]
                if json_result["success"]:
                    result = Result.Pass.create(json_output, editor_log_content)
                else:
                    result = Result.Fail.create(json_output, editor_log_content)
                results[test_spec.__name__] = result

        return results

    # Fails the test if the test result is not a PASS, specifying the information
    @staticmethod
    def _report_result(name : str, result : Result.Base):
        Result = EditorTestSuite.Result
        if isinstance(result, Result.Pass):
            output_str = f"Test {name}:\n{str(result)}"
            print(output_str)
        else:
            error_str = f"Test {name}:\n{str(result)}"
            pytest.fail(error_str)

    ### Running tests ###
    # Starts the editor with the given test and retuns an result dict with a single element specifying the result
    def _exec_editor_test(self, request, workspace, editor,
        run_id : int, log_name : str, test_spec : EditorTestBase, cmdline_args : List[str] = []):

        test_result = None
        results = {}
        test_filename = EditorUtils.get_testcase_module_filepath(test_spec.test_module)
        cmdline = [
            "--runpythontest", test_filename,
            "-logfile", f"@log@/{log_name}",
            "-project-user-path", EditorUtils.retrieve_user_path(run_id, workspace)] + self.global_extra_cmdline_args + cmdline_args
        editor.args.extend(cmdline)
        editor.start(backupFiles = False, launch_ap = False, configure_settings=False)

        try:
            editor.wait(test_spec.timeout)
            output = editor.get_output()
            return_code = editor.get_returncode()
            editor_log_content = EditorUtils.retrieve_editor_log_content(run_id, log_name, workspace)

            Result = EditorTestSuite.Result
            if return_code == 0:
                test_result = Result.Pass.create(output, editor_log_content)
            else:
                has_crashed = return_code != EditorTestSuite._TEST_FAIL_RETCODE
                if has_crashed:
                    test_result = Result.Crash.create(output, return_code, EditorUtils.retrieve_crash_output(run_id, workspace, self.TIMEOUT_CRASH_LOG), None)
                    EditorUtils.delete_crash_report(run_id, workspace)
                else:
                    test_result = Result.Fail.create(output, editor_log_content)
        except WaitTimeoutError:
            editor.kill()            
            editor_log_content = EditorUtils.retrieve_editor_log_content(run_id, log_name, workspace)
            test_result = Result.Timeout.create(output, test_spec.timeout, editor_log_content)
    
        editor_log_content = EditorUtils.retrieve_editor_log_content(run_id, log_name, workspace)
        results = self._get_results_using_output([test_spec], editor_log_content, output)
        results[test_spec.__name__] = test_result
        return results

    # Starts an editor executable with a list of tests and returns a dict of the result of every test ran within that editor
    # instance. In case of failure this function also parses the editor output to find out what specific tests that failed
    def _exec_editor_multitest(self, request, workspace, editor,
        run_id : int, log_name : str, test_spec_list : List[EditorTestBase], cmdline_args=[]):

        results = {}
        test_filenames_str = ";".join(EditorUtils.get_testcase_module_filepath(test_spec.test_module) for test_spec in test_spec_list)
        cmdline = [
            "--runpythontest", test_filenames_str,
            "-logfile", f"@log@/{log_name}",
            "-project-user-path", EditorUtils.retrieve_user_path(run_id, workspace)] + self.global_extra_cmdline_args + cmdline_args

        editor.args.extend(cmdline)
        editor.start(backupFiles = False, launch_ap = False, configure_settings=False)

        editor_log_content = ""
        total_timeout = sum(test_spec.timeout for test_spec in test_spec_list)
        try:
            editor.wait(total_timeout)
            output = editor.get_output()
            return_code = editor.get_returncode()
            editor_log_content = EditorUtils.retrieve_editor_log_content(run_id, log_name, workspace)

            Result = EditorTestSuite.Result
            if return_code == 0:
                # No need to scrap the output, as all the tests have passed
                for test_spec in test_spec_list:
                    results[test_spec.__name__] = Result.Pass.create(output, editor_log_content)
            else:
                results = self._get_results_using_output(test_spec_list, editor_log_content, output)
                has_crashed = return_code != EditorTestSuite._TEST_FAIL_RETCODE
                if has_crashed:
                    crashed_test = None
                    for key, result in results.items():
                        if isinstance(result, Result.Unknown):
                            if not crashed_test:
                                crash_error = EditorUtils.retrieve_crash_output(run_id, workspace, self.TIMEOUT_CRASH_LOG)
                                EditorUtils.delete_crash_report(run_id, workspace)
                                results[key] = Result.Crash.create(output, return_code, crash_error, editor_log_content)
                                crashed_test = results[key]
                            else:
                                results[key] = Result.Unknown.create(output, f"This test has unknown result, test '{crashed_test.__name__}' crashed before this test could be executed", editor_log_content)

        except WaitTimeoutError:
            results = self._get_results_using_output(test_spec_list, editor_log_content, output)
            editor_log_content = EditorUtils.retrieve_editor_log_content(run_id, log_name, workspace)
            editor.kill()
            for key, result in results.items():
                if isinstance(result, Result.Unknown):
                    results[key] = Result.Timeout.create(result.output, test_spec.timeout, editor_log_content)

        return results
    
    # Runs a single test with the given specs, used by the collector to register the test
    def _run_single_test(self, request, workspace, editor, test_spec : EditorTestBase):
        self._setup_editor_test(editor, workspace)
        extra_cmdline_args = []
        if hasattr(test_spec, "extra_cmdline_args"):
            extra_cmdline_args = test_spec.extra_cmdline_args

        results = self._exec_editor_test(request, workspace, editor, 1, "editor_test.log", test_spec, extra_cmdline_args)
        if not hasattr(EditorTestSuite, "results"):
            EditorTestSuite.results = {}

        EditorTestSuite._results.update(results)
        test_name, test_result = next(iter(results.items()))
        self._report_result(test_name, test_result)

    # Runs a batch of tests in one single editor with the given spec list
    def _run_batched_tests(self, request, workspace, editor, test_spec_list : List[EditorTestBase], extra_cmdline_args=[]):
        self._setup_editor_test(editor, workspace)
        results = self._exec_editor_multitest(request, workspace, editor, 1, "editor_test.log", test_spec_list, extra_cmdline_args)
        if not hasattr(EditorTestSuite, "results"):
            EditorTestSuite.results = {}
            
        EditorTestSuite._results.update(results)

    # Runs multiple editors with one test on each editor
    def _run_parallel_tests(self, request, workspace, editor, test_spec_list : List[EditorTestBase], extra_cmdline_args=[]):
        self._setup_editor_test(editor, workspace)
        parallel_editors = self._get_number_parallel_editors(request)
        assert parallel_editors > 0, "Must have at least one editor"
        
        # If there are more tests than max parallel editors, we will split them into multiple consecutive runs
        num_iterations = int(math.ceil(len(test_spec_list) / parallel_editors))
        for iteration in range(num_iterations):
            tests_for_iteration = test_spec_list[iteration*parallel_editors:(iteration+1)*parallel_editors]
            total_threads = len(tests_for_iteration)
            threads = []
            results_per_thread = [None] * total_threads
            for i in range(total_threads):
                def make_func(test_spec, index, my_editor):
                    def run(request, workspace, extra_cmdline_args):
                        results = self._exec_editor_test(request, workspace, my_editor, index+1, f"editor_test.log", test_spec, extra_cmdline_args)
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

            for result in results_per_thread:
                EditorTestSuite._results.update(result)

    # Runs multiple editors with a batch of tests for each editor
    def _run_parallel_batched_tests(self, request, workspace, editor, test_spec_list : List[EditorTestBase], extra_cmdline_args=[]):
        self._setup_editor_test(editor, workspace)
        total_threads = self._get_number_parallel_editors(request)
        assert total_threads > 0, "Must have at least one editor"
        threads = []
        tests_per_editor = int(math.ceil(len(test_spec_list) / total_threads))
        results_per_thread = [None] * total_threads
        for i in range(total_threads):
            tests_for_thread = test_spec_list[i*tests_per_editor:(i+1)*tests_per_editor]
            def make_func(test_spec_list_for_editor, index, my_editor):
                def run(request, workspace, extra_cmdline_args):
                    if len(test_spec_list_for_editor) > 0:
                        results = self._exec_editor_multitest(request, workspace, my_editor, index+1, f"editor_test.log", test_spec_list_for_editor, extra_cmdline_args)
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

        for result in results_per_thread:
            EditorTestSuite._results.update(result)

    @classmethod
    def _get_shared_tests(cls, properties={}):
        from inspect import getmembers, isclass
        
        def matches_properties(test):
            for prop_name, prop_value in properties.items():
                if not hasattr(test, prop_name) or (getattr(test, prop_name) != prop_value):
                    return False
            return True

        tests = [c for c in cls.__dict__.values() if isclass(c) and issubclass(c, EditorSharedTest)]
        return [test for test in tests if matches_properties(test)]

    def _get_number_parallel_editors(self, request):
        parallel_editors_value = request.config.option.parallel_editors
        if parallel_editors_value:
            return int(parallel_editors_value)

        return self.get_number_parallel_editors()