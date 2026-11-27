"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Simplified O3DE test-writing utilities.

Supports different options for running tests using this framework:
SingleTest: A single test that runs 1 test in 1 executable/application.
SharedTest: Multiple tests that run multiple tests in multiple executables/applications.
BatchedTest: Multiple tests that run 1 executable/application with multiple tests in that 1 executable/application.
ParallelTest: Multiple tests that run multiple executables/applications with 1 test in each executable/application.

It is recommended that new modules are created with objects that inherit from objects here for new tests.
Example: creating a new EditorTestSuite(MultiTestSuite) class for testing the Editor application.
"""

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
import shutil
import tempfile
import threading
import types
import typing
import uuid
import warnings

import pytest
import _pytest.python
import _pytest.outcomes
from _pytest.skipping import pytest_runtest_setup as skip_pytest_runtest_setup

import ly_test_tools.launchers.platforms.base as Launchers
import ly_test_tools.o3de.editor_test_utils as editor_utils
from ly_test_tools._internal.managers.workspace import AbstractWorkspaceManager
from ly_test_tools._internal.exceptions import EditorToolsFrameworkException, TestResultException
from ly_test_tools.launchers import launcher_helper
from ly_test_tools.launchers.exceptions import WaitTimeoutError
from ly_test_tools.launchers.platforms.linux.launcher import LinuxEditor, LinuxAtomToolsLauncher
from ly_test_tools.launchers.platforms.win.launcher import WinEditor, WinAtomToolsLauncher

logger = logging.getLogger(__name__)


class AbstractTestBase(object):
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
    Test that will be run alone in one executable, with no parallel or serially batched executables.
    This should only be used if SharedTest, ParallelTest, or BatchedTest cannot be utilized for the test steps.
    It may also be used for debugging test issues in isolation.
    """
    def __init__(self):
        # Extra cmdline arguments to supply to the executable for the test.
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


