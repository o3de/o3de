"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
from __future__ import annotations

import pytest
import _pytest.python
import _pytest.outcomes
from _pytest.skipping import pytest_runtest_setup as skipping_pytest_runtest_setup

import abc
import functools
import inspect
import json
import logging
import re
import types
import warnings

import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.o3de.editor_test_utils as editor_utils
import ly_test_tools._internal.pytest_plugin.test_tools_fixtures

from ly_test_tools.o3de.asset_processor import AssetProcessor

# This file contains ready-to-use test functions which are not actual tests, avoid pytest collection
__test__ = False

logger = logging.getLogger(__name__)


class TestBase(abc.ABC):
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


class SingleTest(TestBase):
    """
    Test that will be run alone in one instance, with no parallel or serially batched instances.
    This should only be used if SharedTest, ParallelTest, or BatchedTest cannot be utilized for the test steps.
    It may also be used for debugging test issues in isolation.
    """
    def __init__(self):
        # Extra cmdline arguments to supply to the instance for the test.
        self.extra_cmdline_args = []
        # Whether to use null renderer, this will override use_null_renderer for the TestSuite if not None.
        self.use_null_renderer = None


class SharedTest(TestBase):
    """
    Can be one of two different test types:
    1. Test that will be run in parallel with tests in multiple instances.
    2. Test that will be run as serially batched with other tests in a single instance.

    Minimizes total test run duration by reducing repeated overhead from starting the instance.
    Does not support per test setup/teardown to avoid creating race conditions.
    """
    # Specifies if the test can be batched in the same instance.
    is_batchable = True
    # Specifies if the test can be run in multiple instances in parallel.
    is_parallelizable = True


class ParallelTest(SharedTest):
    """
    Test that will be run in parallel with tests in multiple instances.
    """
    is_batchable = False
    is_parallelizable = True


class BatchedTest(SharedTest):
    """
    Test that will be run as serially batched with other tests in a single instance.
    """
    is_batchable = True
    is_parallelizable = False


