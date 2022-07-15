"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# Simplified O3DE test-writing utilities.
#
# Supports different options for running tests using this framework:
# SingleTest: A single test that runs 1 test in 1 instance/application.
# SharedTest: Multiple tests that run multiple tests in multiple instances/applications.
# BatchedTest: Multiple tests that run 1 instance/application with multiple tests in that 1 instance/application.
# ParallelTest: Multiple tests that run multiple instances/applications with 1 test in each instance/application.
#
# It is recommended that new modules are created with objects that inherit from objects here for new tests.
# Example: creating a new EditorTestSuite(AbstractTestSuite) class for testing the Editor application.

from __future__ import annotations
__test__ = False  # Avoid pytest collection & warnings since this module is for test functions, but not a test itself.

import abc
import functools
import json
import inspect
import logging
import math
import os
import re
import types
import warnings

import pytest
import _pytest.python
import _pytest.outcomes
from _pytest.skipping import pytest_runtest_setup as skipping_pytest_runtest_setup

import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.o3de.editor_test_utils as editor_utils
from ly_test_tools._internal.managers.workspace import AbstractWorkspaceManager
from ly_test_tools.o3de.asset_processor import AssetProcessor

logger = logging.getLogger(__name__)


class AbstractTestBase(abc.ABC):
    """
    Abstract base Test class
    """
    # Maximum time for run, in seconds.
    timeout = 180
    # Test file that this test will run.
    test_module = None
    # Attach debugger when running the test, useful for debugging crashes. This should never be True on production.
    # It's also recommended to switch to SingleTest for debugging in isolation.
    attach_debugger = False
    # Wait until a debugger is attached at the startup of the test, this is another way of debugging.
    wait_for_debugger = False


class SingleTest(AbstractTestBase):
    """
    Test that will be run alone in one instance, with no parallel or serially batched instances.
    This should only be used if SharedTest, ParallelTest, or BatchedTest cannot be utilized for the test steps.
    It may also be used for debugging test issues in isolation.
    """
    def __init__(self):
        # Extra cmdline arguments to supply to the instance for the test.
        self.extra_cmdline_args = []
        # Whether to use null renderer, this will override use_null_renderer for the BaseTestSuite if not None.
        self.use_null_renderer = None

    def setup(self, **args):
        """
        User-overrideable setup function, which will run before the test
        """
        pass

    def wrap_run(self, **args):
        """
        User-overrideable wrapper function, which will run before and after test.
        Any code before the 'yield' statement will run before the test. With code after yield run after the test.
        """
        yield

    def teardown(self, **args):
        """
        User-overrideable teardown function, which will run after the test
        """
        pass


class SharedTest(AbstractTestBase):
    """
    Runs both of the following test types:
    1. Tests that will be run in parallel with tests in multiple instances.
    2. Tests that will be run as serially batched with other tests in a single instance.

    Minimizes total test run duration by reducing repeated overhead from starting the instance.
    Does not support per test setup/teardown to avoid creating race conditions.
    """
    # Specifies if the test can be batched in the same instance.
    is_batchable = True
    # Specifies if the test can be run in multiple instances in parallel.
    is_parallelizable = True


class ParallelTest(SharedTest):
    """Test that will be run in parallel with tests in multiple instances."""
    is_batchable = False
    is_parallelizable = True


class BatchedTest(SharedTest):
    """Test that will be run as serially batched with other tests in a single instance."""
    is_batchable = True
    is_parallelizable = False


class TestResultException(Exception):
    """Indicates that an unknown result was found during the tests"""
    pass