class Result(object):
    """Holds test results for a given program/application."""

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

        def get_log_output_str(self):
            # type () -> str
            """
            Checks if the log_output attribute exists and returns it.
            :return: Either the log_output string set by the result or a no output message.
            """
            log = getattr(self, "log_output", None)
            if log:
                return log
            else:
                return "-- No log found --"

    class Pass(ResultType):

        def __init__(self, test_spec: type(AbstractTestBase), output: str, log_output: str):
            """
            Represents a test success.
            :param test_spec: The type of test class.
            :param output: The test output.
            :param log_output: The program's log output.
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
            Represents a normal test failure.
            :param test_spec: The type of test class
            :param output: The test output
            :param log_output: The program's log output
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
                "----------------------------------------------------\n"
                f"| Application log  |\n"
                "----------------------------------------------------\n"
                f"{self.get_log_output_str()}\n"
            )
            return output

    class Crash(ResultType):

        def __init__(self,
                     test_spec: type(AbstractTestBase),
                     output: str,
                     ret_code: int,
                     stacktrace: str or None,
                     log_output: str or None) -> None:
            """
            Represents a test which failed with an unexpected crash
            :param test_spec: The type of test class
            :param output: The test output
            :param ret_code: The test's return code
            :param stacktrace: The test's stacktrace if available
            :param log_output: The program's log output
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
                "----------------------------------------------------\n"
                f"| Application log  |\n"
                "----------------------------------------------------\n"
                f"{self.get_log_output_str()}\n"
            )
            return output

    class Timeout(ResultType):

        def __init__(self,
                     test_spec: type(AbstractTestBase),
                     output: str,
                     time_secs: float,
                     log_output: str):
            """
            Represents a test which failed due to freezing, hanging, or executing slowly
            :param test_spec: The type of test class
            :param output: The test output
            :param time_secs: The timeout duration in seconds
            :param log_output: The program's log output
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
                "----------------------------------------------------\n"
                f"| Application log  |\n"
                "----------------------------------------------------\n"
                f"{self.get_log_output_str()}\n"
            )
            return output

    class Unknown(ResultType):

        def __init__(self,
                     test_spec: type(AbstractTestBase),
                     output: str = None,
                     extra_info: str = None,
                     log_output: str = None):
            """
            Represents a failure that the test framework cannot classify
            :param test_spec: The type of test class
            :param output: The test output
            :param extra_info: Any extra information as a string
            :param log_output: The program's log output
            """
            self.output = output
            self.test_spec = test_spec
            self.extra_info = extra_info
            self.log_output = log_output

        def __str__(self):
            output = (
                f"Indeterminate test result interpreted as failure, possible cause: {self.extra_info}\n"
                "------------\n"
                "|  Output  |\n"
                "------------\n"
                f"{self.get_output_str()}\n"
                "----------------------------------------------------\n"
                f"| Application log  |\n"
                "----------------------------------------------------\n"
                f"{self.get_log_output_str()}\n"
            )
            return output


class _RunLogContext:
    """Own the application log directory for one executable launch.

    Numeric run IDs still name artifacts. Only a directory created by this
    context may be removed, and failed runs or unsuccessful artifact copies
    retain their source files for diagnosis.
    """

    def __init__(self, executable, workspace, run_id, log_name):
        self.workspace = workspace
        self.run_id = run_id
        self.log_name = log_name
        self.log_run_id = f"{run_id}_{uuid.uuid4().hex}"
        self.is_editor = type(executable) in (WinEditor, LinuxEditor)
        self.is_atom_tools = type(executable) in (WinAtomToolsLauncher, LinuxAtomToolsLauncher)
        self.owns_directory = self.is_editor or self.is_atom_tools
        self.artifacts_saved = True
        if self.is_atom_tools:
            self.directory = editor_utils.atom_tools_log_path(self.log_run_id, workspace)
            self._read_log = editor_utils.retrieve_non_editor_log_content
        else:
            self.directory = editor_utils.retrieve_log_path(self.log_run_id, workspace)
            self._read_log = editor_utils.retrieve_editor_log_content

        if self.owns_directory:
            # Do not accept an existing directory, even in the unlikely event
            # of an identifier collision.
            os.makedirs(self.directory)
            logger.info("Using test log directory: %s", self.directory)

    def command_line(self):
        if self.is_editor:
            return ["-logfile", f"@log@/{self.log_name}", "-project-log-path", self.directory]
        if self.is_atom_tools:
            return ["-logfile", os.path.join(self.directory, self.log_name),
                    "-project-log-path", self.directory]
        return []

    def read(self):
        return self._read_log(self.log_run_id, self.log_name, self.workspace)

    def save_file(self, path, artifact_name=None):
        try:
            if artifact_name is None:
                saved_path = self.workspace.artifact_manager.save_artifact(path)
            else:
                saved_path = self.workspace.artifact_manager.save_artifact(path, artifact_name)
        except OSError as exc:
            # A missing or inaccessible diagnostic file must not replace the
            # executable's actual failure, crash, or timeout result.
            self.artifacts_saved = False
            logger.warning("Could not save test artifact %s: %s", path, exc)
            return
        if saved_path is None:
            # NullArtifactManager deliberately does not copy files.
            self.artifacts_saved = False

    def save_application_log(self):
        self.save_file(os.path.join(self.directory, self.log_name), f"({self.run_id}){self.log_name}")

    def save_crash_artifacts(self):
        crash_name = os.path.basename(self.workspace.paths.crash_log())
        for name in dict.fromkeys(("error.dmp", crash_name)):
            path = os.path.join(self.directory, name)
            if os.path.exists(path):
                self.save_file(path)

    def collect_crash(self, timeout):
        crash_output = editor_utils.retrieve_crash_output(
            self.log_run_id, self.workspace, timeout, log_directory=self.directory)
        self.save_crash_artifacts()
        return crash_output

    def finish(self, results):
        if not self.owns_directory:
            return
        # Retain failed runs and any run for which artifact collection was
        # incomplete. Successful runs with saved diagnostics need no duplicate
        # project-side directory.
        if not self.artifacts_saved or not all(isinstance(result, Result.Pass) for result in results.values()):
            logger.info("Retaining test log directory: %s", self.directory)
            return
        try:
            shutil.rmtree(self.directory)
        except OSError as exc:
            logger.warning("Could not remove test log directory %s: %s", self.directory, exc)


class MultiTestSuite(object):
    """
    Main object used to run the tests.
    The new test suite class you create for your tests should inherit from this base MultiTestSuite class.
    """
    # When this object is inherited, add any custom attributes as needed.
    # Extra cmdline arguments to supply for every executable instance for this test suite
    global_extra_cmdline_args = ["-BatchMode", "-autotest_mode"]
    # Extra commandline arguments to supply to the asset-processor for this test suite
    global_assetprocessor_extra_cmdline_args = None
    # Tests usually run with no renderer, however some tests require a renderer and will disable this
    use_null_renderer = True
    # Maximum time in seconds for a single executable to stay open across the set of shared tests
    timeout_shared_test = 900
    # Name of the executable's log file.
    log_name = ""
    # Executable name to look for if the test is an Atom Tools test, leave blank if not an Atom Tools test.
    atom_tools_executable_name = ""
    # Maximum time (seconds) for waiting for a crash file to finish being dumped to disk
    _timeout_crash_log = 20
    # Return code for test failure
    _test_fail_retcode = 0xF
    # Test class to use for single test collection
    _single_test_class = SingleTest
    # Test class to use for shared test collection
    _shared_test_class = SharedTest

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

    class MultiTestCollector(pytest.Class):
        """
        Custom pytest collector which programmatically adds test functions based on data in the MultiTestSuite class
        """

        # Decorator function to add extra lookup information for the test functions
        @staticmethod
        def _set_marks(marks: dict) -> typing.Callable:
            """
            Sets pytest marks for a MultiTest test

            :param marks: The pytest marks to set as a dict
            :return: A function to set the marks
            """
            def spec_impl(func):
                @functools.wraps(func)
                def inner(*args, **argv):
                    return func(*args, **argv)

                inner.marks = marks
                return inner

            return spec_impl

        @staticmethod
        def _make_single_run(inner_test_spec: AbstractTestBase) -> typing.Callable:
            """
            Prepares a test run to be collected as a SingleTest, regardless of its actual test type. Single tests are
            ran independently of each other, as opposed to a SharedTest. This also includes setup() and teardown()
            functions if applicable.

            :param inner_test_spec: The test spec class (i.e. SingleTest, SharedTest)
            :return: The function to run as a single test
            """
            @MultiTestSuite.MultiTestCollector._set_marks({"run_type": "run_single"})
            def single_run(self,
                           request: pytest.FixtureRequest,
                           workspace: AbstractWorkspaceManager,
                           collected_test_data: MultiTestSuite.TestData,
                           launcher_platform: Launchers) -> typing.Callable:
                """
                The actual function to be called by pytest. This wraps around _run_single_test() for a customized
                handling of setup() and teardown().

                :param request: The pytest request
                :param workspace: The LyTestTools workspace object
                :param collected_test_data: The TestData object to store the test results
                :param launcher_platform: The LyTestTools launcher
                :return: The function to run the pytest.
                """
                # only single tests are allowed to have setup/teardown, however we can have shared tests that
                # were explicitly set as single, for example via cmdline argument override
                is_single_test = issubclass(inner_test_spec, SingleTest)
                if is_single_test:
                    # Setup step for wrap_run
                    wrap = inner_test_spec.wrap_run(
                        self, request, workspace, collected_test_data)
                    if not isinstance(wrap, types.GeneratorType):
                        raise EditorToolsFrameworkException("wrap_run must return a generator, did you forget 'yield'?")
                    next(wrap, None)
                    # Setup step
                    inner_test_spec.setup(
                        self, request, workspace)
                # Run
                self._run_single_test(request, workspace, collected_test_data, inner_test_spec)
                if is_single_test:
                    # Teardown
                    inner_test_spec.teardown(
                        self, request, workspace, collected_test_data)
                    # Teardown step for wrap_run
                    next(wrap, None)

            return single_run

        @staticmethod
        def _create_runner(runner_name: str, function: typing.Callable, tests: list, cls) -> MultiTestSuite.Runner:
            """
            Create a test runner for SharedTests. This involves both test running and reporting. Create functions
            that runs the tests and sets it to attributes based off of the test names.

            :param runner_name: Name of the test runner
            :param function: The function that will run the tests
            :param tests: A list of AbstractTestBase objects
            :param cls: The MultiTestCollector self.obj
            :return: The MultiTestSuite runner object
            """
            target_runner = MultiTestSuite.Runner(runner_name, function, tests)

            def make_shared_run():
                @MultiTestSuite.MultiTestCollector._set_marks({"runner": target_runner, "run_type": "run_shared"})
                def shared_run(self, request, workspace, collected_test_data, launcher_platform):
                    getattr(self, function.__name__)(
                        request, workspace, collected_test_data, target_runner.tests)

                return shared_run

            setattr(cls, runner_name, make_shared_run())

            # Add the shared tests results, which succeed/fail based what happened on the Runner.
            for shared_test_spec in tests:
                def make_results_run(inner_test_spec):
                    @MultiTestSuite.MultiTestCollector._set_marks(
                        {"runner": target_runner, "test_spec": inner_test_spec, "run_type": "result"})
                    def result(self, request, workspace, collected_test_data, launcher_platform):
                        result_key = inner_test_spec.__name__
                        # The runner must have filled the collected_test_data.results dict fixture for this test.
                        # Hitting this assert could mean if there was an error executing the runner
                        if result_key not in collected_test_data.results:
                            logger.debug(f"\nTest Result objects - collected_test_data.results:\n"
                                         f"{collected_test_data.results}\n")
                            raise TestResultException(f"No results found for {result_key}. "
                                                      f"Test may not have ran due to the executable "
                                                      f"shutting down. Check for issues in previous "
                                                      f"tests.")
                        cls._report_result(result_key, collected_test_data.results[result_key])

                    return result

                result_func = make_results_run(shared_test_spec)
                if hasattr(shared_test_spec, "pytestmark"):
                    result_func.pytestmark = shared_test_spec.pytestmark
                setattr(cls, shared_test_spec.__name__, result_func)
            return target_runner

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

            # Retrieve the test classes.
            single_tests = self.obj.get_single_tests()
            shared_tests = self.obj.get_shared_tests()
            batched_tests = cls.filter_shared_tests(shared_tests, is_parallelizable=False, is_batchable=True)
            parallel_tests = cls.filter_shared_tests(shared_tests, is_parallelizable=True, is_batchable=False)
            parallel_batched_tests = cls.filter_shared_tests(shared_tests, is_parallelizable=True, is_batchable=True)

            # User can provide a CLI option to not parallelize/batch the tests.
            no_batch = self.config.getoption("--no-test-batch", default=False)
            no_parallelize = self.config.getoption("--no-test-parallel", default=False)
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
                single_run_test = MultiTestSuite.MultiTestCollector._make_single_run(test_spec)
                if hasattr(test_spec, "pytestmark"):
                    single_run_test.pytestmark = test_spec.pytestmark
                setattr(self.obj, name, single_run_test)

            # Add the shared tests, with a runner class for storing information from each shared run
            runners = []

            runners.append(self._create_runner("run_batched_tests", cls._run_batched_tests, batched_tests, cls))
            runners.append(self._create_runner("run_parallel_tests", cls._run_parallel_tests, parallel_tests, cls))
            runners.append(self._create_runner("run_parallel_batched_tests", cls._run_parallel_batched_tests,
                                               parallel_batched_tests, cls))

            # Override istestfunction for our own object, to collect runners even if they don't follow "test_" naming
            # avoid declaring istestfunction on the class, so it only exists now after initial pytest collect() has run
            original_istestfunction = self.istestfunction

            def istestfunction(self, obj, name):
                ret = original_istestfunction(obj, name)
                if not ret:
                    ret = hasattr(obj, "marks")
                return ret

            self.istestfunction = types.MethodType(istestfunction, self)

            # Now that we have added the functions to the class, have pytest retrieve all the tests the class contains
            collection = super().collect()
            collected_run_pytestfuncs = [item for item in collection if
                                         # Gets function run type
                                         getattr(item.obj, "marks", {}).setdefault("run_type", None) == "run_shared"]
            collected_result_pytestfuncs = [item for item in collection if
                                            getattr(item.obj, "marks", {}).setdefault("run_type", None) == "result"]
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
    def get_number_parallel_executables():
        """
        Number of program executables to run in parallel, this method can be overridden by the user.
        Note: --parallel-executables CLI arg takes precedence over default class settings.
        See ly_test_tools._internal.pytest_plugin.multi_testing.py for full list of CLI options.
        :return: count of parallel program executables to run
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

    def _get_number_parallel_executables(self, request: _pytest.fixtures.FixtureRequest) -> int:
        """
        Retrieves the number of parallel executables preference based on cmdline overrides or class overrides.
        Defaults to self.get_number_parallel_executables() from inherited MultiTestSuite class.
        :request: The Pytest Request object
        :return: The number of parallel executables to use
        """
        parallel_executables_value = request.config.getoption("--parallel-executables", None)
        if parallel_executables_value:
            return int(parallel_executables_value)

        return self.get_number_parallel_executables()

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
    def collected_test_data(self, request: _pytest.fixtures.FixtureRequest) -> MultiTestSuite.TestData:
        """
        Yields a per-testsuite structure to store the data of each test result and an AssetProcessor object that will be
        re-used on the whole suite
        :request: The Pytest request object
        :yield: The TestData object
        """
        yield from self._collected_test_data(request)

    def _collected_test_data(self, request: _pytest.fixtures.FixtureRequest) -> MultiTestSuite.TestData:
        """
        A wrapped implementation to simplify unit testing pytest fixtures. Users should not call this directly.
        :request: The Pytest request object (unused, but always passed by pytest)
        :yield: The TestData object
        """
        test_data = MultiTestSuite.TestData()
        yield test_data  # yield to pytest while test-class executes
        # resumed by pytest after each test-class finishes
        if test_data.asset_processor:  # was assigned an AP to manage
            test_data.asset_processor.teardown()
            test_data.asset_processor = None
            editor_utils.kill_all_ly_processes(include_asset_processor=True)
        else:  # do not interfere as a preexisting AssetProcessor may be owned by something else
            editor_utils.kill_all_ly_processes(include_asset_processor=False)

    @classmethod
    def get_single_tests(cls) -> list[_single_test_class]:
        """
        Grabs all of the _single_test_class subclassed tests from the MultiTestSuite class.
        Usage example:
           class MyTestSuite(MultiTestSuite):
               class MyFirstTest(SingleTest):
                   from . import script_to_be_run_as_single_test as test_module
        :return: The list of single tests
        """
        single_tests = [
            c[1] for c in cls.__dict__.items() if inspect.isclass(c[1]) and issubclass(c[1], cls._single_test_class)]
        return single_tests

    @classmethod
    def get_shared_tests(cls) -> list[_shared_test_class]:
        """
        Grabs all of the _shared_test_class from the MultiTestSuite
        Usage example:
           class MyTestSuite(MultiTestSuite):
               class MyFirstTest(SharedTest):
                   from . import test_script_to_be_run_1 as test_module
               class MyFirstTest(SharedTest):
                   from . import test_script_to_be_run_2 as test_module
        :return: The list of shared tests
        """
        shared_tests = [
            c[1] for c in cls.__dict__.items() if inspect.isclass(c[1]) and issubclass(c[1], cls._shared_test_class)]
        return shared_tests

    @classmethod
    def get_session_shared_tests(cls, session: _pytest.main.Session) -> list[_shared_test_class]:
        """
        Filters and returns all of the shared tests in a given session.
        :param session: The test session
        :return: The list of tests
        """
        shared_tests = cls.get_shared_tests()
        return cls.filter_session_shared_tests(session, shared_tests)

    @staticmethod
    def filter_session_shared_tests(session_items: list[_pytest.python.Function(_shared_test_class)],
                                    shared_tests: list[_shared_test_class]) -> list[_shared_test_class]:
        """
        Retrieve the test sub-set that was collected this can be less than the original set if were overridden via -k
        argument or similar
        :param session_items: The tests in a session to run
        :param shared_tests: All of the shared tests
        :return: The list of filtered tests
        """

        def will_run(item):
            try:
                skip_pytest_runtest_setup(item)
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
    def filter_shared_tests(shared_tests: list[_shared_test_class],
                            is_batchable: bool = False,
                            is_parallelizable: bool = False) -> list[_shared_test_class]:
        """
        Filters and returns all tests based off of if they are batched and/or parallel
        :param shared_tests: All shared tests
        :param is_batchable: Filter to batched tests
        :param is_parallelizable: Filter to parallel tests
        :return: The list of filtered tests
        """
        return [test for test in shared_tests if (
                getattr(test, "is_batchable", None) is is_batchable
                and
                getattr(test, "is_parallelizable", None) is is_parallelizable)]

    def _setup_test(self, workspace, collected_test_data):
        """
        Sets up for a multi test run. This is responsible for finding executables and
        handles asset processor.
        :param workspace: The LyTestTools Workspace object
        :param collected_test_data: The TestData from calling collected_test_data()
        :return: None
        """
        # Set the self.executable program for Launcher and re-bind our param workspace to it.
        if self.atom_tools_executable_name:  # Atom Tools test.
            self.executable = launcher_helper.create_atom_tools_launcher(workspace, self.atom_tools_executable_name)
        else:  # Editor test.
            self.executable = launcher_helper.create_editor(workspace)
        self.executable.workspace = workspace

        # Setup AP, kill processes, and configure the executable.
        editor_utils.prepare_asset_processor(workspace, collected_test_data, extra_params=self.global_assetprocessor_extra_cmdline_args)
        editor_utils.kill_all_ly_processes(include_asset_processor=False)
        self.executable.configure_settings()

    def _test_reporting(self, collected_test_data, results, workspace, test_spec):
        """
        Handles reportings and unknown test results after test runs.
        :param collected_test_data: The TestData from calling collected_test_data()
        :param results: A list of dicts of Result objects
        :param workspace: The LyTestTools Workspace object
        :param test_spec: The test class that should be a subclass of SingleTest
        """
        save_asset_logs = False

        for result in results:
            if result is None:
                logger.error("Unexpectedly found no test run in the executable log")
                logger.debug(f"Debug Results:\n{results}")
                result = {"Unknown":
                            Result.Unknown(
                                test_spec=test_spec,
                                extra_info="Unexpectedly found no test run information on stdout in the executable log")}
            collected_test_data.results.update(result)
            if not isinstance(result, Result.Pass):
                if isinstance(test_spec, BatchedTest):
                    editor_utils.save_failed_asset_joblogs(workspace)
                    return  # exit early on first batch failure
                save_asset_logs = True
        # If at least one test did not pass, save assets with errors and warnings
        if save_asset_logs:
            editor_utils.save_failed_asset_joblogs(workspace)

    def _setup_cmdline_args(self, cmdline_args, executable, test_spec_list, workspace):
        """
        Prepares a list of command line arguments to pass when calling the executable during a test.
        :param cmdline_args: Any additional command line args
        :param executable: The program executable under test
        :param test_spec_list: A list of test classes to run in the same executable
        :return: A list of additional command line arguments
        """
        if cmdline_args is None:
            cmdline_args = []
        test_cmdline_args = self.global_extra_cmdline_args + cmdline_args
        if type(executable) in [WinEditor, LinuxEditor]:  # Handle Editor CLI args since we need workspace context to populate them.
            test_cmdline_args += [
                "--regset=/Amazon/Preferences/EnablePrefabSystem=true",
                f"--regset-file={os.path.join(workspace.paths.engine_root(), 'Registry', 'prefab.test.setreg')}",
                "--log_IncludeTime=1"
            ]
        if self.use_null_renderer:
            test_cmdline_args += ["-rhi=null"]
        if any([t.attach_debugger for t in test_spec_list]):
            test_cmdline_args += ["--attach-debugger"]
        if any([t.wait_for_debugger for t in test_spec_list]):
            test_cmdline_args += ["--wait-for-debugger"]
        return test_cmdline_args

    ###################
    # SingleTest Code #
    ###################

    def _run_single_test(self,
                         request: _pytest.fixtures.FixtureRequest,
                         workspace: AbstractWorkspaceManager,
                         collected_test_data: MultiTestSuite.TestData,
                         test_spec: SingleTest) -> None:
        """
        Runs a single test (one executable, one test) with the given specs.
        This function also sets up self.executable for a given program under test.
        :param request: The Pytest Request
        :param workspace: The LyTestTools Workspace object
        :param collected_test_data: The TestData from calling collected_test_data()
        :param test_spec: The test class that should be a subclass of SingleTest
        :return: None
        """
        self._setup_test(workspace, collected_test_data)
        
        # Handle command line args and launch test.
        extra_cmdline_args = []
        if hasattr(test_spec, "extra_cmdline_args"):
            extra_cmdline_args = test_spec.extra_cmdline_args
        result = self._exec_single_test(
            request, workspace, self.executable, 1, self.log_name, test_spec, extra_cmdline_args)
        if result is None:
            logger.error(f"Unexpectedly found no test run in the {self.log_name} during {test_spec}")
            result = {"Unknown":
                      Result.Unknown(
                          test_spec=test_spec,
                          extra_info=f"Unexpectedly found no test run information on stdout in the {self.log_name}")}
        collected_test_data.results.update(result)
        test_name, test_result = next(iter(result.items()))
        self._report_result(test_name, test_result)

        # If test did not pass, save assets with errors and warnings
        if not isinstance(test_result, Result.Pass):
            editor_utils.save_failed_asset_joblogs(workspace)

    def _exec_single_test(self,
                          request: _pytest.fixtures.FixtureRequest,
                          workspace: AbstractWorkspaceManager,
                          executable: ly_test_tools.launchers.platforms.base.Launcher,
                          run_id: int,
                          log_name: str,
                          test_spec: AbstractTestBase,
                          cmdline_args: list[str] = None) -> dict[str, Result.ResultType]:
        """
        Starts the executable with the given test and returns a result dict with a single element specifying the result
        :param request: The pytest request
        :param workspace: The LyTestTools Workspace object
        :param executable: The program executable under test
        :param run_id: The unique run id
        :param log_name: The name of the executable log to retrieve
        :param test_spec: The type of test class
        :param cmdline_args: Any additional command line args
        :return: a dictionary of Result objects (should be only one) with a given Result.ResultType (i.e. Result.Pass).
        """
        test_cmdline_args = self._setup_cmdline_args(cmdline_args, executable, [test_spec], workspace)
        logs = _RunLogContext(executable, workspace, run_id, log_name)
        test_filename = editor_utils.get_testcase_module_filepath(test_spec.test_module)
        test_case_name = editor_utils.compile_test_case_name(request, test_spec)
        cmdline = []
        if logs.is_editor:
            cmdline = ["-runpythontest", test_filename, f"-pythontestcase={test_case_name}"]
        elif logs.is_atom_tools:
            cmdline = ["-runpythontest", test_filename]
        if cmdline:
            executable.args.extend(cmdline + logs.command_line() + test_cmdline_args)
        executable.start(backupFiles=False, launch_ap=False, configure_settings=False)

        timed_out = False
        try:
            executable.wait(test_spec.timeout)
        except WaitTimeoutError:
            timed_out = True
            executable.stop()

        # Capture output after the process has exited or been stopped.
        output = executable.get_output()
        executable_log_content = logs.read()
        workspace.artifact_manager.set_dest_path(request.node.originalname)
        logs.save_application_log()

        for name in ("Server.log", "Game.log"):
            path = os.path.join(workspace.paths.project_log(), name)
            if os.path.exists(path) and os.path.getsize(path):
                logs.save_file(path)

        if timed_out:
            logs.save_crash_artifacts()
            test_result = Result.Timeout(test_spec, output, test_spec.timeout, executable_log_content)
        else:
            return_code = executable.get_returncode()
            if return_code == 0:
                test_result = Result.Pass(test_spec, output, executable_log_content)
            elif return_code == self._test_fail_retcode:
                test_result = Result.Fail(test_spec, output, executable_log_content)
            else:
                crash_output = logs.collect_crash(self._timeout_crash_log)
                test_result = Result.Crash(test_spec, output, return_code, crash_output, executable_log_content)

        results = {test_spec.__name__: test_result}
        logs.finish(results)
        return results

    ####################
    # BatchedTest Code #
    ####################

    def _run_batched_tests(self,
                           request: _pytest.fixtures.FixtureRequest,
                           workspace: AbstractWorkspaceManager,
                           collected_test_data: MultiTestSuite.TestData,
                           test_spec_list: list[BatchedTest],
                           extra_cmdline_args: list[str] = None) -> None:
        """
        Runs a batch of tests in one single executable program with the given spec list (one executable, multiple tests)
        This function also sets up self.executable for a given program under test.
        :param request: The Pytest Request
        :param workspace: The LyTestTools Workspace object
        :param collected_test_data: The TestData from calling collected_test_data()
        :param test_spec_list: A list of SharedTest objects.
        :param extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if not test_spec_list:
            return        
        self._setup_test(workspace, collected_test_data)

        if extra_cmdline_args is None:
            extra_cmdline_args = []

        results = self._exec_multitest(
            request, workspace, self.executable, 1, self.log_name, test_spec_list, extra_cmdline_args)
        self._test_reporting(collected_test_data, [results], workspace, BatchedTest)

    #####################
    # ParallelTest Code #
    #####################

    def _run_parallel_tests(self,
                            request: _pytest.fixtures.FixtureRequest,
                            workspace: AbstractWorkspaceManager,
                            collected_test_data: MultiTestSuite.TestData,
                            test_spec_list: list[ParallelTest],
                            extra_cmdline_args: list[str] = None) -> None:
        """
        Runs multiple executable with one test on each executable (multiple executables, one test each).
        This function also sets up self.executable for a given program under test.
        :param request: The Pytest Request
        :param workspace: The LyTestTools Workspace object
        :param collected_test_data: The TestData from calling collected_test_data()
        :param test_spec_list: A list of SharedTest tests to run
        :param extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if not test_spec_list:
            return   
        self._setup_test(workspace, collected_test_data)

        if extra_cmdline_args is None:
            extra_cmdline_args = []

        parallel_executables = self._get_number_parallel_executables(request)
        if not parallel_executables > 0:
            logger.warning("Expected 1 or more parallel_executables, found 0. Setting to 1.")
            parallel_executables = 1

        # If there are more tests than max parallel executables, we will split them into multiple consecutive runs.
        num_iterations = int(math.ceil(len(test_spec_list) / parallel_executables))
        for iteration in range(num_iterations):
            tests_for_iteration = test_spec_list[iteration*parallel_executables:(iteration+1)*parallel_executables]
            total_threads = len(tests_for_iteration)
            threads = []
            results_per_thread = [None] * total_threads
            for i in range(total_threads):
                def make_parallel_test_func(test_spec, index, current_executable):
                    def run(request, workspace, extra_cmdline_args):
                        try:
                            results = self._exec_single_test(
                                request, workspace, current_executable, index + 1, self.log_name, test_spec,
                                extra_cmdline_args)
                        except Exception as e:
                            logger.warning(f"Found exception trying to run Editor tests. Current log name is "
                                           f"{self.log_name} and test is {str(test_spec)}")
                            raise e
                        if not results:
                            raise EditorToolsFrameworkException(f"Results not found. Current log name is "
                                                                f"{self.log_name} and test name is {str(test_spec)}")
                        results_per_thread[index] = results
                    return run

                # Duplicate the executable using the one coming from the fixture.
                current_executable = self.executable.__class__(workspace, self.executable.args.copy())
                parallel_test_function = make_parallel_test_func(tests_for_iteration[i], i, current_executable)
                parallel_test_thread = threading.Thread(
                    target=parallel_test_function, args=(request, workspace, extra_cmdline_args))
                parallel_test_thread.start()
                threads.append(parallel_test_thread)

            for parallel_test_thread in threads:
                parallel_test_thread.join()

            self._test_reporting(collected_test_data, results_per_thread, workspace, ParallelTest)

    ###################
    # SharedTest Code #
    ###################

    def _run_parallel_batched_tests(self,
                                    request: _pytest.fixtures.FixtureRequest,
                                    workspace: AbstractWorkspaceManager,
                                    collected_test_data: MultiTestSuite.TestData,
                                    test_spec_list: list[SharedTest],
                                    extra_cmdline_args: list[str] = None) -> None:
        """
        Runs multiple executables with a batch of tests for each executable (multiple executables, multiple tests each)
        This function also sets up self.executable for a given program under test.
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :collected_test_data: The TestData from calling collected_test_data()
        :test_spec_list: A list of SharedTest tests to run
        :extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if not test_spec_list:
            return        
        self._setup_test(workspace, collected_test_data)

        if extra_cmdline_args is None:
            extra_cmdline_args = []

        total_threads = self._get_number_parallel_executables(request)
        if not total_threads > 0:
            logger.warning("Expected 1 or more total_threads, found 0. Setting to 1.")
            total_threads = 1

        threads = []
        tests_per_executable = int(math.ceil(len(test_spec_list) / total_threads))
        results_per_thread = [None] * total_threads
        for iteration in range(total_threads):
            tests_for_thread = test_spec_list[iteration*tests_per_executable:(iteration+1)*tests_per_executable]

            def make_shared_test_function(test_spec_list_for_executable, index, current_executable):
                def run(request, workspace, extra_cmdline_args):
                    results = None
                    if len(test_spec_list_for_executable) > 0:
                        try:
                            results = self._exec_multitest(
                                request, workspace, current_executable, index + 1, self.log_name,
                                test_spec_list_for_executable, extra_cmdline_args)
                        except Exception as e:
                            logger.warning(f"Found exception trying to run Editor tests. Current log name is "
                                           f"{self.log_name} and tests are {str(test_spec_list_for_executable)}")
                            raise e
                        if not results:
                            raise EditorToolsFrameworkException(f"Results not found. Current log name is "
                                                                f"{self.log_name} and tests are "
                                                                f"{str(test_spec_list_for_executable)}")
                    else:
                        results = {}
                    results_per_thread[index] = results
                return run

            # Duplicate the executable using the one coming from the fixture
            current_executable = self.executable.__class__(workspace, self.executable.args.copy())
            shared_test_function = make_shared_test_function(tests_for_thread, iteration, current_executable)
            shared_test_thread = threading.Thread(
                target=shared_test_function, args=(request, workspace, extra_cmdline_args))
            shared_test_thread.start()
            threads.append(shared_test_thread)

        for shared_test_thread in threads:
            shared_test_thread.join()

        self._test_reporting(collected_test_data, results_per_thread, workspace, SharedTest)

    def _exec_multitest(self,
                        request: _pytest.fixtures.FixtureRequest,
                        workspace: AbstractWorkspaceManager,
                        executable: ly_test_tools.launchers.platforms.base.Launcher,
                        run_id: int,
                        log_name: str,
                        test_spec_list: list[AbstractTestBase],
                        cmdline_args: list[str] = None) -> dict[str, Result.ResultType]:
        """
        Starts executable with a list of tests and returns a dict of the result of every test ran within it.
        In case of failure this function also parses the executable output to find out what specific tests failed.
        :param request: The pytest request
        :param workspace: The LyTestTools Workspace object
        :param executable: The program executable under test
        :param run_id: The unique run id
        :param log_name: The name of the executable log to retrieve
        :param test_spec_list: A list of test classes to run in the same executable
        :param cmdline_args: Any additional command line args
        :return: A dict of Result objects
        """
        test_cmdline_args = self._setup_cmdline_args(cmdline_args, executable, test_spec_list, workspace)
        logs = _RunLogContext(executable, workspace, run_id, log_name)
        temp_files = []
        try:
            cmdline = []
            if logs.is_editor:
                # Store script and case lists in separate files as required by
                # the Editor, and track them before attempting to write.
                test_scripts = ";".join(editor_utils.get_testcase_module_filepath(test.test_module)
                                        for test in test_spec_list)
                test_cases = ";".join(editor_utils.compile_test_case_name(request, test)
                                      for test in test_spec_list)
                with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".txt") as script_file:
                    temp_files.append(script_file.name)
                    script_file.write(test_scripts.replace("\\", "/"))
                with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".txt") as case_file:
                    temp_files.append(case_file.name)
                    case_file.write(test_cases)
                cmdline = ["-runpythontest", temp_files[0], "-pythontestcase", temp_files[1]]
            elif logs.is_atom_tools:
                test_scripts = ";".join(editor_utils.get_testcase_module_filepath(test.test_module)
                                        for test in test_spec_list)
                cmdline = ["-runpythontest", test_scripts]
            if cmdline:
                executable.args.extend(cmdline + logs.command_line() + test_cmdline_args)
            executable.start(backupFiles=False, launch_ap=False, configure_settings=False)

            timed_out = False
            try:
                executable.wait(self.timeout_shared_test)
            except WaitTimeoutError:
                timed_out = True
                executable.stop()

            output = executable.get_output()
            executable_log_content = logs.read()
            logs.save_application_log()
            if timed_out:
                logs.save_crash_artifacts()
                return_code = None
            else:
                return_code = executable.get_returncode()

            if not timed_out and return_code == 0:
                results = {test.__name__: Result.Pass(test, output, executable_log_content)
                           for test in test_spec_list}
            else:
                results = self._get_results_using_output(test_spec_list, output, executable_log_content)
                if len(results) != len(test_spec_list):
                    raise EditorToolsFrameworkException(
                        "Error when retrieving test results, the number of results don't match the number of tests that ran.")

                if timed_out or return_code != self._test_fail_retcode:
                    crash_output = None if timed_out else logs.collect_crash(self._timeout_crash_log)
                    # The first test without a report is the likely offender.
                    # If all reported results, attribute the process exit failure
                    # to the last test instead of dereferencing a missing result.
                    offender = None
                    for name, result in results.items():
                        if isinstance(result, Result.Unknown):
                            if offender is None:
                                offender = name
                            else:
                                failure = "timed out" if timed_out else "crashed"
                                result.extra_info = (f"Test '{offender}' {failure} before this test could be executed")
                    if offender is None:
                        offender = test_spec_list[-1].__name__
                    result = results[offender]
                    if timed_out:
                        results[offender] = Result.Timeout(result.test_spec, output,
                                                           self.timeout_shared_test, result.log_output)
                    else:
                        results[offender] = Result.Crash(result.test_spec, output, return_code,
                                                         crash_output, result.log_output)

            logs.finish(results)
            return results
        finally:
            for path in temp_files:
                try:
                    os.unlink(path)
                except FileNotFoundError:
                    pass
                except OSError as exc:
                    logger.warning("Could not remove batched test list %s: %s", path, exc)

    @staticmethod
    def _get_results_using_output(test_spec_list: list[AbstractTestBase],
                                  output: str,
                                  log_output: str) -> dict[any, [Result.Unknown, Result.Pass, Result.Fail]]:
        """
        Utility function for parsing the output information from the program being tested (i.e. Editor).
        It de-serializes the JSON content printed in the output for every test and returns that information.
        :param test_spec_list: The list of test classes
        :param output: The test output
        :param log_output: The program's log output
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
                    test_spec,
                    output,
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
        :param name: Name of the test
        :param result: The Result object which denotes if the test passed or not
        :return: None
        """
        if isinstance(result, Result.Pass):
            output_str = f"Test {name}:\n{str(result)}"
            print(output_str)
        else:
            error_str = f"Test {name}:\n{str(result)}"
            pytest.fail(error_str)
