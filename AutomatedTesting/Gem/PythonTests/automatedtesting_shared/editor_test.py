from base import TestAutomationBase
import pytest
import logging
import inspect
import os

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
    def retrieve_crash_output(workspace, timeout):
        crash_info = "-- No crash log available --"
        crash_log = os.path.join(workspace.paths.project_log(), 'error.log')
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
    def retrieve_editor_log_output(workspace, timeout=10):
        editor_info = "-- No editor log available --"
        editor_log = os.path.join(workspace.paths.project_log(), 'editor.log')
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
    def retrieve_last_run_test_index_from_output(test_spec_list, output):
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


# Test that will be run alone in one editor
class EditorSingleTest():
    # Test file that this test will run
    test_module = None
    # Maximum time for run, in seconds
    timeout = 180
    # Extra cmdline arguments to supply to the editor for the test
    extra_cmdline_args = []

# Test that will be run in parallel and/or batched with other in a single editor.
# Does not support per test setup/teardown for avoiding race conditions
class EditorSharedTest():
    #- Configurable params -#
    # Test file that this test will run
    test_module = None
    # Maximum time for run, in seconds
    timeout = 180
    # Specifies if the test can be run in multiple editors in parallel
    is_parallelizable = True
    # Specifies if the test can be batched in the same editor
    is_batchable = True


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
    asset_processor = None
    results = {}

    class Result():

        class Base():
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
            def create(cls, output, editor_log):
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
            def create(cls, output, editor_log):
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
            def create(cls, output, ret_code, stacktrace, editor_log):
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
            def create(cls, output, time_secs, editor_log):
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
            def create(cls, output, extra_info, editor_log):
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


    class Collector(pytest.Class):
        def __init__(self, name, collector):
            super().__init__(name, collector)

        def collect(self):
            from inspect import getmembers, isclass
            # Procedurally create test functions using the specifications
            single_tests = [c for c in getmembers(self.cls, isclass) if issubclass(c[1], EditorSingleTest)]
            for name, test_spec in single_tests:
                def make_func(test_spec):
                    def test_run(self, request, workspace, editor, launcher_platform):
                        return self._run_single_test(request, workspace, editor, test_spec)
                    return test_run
                    
                setattr(self.obj, name, make_func(test_spec))

            # Multi Editor tests
            batched_tests = self.obj._get_tests({'is_parallelizable' : False, 'is_batchable' : True})
            if len(batched_tests) > 0:
                def test_all_batched(self, request, workspace, editor, launcher_platform):
                    return self._run_batched_tests(request, workspace, editor, batched_tests)
                setattr(self.obj, "test_all_batched", test_all_batched)
            
            parallel_tests = self.obj._get_tests({'is_parallelizable' : True, 'is_batchable' : False})
            if len(parallel_tests) > 0:
                def test_all_parallel(self, request, workspace, editor, launcher_platform):
                    return self._run_parallel_tests(request, workspace, editor, parallel_tests)
                setattr(self.obj, "test_all_parallel", test_all_parallel)

            parallel_and_batchable_tests = self.obj._get_tests({'is_parallelizable' : True, 'is_batchable' : True})
            if len(parallel_and_batchable_tests) > 0:
                def test_all_parallel_and_batchable(self, request, workspace, editor, launcher_platform):
                    return self._run_parallel_batched_tests(request, workspace, editor, parallel_and_batchable_tests)
                setattr(self.obj, "test_all_parallel_and_batchable", test_all_parallel_and_batchable)

            # Now add the multi editor tests that only report success/fail based on their runtime
            # These tests must always run after the previous ones
            multi_tests = [c for c in getmembers(self.cls, isclass) if issubclass(c[1], EditorSharedTest)]
            for name, test_spec in multi_tests:
                def make_func(name, test_spec):
                    def test_run(self, request, workspace, editor, launcher_platform):
                        if not hasattr(EditorTestSuite, "results") or name not in EditorTestSuite.results:
                            # This test didn't run, attempt to run it
                            return self._run_single_test(request, workspace, editor, test_spec)
                        else:
                            EditorTestSuite._report_result(name, EditorTestSuite.results[name])
                    return test_run

                setattr(self.obj, name, make_func(name, test_spec))

            collection = super().collect()
            return collection


    @staticmethod
    def pytest_custom_makeitem(collector, name, obj):
        return EditorTestSuite.Collector(name, collector)

    def teardown_class(cls):
        if cls.asset_processor:
            cls.asset_processor.teardown()
            cls.asset_processor = None
        EditorUtils.kill_all_ly_processes()

    def _prepare_asset_processor(self, workspace):
        # Start-up an asset processor if we are not running one
        try:
            if EditorTestSuite.asset_processor is None:
                EditorUtils.kill_all_ly_processes()
                EditorTestSuite.asset_processor = AssetProcessor(workspace)
                EditorTestSuite.asset_processor.backup_ap_settings()
                EditorTestSuite.asset_processor.start()
                EditorTestSuite.asset_processor.wait_for_idle()
            else:
                # Make sure the asset processor from before wasn't closed by accident
                EditorTestSuite.asset_processor.start()
                EditorTestSuite.asset_processor.wait_for_idle()
        except Exception as ex:
            EditorTestSuite.asset_processor = None
            raise ex

    def _setup_editor_test(self, workspace):
        self._prepare_asset_processor(workspace)
        EditorUtils.kill_all_ly_processes(include_asset_processor=False)

    def _run_editor_test(self, request, workspace, editor, test_spec, cmdline_args=[]):
        test_result = None
        results = {}
        test_filename = EditorUtils.get_testcase_module_filepath(test_spec.test_module)
        cmdline = ["--runpythontest", test_filename] + self.global_extra_cmdline_args + cmdline_args
        editor.args.extend(cmdline)
        editor.start(backupFiles = False, launch_ap = False)

        try:
            editor.wait(test_spec.timeout)
            output = editor.get_output()
            return_code = editor.get_returncode()
            editor_log_output = EditorUtils.retrieve_editor_log_output(workspace)

            Result = EditorTestSuite.Result
            if return_code == 0:
                test_result = Result.Pass.create(output, editor_log_output)
            else:
                has_crashed = return_code != EditorTestSuite._TEST_FAIL_RETCODE
                if has_crashed:
                    test_result = Result.Crash.create(output, return_code, EditorUtils.retrieve_crash_output(workspace, self.TIMEOUT_CRASH_LOG), None)
                else:
                    test_result = Result.Fail.create(output, editor_log_output)
        except WaitTimeoutError:
            editor.kill()            
            editor_log_output = EditorUtils.retrieve_editor_log_output(workspace)
            test_result = Result.Timeout.create(output, test_spec.timeout, editor_log_output)
    
        results = self._get_results_using_output([test_spec], workspace, output)
        print(results)
        results[test_spec.__name__] = test_result
        return results

    @staticmethod
    def _get_results_using_output(test_spec_list, workspace, output):
        Result = EditorTestSuite.Result
        editor_log_output = EditorUtils.retrieve_editor_log_output(workspace)
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
                        print(json_str)
                        test_json = json.loads(json_str)
                        found_jsons[test_json["name"]] = test_json
                    except Exception as ex:
                        continue

        for test_spec in test_spec_list:
            name = EditorUtils.get_testcase_module_name(test_spec.test_module)
            print(name)
            if name not in found_jsons.keys():
                results[test_spec.__name__] = Result.Unknown.create(output, "??", editor_log_output)
            else:
                result = None
                json_result = found_jsons[name]
                json_output = json_result["output"]
                if json_result["success"]:
                    result = Result.Pass.create(json_output, editor_log_output)
                else:
                    result = Result.Fail.create(json_output, editor_log_output)
                results[test_spec.__name__] = result

        return results

    def _run_editor_multitest(self, request, workspace, editor, test_spec_list, cmdline_args=[]):
        ## Run editor ##
        global_errors = []
        results = {}
        test_filenames_str = ";".join(EditorUtils.get_testcase_module_filepath(test_spec.test_module) for test_spec in test_spec_list)
        cmdline = ["--runpythontest", test_filenames_str] + self.global_extra_cmdline_args + cmdline_args
        editor.args.extend(cmdline)
        editor.start(backupFiles = False, launch_ap = False)

        total_timeout = sum(test_spec.timeout for test_spec in test_spec_list)
        try:
            editor.wait(total_timeout)
            output = editor.get_output()
            return_code = editor.get_returncode()

            Result = EditorTestSuite.Result
            if return_code == 0 and not global_errors:
                # No need to scrap the output, as all the tests have passed
                for test_spec in test_spec_list:
                    results[test_spec.__name__] = Result(Result.PASS, [])
            else:
                has_crashed = return_code != EditorTestSuite._TEST_FAIL_RETCODE
                if has_crashed:
                    crash_index = EditorUtils.retrieve_last_run_test_index_from_output(test_spec_list, output)
                    crash_error = EditorUtils.retrieve_crash_output(workspace, self.TIMEOUT_CRASH_LOG)
                    crashed_test = test_spec_list[crash_index]
                    results[crashed_test.__name__] = Result.Crash.create(output, return_code, crash_error, None)
                    for unknown_test in test_spec_list[crash_index+1:]:
                        results[unknown_test.__name__] = Result.Unknown.create(output, f"This test has unknown result, test '{crashed_test.__name__}' crashed before this test could be executed")
                else:
                    results = self._get_results_using_output(test_spec_list, workspace, output)

        except WaitTimeoutError:
            results = self._get_results_using_output(test_spec_list, workspace, output)

        return results
    
    @staticmethod
    def _report_result(name, result):
        Result = EditorTestSuite.Result
        if not isinstance(result, Result.Pass):
            error_str = f"Test {name}:\n{str(result)}"
            pytest.fail(error_str)

    def _run_single_test(self, request, workspace, editor, test_spec):
        self._setup_editor_test(workspace)
        extra_cmdline_args = []
        if hasattr(test_spec, "extra_cmdline_args"):
            extra_cmdline_args = test_spec.extra_cmdline_args

        results = self._run_editor_test(request, workspace, editor, test_spec, extra_cmdline_args)
        if not hasattr(EditorTestSuite, "results"):
            EditorTestSuite.results = {}

        EditorTestSuite.results.update(results)
        test_name, test_result = next(iter(results.items()))
        self._report_result(test_name, test_result)

    def _run_batched_tests(self, request, workspace, editor, test_spec_list, extra_cmdline_args=[]):
        self._setup_editor_test(workspace)
        results = self._run_editor_multitest(request, workspace, editor, test_spec_list)
        if not hasattr(EditorTestSuite, "results"):
            EditorTestSuite.results = {}
            
        EditorTestSuite.results.update(results)
        for test_name, test_result in results.items():
            self._report_result(test_name, test_result)

    def _run_parallel_tests(self, request, workspace, editor, test_spec_list, extra_cmdline_args=[]):
        self._setup_editor_test(workspace)
        

        for test in test_spec_list:

        results = self._run_editor_multitest(request, workspace, editor, test_spec_list)
        if not hasattr(EditorTestSuite, "results"):
            EditorTestSuite.results = {}
            
        EditorTestSuite.results.update(results)
        for test_name, test_result in results.items():
            self._report_result(test_name, test_result)

    def _run_parallel_batched_tests(self, request, workspace, editor, testmodule_list_per_editor, extra_cmdline_args=[]):
        pass

    @classmethod
    def _get_tests(cls, properties={}):
        from inspect import getmembers, isclass
        
        def matches_properties(test):
            for prop_name, prop_value in properties.items():
                if not hasattr(test, prop_name) or (getattr(test, prop_name) != prop_value):
                    return False
            return True

        tests = [c for c in getmembers(cls, isclass) if issubclass(c[1], EditorSharedTest)]
        return [test[1] for test in tests if matches_properties(test[1])]