class Result(object):
    """Holds test results for a given program/application."""
    log_attribute = ''

    class ResultType(abc.ABC):
        """Generic result-type for data shared among results"""

        @abc.abstractmethod
        def __str__(self):
            # type () -> str
            return ""

        def get_output_str(self):
            # type () -> str
            """
            Checks if the output attribute exists and returns it.
            :return: Output string from running a test, or a no output message
            """
            output = getattr(self, "output", None)
            if output:
                return output
            else:
                return "-- No output --"

        def get_log_attribute_str(self):
            # type () -> str
            """
            Checks if the log_attribute exists and returns it.
            :return: Either the log_attribute string or a no output message
            """
            log = getattr(self, Result.log_attribute, None)
            if log:
                return log
            else:
                return "-- No log found --"

    class Pass(ResultType):

        def __init__(self, test_spec: type(AbstractTestBase), output: str, log_output: str):
            """
            Represents a test success
            :test_spec: The type of test class
            :output: The test output
            :log_output: The program's log output
            """
            self.test_spec = test_spec
            self.output = output
            self.log_output = log_output

        def __str__(self):
            output = (
                "Test Passed\n"
                "------------\n"
                "|  Output  |\n"
                "------------\n"
                f"{self.get_output_str()}\n"
            )
            return output

    class Fail(ResultType):

        def __init__(self, test_spec: type(AbstractTestBase), output: str, log_output: str):
            """
            Represents a normal test failure
            :test_spec: The type of test class
            :output: The test output
            :log_output: The program's log output
            """
            self.test_spec = test_spec
            self.output = output
            self.log_output = log_output

        def __str__(self):
            output = (
                "Test FAILED\n"
                "------------\n"
                "|  Output  |\n"
                "------------\n"
                f"{self.get_output_str()}\n"
                "-------------------------------\n"
                "|  Program (i.e. Editor) log  |\n"
                "-------------------------------\n"
                f"{self.get_log_attribute_str()}\n"
            )
            return output

    class Crash(ResultType):

        def __init__(self,
                     test_spec: type(AbstractTestBase),
                     output: str,
                     ret_code: int,
                     stacktrace: str,
                     log_output: str or None) -> None:
            """
            Represents a test which failed with an unexpected crash
            :test_spec: The type of test class
            :output: The test output
            :ret_code: The test's return code
            :stacktrace: The test's stacktrace if available
            :log_output: The program's log output
            """
            self.output = output
            self.test_spec = test_spec
            self.ret_code = ret_code
            self.stacktrace = stacktrace
            self.log_output = log_output

        def __str__(self):
            stacktrace_str = "-- No stacktrace data found --" if not self.stacktrace else self.stacktrace
            output = (
                f"Test CRASHED, return code {hex(self.ret_code)}\n"
                "---------------\n"
                "|  Stacktrace |\n"
                "---------------\n"
                f"{stacktrace_str}"
                "------------\n"
                "|  Output  |\n"
                "------------\n"
                f"{self.get_output_str()}\n"
                "-------------------------------\n"
                "|  Program (i.e. Editor) log  |\n"
                "------------------------------\n"
                f"{self.get_log_attribute_str()}\n"
            )
            return output

    class Timeout(ResultType):

        def __init__(self, test_spec: type(AbstractTestBase), output: str, time_secs: float, log_output: str):
            """
            Represents a test which failed due to freezing, hanging, or executing slowly
            :test_spec: The type of test class
            :output: The test output
            :time_secs: The timeout duration in seconds
            :log_output: The program's log output
            :return: The Timeout object
            """
            self.output = output
            self.test_spec = test_spec
            self.time_secs = time_secs
            self.log_output = log_output

        def __str__(self):
            output = (
                f"Test ABORTED after not completing within {self.time_secs} seconds\n"
                "------------\n"
                "|  Output  |\n"
                "------------\n"
                f"{self.get_output_str()}\n"
                "-------------------------------\n"
                "|  Program (i.e. Editor) log  |\n"
                "-------------------------------\n"
                f"{self.get_log_attribute_str()}\n"
            )
            return output

    class Unknown(ResultType):

        def __init__(self, test_spec: type(AbstractTestBase), output: str = None, extra_info: str = None,
                     log_output: str = None):
            """
            Represents a failure that the test framework cannot classify
            :test_spec: The type of test class
            :output: The test output
            :extra_info: Any extra information as a string
            :log_output: The program's log output
            """
            self.output = output
            self.test_spec = test_spec
            self.log_output = log_output
            self.extra_info = extra_info

        def __str__(self):
            output = (
                f"Indeterminate test result interpreted as failure, possible cause: {self.extra_info}\n"
                "------------\n"
                "|  Output  |\n"
                "------------\n"
                f"{self.get_output_str()}\n"
                "-------------------------------\n"
                "|  Program (i.e. Editor) log  |\n"
                "-------------------------------\n"
                f"{self.get_log_attribute_str()}\n"
            )
            return output