@pytest.mark.usefixtures("launcher")
@pytest.mark.parametrize("crash_log_watchdog", [("raise_on_crash", False)])
class TestSuite(object):
    """
    This is the main object used to hold SingleTest, SharedTest, ParallelTest, and BatchedTest tests.
    """
    # Extra cmdline arguments to supply for every instance for this test suite.
    global_extra_cmdline_args = ["-BatchMode", "-autotest_mode"]
    # Tests usually run with no renderer, however some tests require a renderer.
    use_null_renderer = True
    # Maximum time for a single instance to stay open on a shared test.
    shared_test_timeout = 300

    @staticmethod
    def get_number_parallel_instances():
        """Function to calculate number of instances to run in parallel, this can be overridden by the user."""
        return 8

    def _get_number_parallel_instances(self,
                                       request: _pytest.fixtures.FixtureRequest,
                                       config_option: str) -> int:
        """
        Retrieves the number of parallel preference cmdline overrides
        :request: The Pytest Request
        :return: The number of parallel instances to use
        """
        parallel_instances_value = request.config.getoption(config_option, None)
        if parallel_instances_value:
            return int(parallel_instances_value)

        return self.get_number_parallel_instances()

    # Maximum time (seconds) for waiting for a crash file, in seconds.
    _TIMEOUT_CRASH_LOG = 20
    # Return code for test failure.
    _TEST_FAIL_RETCODE = 0xF

    class TestData:
        # Required to tell pytest to skip collecting this class since "Test" is in the name, avoiding pytest warnings.
        __test__ = False

        def __init__(self):
            self.results = {}  # Dict of str(test_spec.__name__) -> Result
            self.asset_processor = None

    @pytest.fixture(scope="class")
    def test_data(self, request: _pytest.fixtures.FixtureRequest) -> TestSuite.TestData:
        """
        Yields a per-testsuite structure to store the data of each test result and an AssetProcessor object that will be
        re-used on the whole suite
        :request: The Pytest request object
        :yield: The TestData object
        """
        yield from self._test_data(request)

    def _test_data(self, request: _pytest.fixtures.FixtureRequest) -> TestSuite.TestData:
        """
        A wrapper function for unit testing of this file to call directly. Do not use in production.
        """
        test_data = TestSuite.TestData()
        yield test_data
        if test_data.asset_processor:
            test_data.asset_processor.stop(1)
            test_data.asset_processor.teardown()
            test_data.asset_processor = None
            editor_utils.kill_all_ly_processes(include_asset_processor=True)
        else:
            editor_utils.kill_all_ly_processes(include_asset_processor=False)

    class Runner:
        def __init__(self, name, func, tests):
            self.name = name
            self.func = func
            self.tests = tests
            self.run_pytestfunc = None
            self.result_pytestfuncs = []

    class BaseTestClass(pytest.Class):
        """
        Custom pytest collector which programmatically adds test functions based on data in the TestSuite class.
        """

        def __init__(self):
            super(TestSuite.BaseTestClass, self).__init__(self)
            self.single_tests = None
            self.shared_tests = None
            self.batched_tests = None
            self.parallel_tests = None
            self.parallel_batched_tests = None

        def collect(self):
            """
            This collector does the following:
            1) Iterates through all the SingleTest subclasses defined inside the suite.
            2) Adds a test function to the suite to run each separately, and report results.
            3) Iterates through all the SharedTest subclasses defined inside the suite,
               grouping tests based on the specs in by 3 categories: batched, parallel and batched+parallel.
            4) Each category gets a single test runner function registered to run all the tests of the category.
            5) A result function will be added for every individual test, which will pass/fail based on the results
               from the previously executed runner function.
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

            # Retrieve the test specs
            self.single_tests = self.obj.get_single_tests()
            self.shared_tests = self.obj.get_shared_tests()
            self.batched_tests = cls.filter_shared_tests(self.shared_tests, is_batchable=True)
            self.parallel_tests = cls.filter_shared_tests(self.shared_tests, is_parallelizable=True)
            self.parallel_batched_tests = cls.filter_shared_tests(
                self.shared_tests, is_parallelizable=True, is_batchable=True)

            # If user provides option to not parallelize/batch the tests, move them into single tests.
            no_parallelize = self.config.getoption("--no-instance-parallel", default=False)
            no_batch = self.config.getoption("--no-instance-batch", default=False)
            if no_parallelize:
                self.single_tests += self.parallel_tests
                parallel_tests = []
                self.batched_tests += self.parallel_batched_tests
                parallel_batched_tests = []
            if no_batch:
                self.single_tests += self.batched_tests
                batched_tests = []
                self.parallel_tests += self.parallel_batched_tests
                parallel_batched_tests = []

            # Add the single tests, these will run normally.
            for test_spec in self.single_tests:
                name = test_spec.__name__

                def make_test_func(name, test_spec, launcher):
                    @set_marks({"run_type": "run_single"})
                    def single_run(self, request, workspace, test_data, launcher_platform, launcher):
                        # only single tests are allowed to have setup/teardown, however we can have shared tests that
                        # were explicitly set as single, for example via cmdline argument override
                        is_single_test = issubclass(test_spec, SingleTest)
                        if is_single_test:
                            # Setup step for wrap_run
                            wrap = test_spec.wrap_run(self, request, workspace, test_data, launcher_platform, *args)
                            assert isinstance(wrap,
                                              types.GeneratorType), "wrap_run must return a generator, did you forget 'yield'?"
                            next(wrap, None)
                            # Setup step
                            test_spec.setup(self, request, workspace, test_data, launcher_platform, *args)
                        # Run
                        self._run_single_test(request, workspace, editor, test_data, test_spec)
                        if is_single_test:
                            # Teardown
                            test_spec.teardown(self, request, workspace, editor, test_data, launcher_platform)
                            # Teardown step for wrap_run
                            next(wrap, None)

                    return single_run

                f = make_test_func(name, test_spec, launcher)
                if hasattr(test_spec, "pytestmark"):
                    f.pytestmark = test_spec.pytestmark
                setattr(self.obj, name, f)

            # Add the shared tests, for these we will create a runner class for storing the run information
            # that will be later used for selecting what tests runners will be run
            runners = []

            def create_runner(name, function, tests):
                runner = TestSuite.Runner(name, function, tests)

                def make_func():
                    @set_marks({"runner": runner, "run_type": "run_shared"})
                    def shared_run(self, request, workspace, editor, test_data, launcher_platform):
                        getattr(self, function.__name__)(request, workspace, editor, test_data, runner.tests)

                    return shared_run

                setattr(self.obj, name, make_func())

                # Add the shared tests results, these just succeed/fail based what happened on the Runner.
                for test_spec in tests:
                    def make_func(test_spec):
                        @set_marks({"runner": runner, "test_spec": test_spec, "run_type": "result"})
                        def result(self, request, workspace, editor, test_data, launcher_platform):
                            # The runner must have filled the test_data.results dict fixture for this test.
                            # Hitting this assert could mean if there was an error executing the runner
                            if test_spec.__name__ not in test_data.results:
                                raise Result.TestResultException(f"No results found for {test_spec.__name__}. "
                                                                       f"Test may not have ran due to the Editor "
                                                                       f"shutting down. Check for issues in previous "
                                                                       f"tests.")
                            cls._report_result(test_spec.__name__, test_data.results[test_spec.__name__])

                        return result

                    result_func = make_func(test_spec)
                    if hasattr(test_spec, "pytestmark"):
                        result_func.pytestmark = test_spec.pytestmark
                    setattr(self.obj, test_spec.__name__, result_func)
                runners.append(runner)

            create_runner("run_batched_tests", cls._run_batched_tests, batched_tests)
            create_runner("run_parallel_tests", cls._run_parallel_tests, parallel_tests)
            create_runner("run_parallel_batched_tests", cls._run_parallel_batched_tests, parallel_batched_tests)

            # Now that we have added all the functions to the class, we will run
            # a class test collection to retrieve all the tests.
            instance = super().collect()[0]

            # Override the istestfunction for the object, with this we make sure that the
            # runners are always collected, even if they don't follow the "test_" naming
            original_istestfunction = instance.istestfunction

            def istestfunction(self, obj, name):
                ret = original_istestfunction(obj, name)
                if not ret:
                    ret = hasattr(obj, "marks")
                return ret

            instance.istestfunction = types.MethodType(istestfunction, instance)
            collection = instance.collect()

            def get_func_run_type(f):
                return getattr(f, "marks", {}).setdefault("run_type", None)

            collected_run_pytestfuncs = [
                item for item in collection if get_func_run_type(item.obj) == "run_shared"
            ]
            collected_result_pytestfuncs = [
                item for item in collection if get_func_run_type(item.obj) == "result"
            ]
            # We'll remove and store the runner functions for later, this way they won't
            # be deselected by any filtering mechanism. The result functions for these we are actually
            # interested on them to be filtered to tell what is the final subset of tests to run
            collection = [
                item for item in collection if item not in collected_run_pytestfuncs
            ]

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
    def pytest_custom_makeitem(collector, name, obj):
        return TestSuite.BaseTestClass(name, collector)

    @classmethod
    def pytest_custom_modify_items(cls, session: _pytest.main.Session, items: list[TestBase],
                                   config: _pytest.config.Config) -> None:
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

    @classmethod
    def get_single_tests(cls) -> list[SingleTest]:
        """
        Grabs all of the SingleTests subclassed tests from the TestSuite class
        Usage example:
           class MyTestSuite(TestSuite):
               class MyFirstTest(SingleTest):
                   from . import python_script_to_be_run_by_editor as test_module
        :return: The list of single tests
        """
        single_tests = [c[1] for c in cls.__dict__.items() if
                        inspect.isclass(c[1]) and issubclass(c[1], SingleTest)]
        return single_tests

    @classmethod
    def get_shared_tests(cls) -> list[SharedTest]:
        """
        Grabs all of the SharedTests from the TestSuite.
        Usage example:
           class MyTestSuite(TestSuite):
               class MyFirstTest(SharedTest):
                   from . import python_script_to_be_run_by_instance as test_module
        :return: The list of shared tests
        """
        shared_tests = [c[1] for c in cls.__dict__.items() if
                        inspect.isclass(c[1]) and issubclass(c[1], SharedTest)]
        return shared_tests

    @classmethod
    def get_session_shared_tests(cls, session: _pytest.main.Session) -> list[TestBase]:
        """
        Filters and returns all of the shared tests in a given session.
        :session: The test session.
        :return: The list of tests.
        """
        shared_tests = cls.get_shared_tests()
        return cls.filter_session_shared_tests(session, shared_tests)

    @staticmethod
    def filter_session_shared_tests(session_items: list[_pytest.python.Function(TestBase)],
                                    shared_tests: list[SharedTest]) -> list[TestBase]:
        """
        Retrieve the test sub-set that was collected.
        This can be less than the original set if were overriden via -k CLI argument or something similar.
        :session_items: The tests in a session to run.
        :shared_tests: All of the shared tests.
        :return: The list of filtered tests.
        """

        def will_run(item):
            try:
                skipping_pytest_runtest_setup(item)
                return True
            except (Warning, Exception, _pytest.outcomes.OutcomeException) as ex:
                # Intentionally broad to avoid events other than system interrupts
                warnings.warn(f"Test deselected from execution queue due to {ex}")
                return False

        session_items_by_name = {item.originalname: item for item in session_items}
        selected_shared_tests = [test for test in shared_tests if test.__name__ in session_items_by_name.keys() and
                                 will_run(session_items_by_name[test.__name__])]
        return selected_shared_tests

    @staticmethod
    def filter_shared_tests(shared_tests: list[SharedTest], is_batchable: bool = False,
                            is_parallelizable: bool = False) -> list[SharedTest]:
        """
        Filters and returns all tests based off of if they are batched and/or parallel.
        :shared_tests: All shared tests.
        :is_batchable: Filter to batched tests.
        :is_parallelizable: Filter to parallel tests.
        :return: The list of filtered tests.
        """
        return [
            t for t in shared_tests if (
                    getattr(t, "is_batchable", None) is is_batchable
                    and
                    getattr(t, "is_parallelizable", None) is is_parallelizable
            )
        ]

    def _prepare_asset_processor(self, workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                                 test_data: TestData) -> None:
        """
        Prepares the asset processor for the test depending on whether or not the process is open and if the current
        test owns it.
        :workspace: The workspace object in case an AssetProcessor object needs to be created
        :test_data: The test data from calling test_data()
        :return: None
        """
        try:
            # Start-up an asset processor if we are not running one
            # If another AP process exist, don't kill it, as we don't own it
            if test_data.asset_processor is None:
                if not process_utils.process_exists("AssetProcessor", ignore_extensions=True):
                    editor_utils.kill_all_ly_processes(include_asset_processor=True)
                    test_data.asset_processor = AssetProcessor(workspace)
                    test_data.asset_processor.start()
                else:
                    editor_utils.kill_all_ly_processes(include_asset_processor=False)
            else:
                # Make sure the asset processor from before wasn't closed by accident
                test_data.asset_processor.start()
        except Exception as ex:
            test_data.asset_processor = None
            raise ex

    def _setup_editor_test(self, editor: ly_test_tools.launchers.platforms.base.Launcher,
                           workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                           test_data: TestData) -> None:
        """
        Sets up an editor test by preparing the Asset Processor, killing all other O3DE processes, and configuring
        :editor: The launcher Editor object
        :workspace: The test Workspace object
        :test_data: The TestData from calling test_data()
        :return: None
        """
        self._prepare_asset_processor(workspace, test_data)
        editor_utils.kill_all_ly_processes(include_asset_processor=False)
        editor.configure_settings()

    @staticmethod
    def _get_results_using_output(
            test_spec_list: list[TestBase], output: str, editor_log_content: str) -> dict[str, Result]:
        """
        Utility function for parsing the output information from the editor. It deserializes the JSON content printed in
        the output for every test and returns that information.
        :test_spec_list: The list of EditorTests
        :output: The Editor from Editor.get_output()
        :editor_log_content: The contents of the editor log as a string
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
                continue

        # Try to find the element in the log, this is used for cutting the log contents later
        log_matches = pattern.finditer(editor_log_content)
        for m in log_matches:
            try:
                elem = json.loads(m.groups()[0])
                if elem["name"] in found_jsons:
                    found_jsons[elem["name"]]["log_match"] = m
            except Exception:  # Intentionally broad, to avoid failing if the log data is corrupt
                continue

        log_start = 0
        for test_spec in test_spec_list:
            name = editor_utils.get_module_filename(test_spec.test_module)
            if name not in found_jsons.keys():
                results[test_spec.__name__] = Result.Unknown(
                    test_spec, output,
                    f"Found no test run information on stdout for {name} in the editor log",
                    editor_log_content)
            else:
                result = None
                json_result = found_jsons[name]
                json_output = json_result["output"]

                # Cut the editor log so it only has the output for this run
                if "log_match" in json_result:
                    m = json_result["log_match"]
                    end = m.end() if test_spec != test_spec_list[-1] else -1
                else:
                    end = -1
                cur_log = editor_log_content[log_start: end]
                log_start = end

                if json_result["success"]:
                    result = Result.Pass(test_spec, json_output, cur_log)
                else:
                    result = Result.Fail(test_spec, json_output, cur_log)
                results[test_spec.__name__] = result

        return results

    @staticmethod
    def _report_result(name: str, result: Result) -> None:
        """
        Fails the test if the test result is not a PASS, specifying the information
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


class Result:
    class TestResultException(Exception):
        """ Indicates that an unknown result was found during the tests  """

    class Base:
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

        def get_instance_log_str(self):
            # type () -> str
            """
            Checks if a given instance's log attribute exists and returns it.
            :return: Either the editor_log string or a no output message
            """
            log = getattr(self, "editor_log", None)
            if log:
                return log
            else:
                return "-- No editor log found --"

    class Pass(Base):

        def __init__(self, test_spec: type(TestBase), output: str, editor_log: str):
            """
            Represents a test success
            :test_spec: The type of TestBase
            :output: The test output
            :editor_log: The editor log's output
            """
            self.test_spec = test_spec
            self.output = output
            self.editor_log = editor_log

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

        def __init__(self, test_spec: type(TestBase), output: str, editor_log: str):
            """
            Represents a normal test failure
            :test_spec: The type of TestBase
            :output: The test output
            :editor_log: The editor log's output
            """
            self.test_spec = test_spec
            self.output = output
            self.editor_log = editor_log

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
                f"{self.get_instance_log_str()}\n"
            )
            return output

    class Crash(Base):

        def __init__(self, test_spec: type(TestBase), output: str, ret_code: int, stacktrace: str,
                     editor_log: str):
            """
            Represents a test which failed with an unexpected crash
            :test_spec: The type of TestBase
            :output: The test output
            :ret_code: The test's return code
            :stacktrace: The test's stacktrace if available
            :editor_log: The editor log's output
            """
            self.output = output
            self.test_spec = test_spec
            self.ret_code = ret_code
            self.stacktrace = stacktrace
            self.editor_log = editor_log

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
                f"{self.get_instance_log_str()}\n"
            )
            return output

    class Timeout(Base):

        def __init__(self, test_spec: type(TestBase), output: str, time_secs: float, editor_log: str):
            """
            Represents a test which failed due to freezing, hanging, or executing slowly
            :test_spec: The type of TestBase
            :output: The test output
            :time_secs: The timeout duration in seconds
            :editor_log: The editor log's output
            :return: The Timeout object
            """
            self.output = output
            self.test_spec = test_spec
            self.time_secs = time_secs
            self.editor_log = editor_log

        def __str__(self):
            output = (
                f"Test ABORTED after not completing within {self.time_secs} seconds\n"
                f"------------\n"
                f"|  Output  |\n"
                f"------------\n"
                f"{self.get_output_str()}\n"
                f"--------------\n"
                f"| Editor log |\n"
                f"--------------\n"
                f"{self.get_instance_log_str()}\n"
            )
            return output

    class Unknown(Base):

        def __init__(self, test_spec: type(TestBase), output: str = None, extra_info: str = None,
                     editor_log: str = None):
            """
            Represents a failure that the test framework cannot classify
            :test_spec: The type of TestBase
            :output: The test output
            :extra_info: Any extra information as a string
            :editor_log: The editor log's output
            """
            self.output = output
            self.test_spec = test_spec
            self.editor_log = editor_log
            self.extra_info = extra_info

        def __str__(self):
            output = (
                f"Indeterminate test result interpreted as failure, possible cause: {self.extra_info}\n"
                f"------------\n"
                f"|  Output  |\n"
                f"------------\n"
                f"{self.get_output_str()}\n"
                f"--------------\n"
                f"| Editor log |\n"
                f"--------------\n"
                f"{self.get_instance_log_str()}\n"
            )
            return output