class AbstractTestSuite(object):
    """
    Main object used to run the tests.
    The new test suite class you create for your tests should inherit from this base AbstractTestSuite class.
    """
    # When this object is inherited, add any custom attributes as needed.
    # Test class to use for single test collection
    single_test_class = SingleTest
    # Test class to use for shared test collection
    shared_test_class = SharedTest

    class TestData:
        __test__ = False  # Avoid pytest collection & warnings since "test" is in the class name.

        def __init__(self):
            self.results = {}  # Dict of str(test_spec.__name__) -> Result
            self.asset_processor = None

    class Runner:
        def __init__(self, name, func, tests):
            self.name = name
            self.func = func
            self.tests = tests
            self.run_pytestfunc = None
            self.result_pytestfuncs = []

    class AbstractTestClass(pytest.Class):
        """
        Custom pytest collector which programmatically adds test functions based on data in the AbstractTestSuite class
        """

        def collect(self):
            """
            This collector does the following:
            1) Iterates through all the SingleTest subclasses defined inside the suite.
               Adds a test function to the suite to run each separately, and report results
            2) Iterates through all the SharedTest subclasses defined inside the suite,
               grouping tests based on the classes in by 3 categories: batched, parallel and batched+parallel.
               Each category gets a single test runner function registered to run all the tests of the category
               A result function will be added for every individual test, which will pass/fail based on the results
               from the previously executed runner function
            """
            cls = self.obj

            # Decorator function to add extra lookup information for the test functions
            def set_marks(marks):
                def spec_impl(func):
                    @functools.wraps(func)
                    def inner(*args, **argv):
                        return func(*args, **argv)

                    inner.marks = marks
                    return inner

                return spec_impl

            # Retrieve the test classes.
            single_tests = self.obj.get_single_tests()
            shared_tests = self.obj.get_shared_tests()
            batched_tests = cls.filter_shared_tests(shared_tests, is_parallelizable=False, is_batchable=True)
            parallel_tests = cls.filter_shared_tests(shared_tests, is_parallelizable=True, is_batchable=False)
            parallel_batched_tests = cls.filter_shared_tests(shared_tests, is_parallelizable=True, is_batchable=True)

            # User can provide a CLI option to not parallelize/batch the tests.
            no_parallelize = self.config.getoption("--no-test-batch", default=False)
            no_batch = self.config.getoption("--no-test-parallel", default=False)
            if no_parallelize:
                single_tests += parallel_tests
                parallel_tests = []
                batched_tests += parallel_batched_tests
                parallel_batched_tests = []
            if no_batch:
                single_tests += batched_tests
                batched_tests = []
                parallel_tests += parallel_batched_tests
                parallel_batched_tests = []

            # Add the single tests, these will run separately
            for test_spec in single_tests:
                name = test_spec.__name__

                def make_single_run(inner_test_spec):
                    @set_marks({"run_type": "run_single"})
                    def single_run(self, request, workspace, editor, collected_test_data, launcher_platform):
                        # only single tests are allowed to have setup/teardown, however we can have shared tests that
                        # were explicitly set as single, for example via cmdline argument override
                        is_single_test = issubclass(inner_test_spec, SingleTest)
                        if is_single_test:
                            # Setup step for wrap_run
                            wrap = inner_test_spec.wrap_run(
                                self, request, workspace, editor, collected_test_data, launcher_platform)
                            assert isinstance(wrap, types.GeneratorType), (
                                "wrap_run must return a generator, did you forget 'yield'?")
                            next(wrap, None)
                            # Setup step
                            inner_test_spec.setup(
                                self, request, workspace, editor, collected_test_data, launcher_platform)
                        # Run
                        self._run_single_test(request, workspace, editor, collected_test_data, inner_test_spec)
                        if is_single_test:
                            # Teardown
                            inner_test_spec.teardown(
                                self, request, workspace, editor, collected_test_data, launcher_platform)
                            # Teardown step for wrap_run
                            next(wrap, None)

                    return single_run

                single_run_test = make_single_run(test_spec)
                if hasattr(test_spec, "pytestmark"):
                    single_run_test.pytestmark = test_spec.pytestmark
                setattr(self.obj, name, single_run_test)

            # Add the shared tests, with a runner class for storing information from each shared run
            runners = []

            def create_runner(runner_name, function, tests):
                target_runner = AbstractTestSuite.Runner(runner_name, function, tests)

                def make_shared_run():
                    @set_marks({"runner": target_runner, "run_type": "run_shared"})
                    def shared_run(self, request, workspace, editor, collected_test_data, launcher_platform):
                        getattr(self, function.__name__)(
                            request, workspace, editor, collected_test_data, target_runner.tests)

                    return shared_run

                setattr(self.obj, runner_name, make_shared_run())

                # Add the shared tests results, which succeed/fail based what happened on the Runner.
                for shared_test_spec in tests:
                    def make_results_run(inner_test_spec):
                        @set_marks({"runner": target_runner, "test_spec": inner_test_spec, "run_type": "result"})
                        def result(self, request, workspace, editor, collected_test_data, launcher_platform):
                            result_key = inner_test_spec.__name__
                            # The runner must have filled the collected_test_data.results dict fixture for this test.
                            # Hitting this assert could mean if there was an error executing the runner
                            if result_key not in collected_test_data.results:
                                raise TestResultException(f"No results found for {result_key}. "
                                                          f"Test may not have ran due to the Editor "
                                                          f"shutting down. Check for issues in previous "
                                                          f"tests.")
                            cls._report_result(result_key, collected_test_data.results[result_key])

                        return result

                    result_func = make_results_run(shared_test_spec)
                    if hasattr(shared_test_spec, "pytestmark"):
                        result_func.pytestmark = shared_test_spec.pytestmark
                    setattr(self.obj, shared_test_spec.__name__, result_func)
                runners.append(target_runner)

            create_runner("run_batched_tests", cls._run_batched_tests, batched_tests)
            create_runner("run_parallel_tests", cls._run_parallel_tests, parallel_tests)
            create_runner("run_parallel_batched_tests", cls._run_parallel_batched_tests, parallel_batched_tests)

            # Now that we have added the functions to the class, have pytest retrieve all the tests the class contains
            pytest_class_instance = super().collect()[0]

            # Override the istestfunction for the object, with this we make sure that the
            # runners are always collected, even if they don't follow the "test_" naming
            original_istestfunction = pytest_class_instance.istestfunction

            def istestfunction(self, obj, name):
                ret = original_istestfunction(obj, name)
                if not ret:
                    ret = hasattr(obj, "marks")
                return ret

            pytest_class_instance.istestfunction = types.MethodType(istestfunction, pytest_class_instance)
            collection = pytest_class_instance.collect()

            def get_func_run_type(function):
                return getattr(function, "marks", {}).setdefault("run_type", None)

            collected_run_pytestfuncs = [item for item in collection if get_func_run_type(item.obj) == "run_shared"]
            collected_result_pytestfuncs = [item for item in collection if get_func_run_type(item.obj) == "result"]
            # We'll remove and store the runner functions for later, this way they won't be deselected by any
            # filtering mechanism. This collection process helps us determine which subset of tests to run.
            collection = [item for item in collection if item not in collected_run_pytestfuncs]

            # Match each generated pytestfunctions with every runner and store them
            for run_pytestfunc in collected_run_pytestfuncs:
                runner = run_pytestfunc.function.marks["runner"]
                runner.run_pytestfunc = run_pytestfunc

            for result_pytestfunc in collected_result_pytestfuncs:
                runner = result_pytestfunc.function.marks["runner"]
                runner.result_pytestfuncs.append(result_pytestfunc)

            self.obj._runners = runners
            return collection

    @staticmethod
    def get_number_parallel_instances():
        """
        Number of program instances to run in parallel, this method can be overridden by the user.
        Note: Some CLI options (i.e. '--editors-parallel') takes precedence over class settings.
        See ly_test_tools._internal.pytest_plugin.editor_test.py for full list of CLI options.
        :return: count of parallel program instances to run
        """
        count = 1
        found_processors = os.cpu_count()
        if found_processors:
            # only schedule on half the cores since the application will also run multithreaded
            # also compensates for hyperthreaded/clustered/virtual cores inflating this count
            count = math.floor(found_processors / 2)
            if count < 1:
                count = 1

        return count

    @classmethod
    def pytest_custom_modify_items(
            cls, session: _pytest.main.Session, items: list[AbstractTestBase], config: _pytest.config.Config) -> None:
        """
        Adds the runners' functions and filters the tests that will run. The runners will be added if they have any
        selected tests
        :param session: The Pytest Session
        :param items: The test case functions
        :param config: The Pytest Config object
        :return: None
        """
        new_items = []
        for runner in cls._runners:
            runner.tests[:] = cls.filter_session_shared_tests(items, runner.tests)
            if len(runner.tests) > 0:
                new_items.append(runner.run_pytestfunc)
                # Re-order dependent tests so they are run just after the runner
                for result_pytestfunc in runner.result_pytestfuncs:
                    found_test = next((item for item in items if item == result_pytestfunc), None)
                    if found_test:
                        items.remove(found_test)
                        new_items.append(found_test)

        items[:] = items + new_items

    @pytest.fixture(scope="class")
    def collected_test_data(self, request: _pytest.fixtures.FixtureRequest) -> AbstractTestSuite.TestData:
        """
        Yields a per-testsuite structure to store the data of each test result and an AssetProcessor object that will be
        re-used on the whole suite
        :request: The Pytest request object
        :yield: The TestData object
        """
        yield from self._collected_test_data(request)

    def _collected_test_data(self, request: _pytest.fixtures.FixtureRequest) -> AbstractTestSuite.TestData:
        """
        A wrapped implementation to simplify unit testing pytest fixtures. Users should not call this directly.
        :request: The Pytest request object (unused, but always passed by pytest)
        :yield: The TestData object
        """
        test_data = AbstractTestSuite.TestData()
        yield test_data  # yield to pytest while test-class executes
        # resumed by pytest after each test-class finishes
        if test_data.asset_processor:  # was assigned an AP to manage
            test_data.asset_processor.teardown()
            test_data.asset_processor = None
            editor_utils.kill_all_ly_processes(include_asset_processor=True)
        else:  # do not interfere as a preexisting AssetProcessor may be owned by something else
            editor_utils.kill_all_ly_processes(include_asset_processor=False)

    @classmethod
    def get_single_tests(cls) -> list[single_test_class]:
        """
        Grabs all of the single_test_class subclassed tests from the AbstractTestSuite class.
        Usage example:
           class MyTestSuite(AbstractTestSuite):
               class MyFirstTest(SingleTest):
                   from . import script_to_be_run_as_single_test as test_module
        :return: The list of single tests
        """
        single_tests = [
            c[1] for c in cls.__dict__.items() if inspect.isclass(c[1]) and issubclass(c[1], cls.single_test_class)]
        return single_tests

    @classmethod
    def get_shared_tests(cls) -> list[shared_test_class]:
        """
        Grabs all of the shared_test_class from the AbstractTestSuite
        Usage example:
           class MyTestSuite(AbstractTestSuite):
               class MyFirstTest(SharedTest):
                   from . import test_script_to_be_run_1 as test_module
               class MyFirstTest(SharedTest):
                   from . import test_script_to_be_run_2 as test_module
        :return: The list of shared tests
        """
        shared_tests = [
            c[1] for c in cls.__dict__.items() if inspect.isclass(c[1]) and issubclass(c[1], cls.shared_test_class)]
        return shared_tests

    @classmethod
    def get_session_shared_tests(cls, session: _pytest.main.Session) -> list[shared_test_class]:
        """
        Filters and returns all of the shared tests in a given session.
        :session: The test session
        :return: The list of tests
        """
        shared_tests = cls.get_shared_tests()
        return cls.filter_session_shared_tests(session, shared_tests)

    @staticmethod
    def filter_session_shared_tests(session_items: list[_pytest.python.Function(shared_test_class)],
                                    shared_tests: list[shared_test_class]) -> list[shared_test_class]:
        """
        Retrieve the test sub-set that was collected this can be less than the original set if were overridden via -k
        argument or similar
        :session_items: The tests in a session to run
        :shared_tests: All of the shared tests
        :return: The list of filtered tests
        """

        def will_run(item):
            try:
                skipping_pytest_runtest_setup(item)
                return True
            except (Warning, Exception, _pytest.outcomes.OutcomeException) as ex:
                # intentionally broad to avoid events other than system interrupts
                warnings.warn(f"Test deselected from execution queue due to {ex}")
                return False

        session_items_by_name = {item.originalname: item for item in session_items}
        selected_shared_tests = [test for test in shared_tests if test.__name__ in session_items_by_name.keys() and
                                 will_run(session_items_by_name[test.__name__])]
        return selected_shared_tests

    @staticmethod
    def filter_shared_tests(shared_tests: list[shared_test_class],
                            is_batchable: bool = False,
                            is_parallelizable: bool = False) -> list[shared_test_class]:
        """
        Filters and returns all tests based off of if they are batched and/or parallel
        :shared_tests: All shared tests
        :is_batchable: Filter to batched tests
        :is_parallelizable: Filter to parallel tests
        :return: The list of filtered tests
        """
        return [test for test in shared_tests if (
                getattr(test, "is_batchable", None) is is_batchable
                and
                getattr(test, "is_parallelizable", None) is is_parallelizable)]

    @staticmethod
    def _get_results_using_output(test_spec_list: list[AbstractTestBase],
                                  output: str,
                                  log_output: str) -> dict[any, [Result.Unknown, Result.Pass, Result.Fail]]:
        """
        Utility function for parsing the output information from the program being tested (i.e. editor).
        It de-serializes the JSON content printed in the output for every test and returns that information.
        :test_spec_list: The list of test classes
        :output: The test output
        :log_output: The program's log output
        :return: A dict of the tests and their respective Result objects
        """
        results = {}
        pattern = re.compile(r"JSON_START\((.+?)\)JSON_END")
        out_matches = pattern.finditer(output)
        found_jsons = {}
        for m in out_matches:
            try:
                elem = json.loads(m.groups()[0])
                found_jsons[elem["name"]] = elem
            except Exception:  # Intentionally broad to avoid failing if the output data is corrupt
                logger.warning("Error reading result JSON", exc_info=True)
                continue

        # Try to find the element in the log output, this is used for cutting the log contents later
        log_matches = pattern.finditer(log_output)
        for m in log_matches:
            try:
                elem = json.loads(m.groups()[0])
                if elem["name"] in found_jsons:
                    found_jsons[elem["name"]]["log_match"] = m
            except Exception:  # Intentionally broad, to avoid failing if the log data is corrupt
                logger.warning("Error reading result JSON", exc_info=True)
                continue

        log_start = 0
        for test_spec in test_spec_list:
            name = editor_utils.get_module_filename(test_spec.test_module)
            if name not in found_jsons.keys():
                results[test_spec.__name__] = Result.Unknown(
                    test_spec, output,
                    f"Found no test run information on stdout for {name} in the test output",
                    log_output)
            else:
                result = None
                json_result = found_jsons[name]
                json_output = json_result["output"]

                # Cut the log output so it only has the log contents for this run
                if "log_match" in json_result:
                    m = json_result["log_match"]
                    end = m.end() if test_spec != test_spec_list[-1] else -1
                else:
                    end = -1
                cur_log = log_output[log_start: end]
                log_start = end

                if json_result["success"]:
                    result = Result.Pass(test_spec, json_output, cur_log)
                else:
                    result = Result.Fail(test_spec, json_output, cur_log)
                results[test_spec.__name__] = result

        return results

    @staticmethod
    def _report_result(name: str, result: Result.ResultType) -> None:
        """
        Raises a pytest failure if the test result is not a PASS, specifying the information
        :name: Name of the test
        :result: The Result object which denotes if the test passed or not
        :return: None
        """
        if isinstance(result, Result.Pass):
            output_str = f"Test {name}:\n{str(result)}"
            print(output_str)
        else:
            error_str = f"Test {name}:\n{str(result)}"
            pytest.fail(error_str)

    def _prepare_asset_processor(self, workspace: AbstractWorkspaceManager, collected_test_data: TestData) -> None:
        """
        Prepares the asset processor for the test depending on whether or not the process is open and if the current
        test owns it.
        :workspace: The workspace object in case an AssetProcessor object needs to be created
        :collected_test_data: The test data from calling collected_test_data()
        :return: None
        """
        try:
            # Start-up an asset processor if we are not already managing one
            if collected_test_data.asset_processor is None:
                if not process_utils.process_exists("AssetProcessor", ignore_extensions=True):
                    editor_utils.kill_all_ly_processes(include_asset_processor=True)
                    collected_test_data.asset_processor = AssetProcessor(workspace)
                    collected_test_data.asset_processor.start()
                else:  # If another AP process already exists, do not kill it as we do not manage it
                    editor_utils.kill_all_ly_processes(include_asset_processor=False)
            else:  # Make sure existing asset processor wasn't closed by accident
                collected_test_data.asset_processor.start()
        except Exception as ex:
            collected_test_data.asset_processor = None
            raise ex

    def _setup_test_run(self, workspace: AbstractWorkspaceManager, collected_test_data: AbstractTestSuite.TestData) -> None:
        """
        Sets up a test run by preparing the Asset Processor and killing all other O3DE processes relevant to tests.
        :workspace: The test Workspace object
        :collected_test_data: The TestData from calling collected_test_data()
        :return: None
        """
        self._prepare_asset_processor(workspace, collected_test_data)
        editor_utils.kill_all_ly_processes(include_asset_processor=False)
