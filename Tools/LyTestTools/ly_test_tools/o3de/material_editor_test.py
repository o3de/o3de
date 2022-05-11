"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Simplified O3DE MaterialEditor test-writing utilities.

# Tests can be parallelized (run multiple MaterialEditors simultaneously) using the MaterialEditorParallelTest.
# Tests can also be batched (multiple tests run in the same MaterialEditor instance) using MaterialEditorBatchedTest.
# Results in parallel & batched runs are collated and have crash detection.
# Tests can be ran as single tests not parallel or batched using MaterialEditorSingleTest instead.
# All test classes should be subclassed inside MaterialEditorTestSuite for running MaterialEditor python test scripts.

# Usage example:
#    class MyTestSuite(MaterialEditorTestSuite):  # MaterialEditorTestSuite should always be the parent class.
#
#        class MyFirstTest(MaterialEditorSingleTest):
#            from . import script_to_be_run_by_material_editor as test_module
#
#        class MyTestInParallel_1(MaterialEditorParallelTest):
#            from . import another_script_to_be_run_by_material_editor as test_module
#
#        class MyTestInParallel_2(MaterialEditorParallelTest):
#            from . import yet_another_script_to_be_run_by_material_editor as test_module

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
import math
import os
import re
import threading
import types
import warnings

import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.o3de.editor_test_utils as editor_utils
import ly_test_tools._internal.pytest_plugin.test_tools_fixtures

from ly_test_tools.o3de.asset_processor import AssetProcessor
from ly_test_tools.launchers.exceptions import WaitTimeoutError

# This file contains ready-to-use test functions which are not actual tests, avoid pytest collection
__test__ = False

logger = logging.getLogger(__name__)


class MaterialEditorTestBase(abc.ABC):
    """
    Abstract MaterialEditor Test
    """
    # Maximum time for run, in seconds
    timeout = 180
    # Test file that this test will run
    test_module = None
    # Attach debugger when running the test, useful for debugging crashes. This should never be True on production.
    # It's also recommended to switch to MaterialEditorSingleTest for debugging in isolation
    attach_debugger = False
    # Wait until a debugger is attached at the startup of the test, this is another way of debugging.
    wait_for_debugger = False


class MaterialEditorSingleTest(MaterialEditorTestBase):
    """
    Test that will be run alone in one material_editor, with no parallel material_editor objects.
    """

    def __init__(self):
        # Extra cmdline arguments to supply to the material_editor for the test
        self.extra_cmdline_args = []
        # Whether to use null renderer, this will override use_null_renderer for the Suite if not None
        self.use_null_renderer = None

    @staticmethod
    def setup(instance, request, workspace, material_editor, material_editor_test_results, launcher_platform):
        """
        User-overrideable setup function, which will run before the test
        """
        pass

    @staticmethod
    def wrap_run(instance, request, workspace, material_editor, material_editor_test_results, launcher_platform):
        """
        User-overrideable wrapper function, which will run before and after test.
        Any code before the 'yield' statement will run before the test. With code after yield run after the test.
        """
        yield

    @staticmethod
    def teardown(instance, request, workspace, material_editor, material_editor_test_results, launcher_platform):
        """
        User-overrideable teardown function, which will run after the test
        """
        pass


class MaterialEditorSharedTest(MaterialEditorTestBase):
    """
    Test that will be run in parallel with tests in different material_editor instances, as well as serially batched
    with other tests in each material_editor instance. Minimizes total test run duration.

    Does not support per test setup/teardown to avoid creating race conditions
    """
    # Specifies if the test can be batched in the same MaterialEditor
    is_batchable = True
    # Specifies if the test can be run in multiple MaterialEditors in parallel
    is_parallelizable = True


class MaterialEditorParallelTest(MaterialEditorSharedTest):
    """
    Test that will be run in parallel with tests in different material_editor instances, though not serially batched
    with other tests in each material_editor instance. Reduces total test run duration and side effects between tests.

    Does not support per test setup/teardown to avoid creating race conditions
    """
    is_batchable = False
    is_parallelizable = True


class MaterialEditorBatchedTest(MaterialEditorSharedTest):
    """
    Test that will be batched along with the other batched tests in the same material_editor instance, though not
    executed in parallel with other material_editor instances. Reduces repeated overhead from starting MaterialEditor.

    Does not support per test setup/teardown to avoid creating race conditions.
    """
    is_batchable = True
    is_parallelizable = False


class Result:
    class MaterialEditorTestResultException(Exception):
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

        def get_material_editor_log_str(self):
            # type () -> str
            """
            Checks if the material_editor_log attribute exists and returns it.
            :return: Either the material_editor_log string or a no output message
            """
            log = getattr(self, "material_editor_log", None)
            if log:
                return log
            else:
                return "-- No MaterialEditor log found --"

    class Pass(Base):

        def __init__(self, test_spec: type(MaterialEditorTestBase), output: str, material_editor_log: str):
            """
            Represents a test success
            :test_spec: The type of MaterialEditorTestBase
            :output: The test output
            :material_editor_log: The MaterialEditor log's output
            """
            self.test_spec = test_spec
            self.output = output
            self.material_editor_log = material_editor_log

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

        def __init__(self, test_spec: type(MaterialEditorTestBase), output: str, material_editor_log: str):
            """
            Represents a normal test failure
            :test_spec: The type of MaterialEditorTestBase
            :output: The test output
            :material_editor_log: The MaterialEditor log's output
            """
            self.test_spec = test_spec
            self.output = output
            self.material_editor_log = material_editor_log

        def __str__(self):
            output = (
                f"Test FAILED\n"
                f"------------\n"
                f"|  Output  |\n"
                f"------------\n"
                f"{self.get_output_str()}\n"
                f"--------------\n"
                f"| MaterialEditor log |\n"
                f"--------------\n"
                f"{self.get_material_editor_log_str()}\n"
            )
            return output

    class Crash(Base):

        def __init__(self,
                     test_spec: type(MaterialEditorTestBase),
                     output: str,
                     ret_code: int,
                     stacktrace: str,
                     material_editor_log: str):
            """
            Represents a test which failed with an unexpected crash
            :test_spec: The type of MaterialEditorTestBase
            :output: The test output
            :ret_code: The test's return code
            :stacktrace: The test's stacktrace if available
            :material_editor_log: The MaterialEditor log's output
            """
            self.output = output
            self.test_spec = test_spec
            self.ret_code = ret_code
            self.stacktrace = stacktrace
            self.material_editor_log = material_editor_log

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
                f"| MaterialEditor log |\n"
                f"--------------\n"
                f"{self.get_material_editor_log_str()}\n"
            )
            return output

    class Timeout(Base):

        def __init__(self,
                     test_spec: type(MaterialEditorTestBase),
                     output: str,
                     time_secs: float,
                     material_editor_log: str):
            """
            Represents a test which failed due to freezing, hanging, or executing slowly
            :test_spec: The type of MaterialEditorTestBase
            :output: The test output
            :time_secs: The timeout duration in seconds
            :material_editor_log: The MaterialEditor log's output
            :return: The Timeout object
            """
            self.output = output
            self.test_spec = test_spec
            self.time_secs = time_secs
            self.material_editor_log = material_editor_log

        def __str__(self):
            output = (
                f"Test ABORTED after not completing within {self.time_secs} seconds\n"
                f"------------\n"
                f"|  Output  |\n"
                f"------------\n"
                f"{self.get_output_str()}\n"
                f"--------------\n"
                f"| MaterialEditor log |\n"
                f"--------------\n"
                f"{self.get_material_editor_log_str()}\n"
            )
            return output

    class Unknown(Base):

        def __init__(self,
                     test_spec: type(MaterialEditorTestBase),
                     output: str = None,
                     extra_info: str = None,
                     material_editor_log: str = None):
            """
            Represents a failure that the test framework cannot classify
            :test_spec: The type of MaterialEditorTestBase
            :output: The test output
            :extra_info: Any extra information as a string
            :material_editor_log: The MaterialEditor log's output
            """
            self.output = output
            self.test_spec = test_spec
            self.material_editor_log = material_editor_log
            self.extra_info = extra_info

        def __str__(self):
            output = (
                f"Indeterminate test result interpreted as failure, possible cause: {self.extra_info}\n"
                f"------------\n"
                f"|  Output  |\n"
                f"------------\n"
                f"{self.get_output_str()}\n"
                f"--------------\n"
                f"| MaterialEditor log |\n"
                f"--------------\n"
                f"{self.get_material_editor_log_str()}\n"
            )
            return output


@pytest.mark.parametrize("crash_log_watchdog", [("raise_on_crash", False)])
class MaterialEditorTestSuite:
    # Extra cmdline arguments to supply for every material_editor instance for this test suite
    global_extra_cmdline_args = ["-BatchMode", "-autotest_mode"]
    # Tests usually run with no renderer, however some tests require a renderer
    use_null_renderer = True
    # Maximum time for a single MaterialEditor to stay open on a shared test
    timeout_material_editor_shared_test = 60

    _TIMEOUT_CRASH_LOG = 20  # Maximum time (seconds) for waiting for a crash file, in seconds
    _TEST_FAIL_RETCODE = 0xF  # Return code for test failure

    class TestData:
        __test__ = False  # Required to tell PyTest to skip collecting this class even though it has "Test" in the name; avoids PyTest warnings.

        def __init__(self):
            self.results = {}  # Dict of str(test_spec.__name__) -> Result
            self.asset_processor = None

    @pytest.fixture(scope="class")
    def material_editor_test_data(self, request: _pytest.fixtures.FixtureRequest) -> MaterialEditorTestSuite.TestData:
        """
        Yields a per-testsuite structure to store the data of each test result and an AssetProcessor object that will be
        re-used on the whole suite
        :request: The Pytest request object
        :yield: The TestData object
        """
        yield from self._material_editor_test_data(request)

    def _material_editor_test_data(self, request: _pytest.fixtures.FixtureRequest) -> MaterialEditorTestSuite.TestData:
        """
        A wrapper function for unit testing of this file to call directly. Do not use in production.
        """
        test_data = MaterialEditorTestSuite.TestData()
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

    class MaterialEditorTestClass(pytest.Class):
        """
        Custom pytest collector which programmatically adds test functions based on data in the TestSuite class
        """

        def collect(self):
            """
            This collector does the following:
            1) Iterates through all the MaterialEditorSingleTest subclasses defined inside the suite.
               Adds a test function to the suite to run each separately, and report results
            2) Iterates through all the MaterialEditorSharedTest subclasses defined inside the suite,
               grouping tests based on the specs in by 3 categories: batched, parallel and batched+parallel.
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

            # Retrieve the test specs
            single_tests = self.obj.get_single_tests()
            shared_tests = self.obj.get_shared_tests()
            batched_tests = cls.filter_shared_tests(shared_tests, is_batchable=True)
            parallel_tests = cls.filter_shared_tests(shared_tests, is_parallelizable=True)
            parallel_batched_tests = cls.filter_shared_tests(shared_tests, is_parallelizable=True, is_batchable=True)

            # If user provides option to not parallelize/batch the tests, move them into single tests
            no_parallelize = self.config.getoption("--no-instance-parallel", default=False)
            no_batch = self.config.getoption("--no-instance-batch", default=False)
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

            # Add the single tests, these will run normally
            for test_spec in single_tests:
                name = test_spec.__name__

                def make_test_func(name, test_spec):
                    @set_marks({"run_type": "run_single"})
                    def single_run(self, request, workspace, material_editor, material_editor_test_data, launcher_platform):
                        # only single tests are allowed to have setup/teardown, however we can have shared tests that
                        # were explicitly set as single, for example via cmdline argument override
                        is_single_test = issubclass(test_spec, MaterialEditorSingleTest)
                        if is_single_test:
                            # Setup step for wrap_run
                            wrap = test_spec.wrap_run(
                                self, request, workspace, material_editor, material_editor_test_data, launcher_platform)
                            assert isinstance(wrap,
                                              types.GeneratorType), "wrap_run must return a generator, did you forget 'yield'?"
                            next(wrap, None)
                            # Setup step
                            test_spec.setup(
                                self, request, workspace, material_editor, material_editor_test_data, launcher_platform)
                        # Run
                        self._run_single_test(request, workspace, material_editor, material_editor_test_data, test_spec)
                        if is_single_test:
                            # Teardown
                            test_spec.teardown(
                                self, request, workspace, material_editor, material_editor_test_data, launcher_platform)
                            # Teardown step for wrap_run
                            next(wrap, None)

                    return single_run

                f = make_test_func(name, test_spec)
                if hasattr(test_spec, "pytestmark"):
                    f.pytestmark = test_spec.pytestmark
                setattr(self.obj, name, f)

            # Add the shared tests, for these we will create a runner class for storing the run information
            # that will be later used for selecting what tests runners will be run
            runners = []

            def create_runner(name, function, tests):
                runner = MaterialEditorTestSuite.Runner(name, function, tests)

                def make_func():
                    @set_marks({"runner": runner, "run_type": "run_shared"})
                    def shared_run(self, request, workspace, material_editor, material_editor_test_data, launcher_platform):
                        getattr(self, function.__name__)(request, workspace, material_editor, material_editor_test_data,
                                                         runner.tests)

                    return shared_run

                setattr(self.obj, name, make_func())

                # Add the shared tests results, these just succeed/fail based what happened on the Runner.
                for test_spec in tests:
                    def make_func(test_spec):
                        @set_marks({"runner": runner, "test_spec": test_spec, "run_type": "result"})
                        def result(self, request, workspace, material_editor, material_editor_test_data, launcher_platform):
                            # The runner must have filled the material_editor_test_data.results dict fixture for this test.
                            # Hitting this assert could mean if there was an error executing the runner
                            if test_spec.__name__ not in material_editor_test_data.results:
                                raise Result.MaterialEditorTestResultException(
                                    f"No results found for {test_spec.__name__}. "
                                    f"Test may not have ran due to the MaterialEditor "
                                    f"shutting down. Check for issues in previous "
                                    f"tests.")
                            cls._report_result(test_spec.__name__, material_editor_test_data.results[test_spec.__name__])

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
        return MaterialEditorTestSuite.MaterialEditorTestClass(name, collector)

    @classmethod
    def pytest_custom_modify_items(cls, session: _pytest.main.Session, items: list[MaterialEditorTestBase],
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
    def get_single_tests(cls) -> list[MaterialEditorSingleTest]:
        """
        Grabs all of the MaterialEditorSingleTests subclassed tests from the MaterialEditorTestSuite class
        Usage example:
           class MyTestSuite(MaterialEditorTestSuite):
               class MyFirstTest(MaterialEditorSingleTest):
                   from . import script_to_be_run_by_editor as test_module
        :return: The list of single tests
        """
        single_tests = [c[1] for c in cls.__dict__.items() if
                        inspect.isclass(c[1]) and issubclass(c[1], MaterialEditorSingleTest)]
        return single_tests

    @classmethod
    def get_shared_tests(cls) -> list[MaterialEditorSharedTest]:
        """
        Grabs all of the MaterialEditorSharedTests from the MaterialEditorTestSuite
        Usage example:
           class MyTestSuite(MaterialEditorTestSuite):
               class MyFirstTest(MaterialEditorSharedTest):
                   from . import script_to_be_run_by_editor as test_module
        :return: The list of shared tests
        """
        shared_tests = [c[1] for c in cls.__dict__.items() if
                        inspect.isclass(c[1]) and issubclass(c[1], MaterialEditorSharedTest)]
        return shared_tests

    @classmethod
    def get_session_shared_tests(cls, session: _pytest.main.Session) -> list[MaterialEditorTestBase]:
        """
        Filters and returns all of the shared tests in a given session.
        :session: The test session
        :return: The list of tests
        """
        shared_tests = cls.get_shared_tests()
        return cls.filter_session_shared_tests(session, shared_tests)

    @staticmethod
    def filter_session_shared_tests(session_items: list[_pytest.python.Function(MaterialEditorTestBase)],
                                    shared_tests: list[MaterialEditorSharedTest]) -> list[MaterialEditorTestBase]:
        """
        Retrieve the test sub-set that was collected this can be less than the original set if were overriden via -k
        argument or similars
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
    def filter_shared_tests(shared_tests: list[MaterialEditorSharedTest], is_batchable: bool = False,
                            is_parallelizable: bool = False) -> list[MaterialEditorSharedTest]:
        """
        Filters and returns all tests based off of if they are batchable and/or parallelizable
        :shared_tests: All shared tests
        :is_batchable: Filter to batchable tests
        :is_parallelizable: Filter to parallelizable tests
        :return: The list of filtered tests
        """
        return [
            t for t in shared_tests if (
                    getattr(t, "is_batchable", None) is is_batchable
                    and
                    getattr(t, "is_parallelizable", None) is is_parallelizable
            )
        ]

    def _prepare_asset_processor(self, workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                                 material_editor_test_data: TestData) -> None:
        """
        Prepares the asset processor for the test depending on whether or not the process is open and if the current
        test owns it.
        :workspace: The workspace object in case an AssetProcessor object needs to be created
        :material_editor_test_data: The test data from calling material_editor_test_data()
        :return: None
        """
        try:
            # Start-up an asset processor if we are not running one
            # If another AP process exist, don't kill it, as we don't own it
            if material_editor_test_data.asset_processor is None:
                if not process_utils.process_exists("AssetProcessor", ignore_extensions=True):
                    editor_utils.kill_all_ly_processes(include_asset_processor=True)
                    material_editor_test_data.asset_processor = AssetProcessor(workspace)
                    material_editor_test_data.asset_processor.start()
                else:
                    editor_utils.kill_all_ly_processes(include_asset_processor=False)
            else:
                # Make sure the asset processor from before wasn't closed by accident
                material_editor_test_data.asset_processor.start()
        except Exception as ex:
            material_editor_test_data.asset_processor = None
            raise ex

    def _setup_instance_test(self,
                           material_editor: ly_test_tools.launchers.platforms.base.Launcher,
                           workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                           material_editor_test_data: TestData) -> None:
        """
        Sets up an editor test by preparing the Asset Processor, killing all other O3DE processes, and configuring
        :material_editor: The MaterialEditor launcher object
        :workspace: The test Workspace object
        :material_editor_test_data: The TestData from calling material_editor_test_data()
        :return: None
        """
        self._prepare_asset_processor(workspace, material_editor_test_data)
        editor_utils.kill_all_ly_processes(include_asset_processor=False)
        material_editor.configure_settings()

    @staticmethod
    def _get_results_using_output(test_spec_list: list[MaterialEditorTestBase],
                                  output: str,
                                  material_editor_log_content: str
                                  ) -> dict[str, Result]:
        """
        Utility function for parsing the output information from the MaterialEditor.
        It deserializes the JSON content printed in the output for every test and returns that information.
        :test_spec_list: The list of MaterialEditorTest objects
        :output: The material_editor from material_editor.get_output()
        :material_editor_log_content: The contents of the MaterialEditor log as a string
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
        log_matches = pattern.finditer(material_editor_log_content)
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
                    f"Found no test run information on stdout for {name} in the MaterialEditor log",
                    material_editor_log_content)
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
                cur_log = material_editor_log_content[log_start: end]
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

    def _exec_instance_test(self,
                          request: _pytest.fixtures.FixtureRequest,
                          workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                          material_editor: ly_test_tools.launchers.platforms.base.Launcher,
                          run_id: int,
                          log_name: str,
                          test_spec: MaterialEditorTestBase,
                          cmdline_args: list[str] = None) -> dict[str, Result]:
        """
        Starts the editor with the given test and returns an result dict with a single element specifying the result
        :request: The pytest request
        :workspace: The LyTestTools Workspace object
        :material_editor: The MaterialEditor launcher object
        :run_id: The unique run id
        :log_name: The name of the editor log to retrieve
        :test_spec: The type of MaterialEditorTestBase
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
        material_editor.args.extend(cmdline)
        material_editor.start(backupFiles=False, launch_ap=False, configure_settings=False)

        try:
            material_editor.wait(test_spec.timeout)
            output = material_editor.get_output()
            return_code = material_editor.get_returncode()
            material_editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)
            # Save the editor log
            workspace.artifact_manager.save_artifact(
                os.path.join(editor_utils.retrieve_log_path(run_id, workspace), log_name),
                f'({run_id}){log_name}')
            if return_code == 0:
                test_result = Result.Pass(test_spec, output, material_editor_log_content)
            else:
                has_crashed = return_code != MaterialEditorTestSuite._TEST_FAIL_RETCODE
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
                    test_result = Result.Fail(test_spec, output, material_editor_log_content)
        except WaitTimeoutError:
            output = material_editor.get_output()
            material_editor.stop()
            material_editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)
            test_result = Result.Timeout(test_spec, output, test_spec.timeout, material_editor_log_content)

        material_editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)
        results = self._get_results_using_output([test_spec], output, material_editor_log_content)
        results[test_spec.__name__] = test_result
        return results

    def _exec_instance_multitest(self,
                               request: _pytest.fixtures.FixtureRequest,
                               workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                               material_editor: ly_test_tools.launchers.platforms.base.Launcher,
                               run_id: int,
                               log_name: str,
                               test_spec_list: list[MaterialEditorTestBase],
                               cmdline_args: list[str] = None) -> dict[str, Result]:
        """
        Starts a MaterialEditor executable with a list of tests and returns a dict of the result of every test ran
        within that material_editor instance. In case of failure this function also parses the material_editor output
        to find out what specific tests failed.
        :request: The pytest request
        :workspace: The LyTestTools Workspace object
        :material_editor: The MaterialEditor launcher object
        :run_id: The unique run id
        :log_name: The name of the editor log to retrieve
        :test_spec_list: A list of MaterialEditorTestBase tests to run in the same material_editor instance
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

        material_editor.args.extend(cmdline)
        material_editor.start(backupFiles=False, launch_ap=False, configure_settings=False)

        output = ""
        material_editor_log_content = ""
        try:
            material_editor.wait(self.timeout_material_editor_shared_test)
            output = material_editor.get_output()
            return_code = material_editor.get_returncode()
            material_editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)
            # Save the MaterialEditor log
            try:
                workspace.artifact_manager.save_artifact(
                    os.path.join(editor_utils.retrieve_log_path(run_id, workspace), log_name), f'({run_id}){log_name}')
            except FileNotFoundError:
                # Error logging is already performed and we don't want this to fail the test
                pass
            if return_code == 0:
                # No need to scrape the output, as all the tests have passed
                for test_spec in test_spec_list:
                    results[test_spec.__name__] = Result.Pass(test_spec, output, material_editor_log_content)
            else:
                # Scrape the output to attempt to find out which tests failed.
                # This function should always populate the result list, if it didn't find it, it will have "Unknown" type of result
                results = self._get_results_using_output(test_spec_list, output, material_editor_log_content)
                assert len(results) == len(
                    test_spec_list), "bug in _get_results_using_output(), the number of results don't match the tests ran"

                # If the editor crashed, find out in which test it happened and update the results
                has_crashed = return_code != MaterialEditorTestSuite._TEST_FAIL_RETCODE
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
                                                                       crash_error, result.material_editor_log)
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
                                                               crash_error, crashed_result.material_editor_log)
        except WaitTimeoutError:
            material_editor.stop()
            output = material_editor.get_output()
            material_editor_log_content = editor_utils.retrieve_editor_log_content(run_id, log_name, workspace)

            # The editor timed out when running the tests, get the data from the output to find out which ones ran
            results = self._get_results_using_output(test_spec_list, output, material_editor_log_content)
            assert len(results) == len(
                test_spec_list), "bug in _get_results_using_output(), the number of results don't match the tests ran"
            # Similar logic here as crashes, the first test that has no result is the one that timed out
            timed_out_result = None
            for test_spec_name, result in results.items():
                if isinstance(result, Result.Unknown):
                    if not timed_out_result:
                        results[test_spec_name] = Result.Timeout(result.test_spec, result.output,
                                                                 self.timeout_material_editor_shared_test,
                                                                 result.material_editor_log)
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
                                                         self.timeout_material_editor_shared_test,
                                                         result.material_editor_log)
        return results

    def _run_single_test(self, request: _pytest.fixtures.FixtureRequest,
                         workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                         material_editor: ly_test_tools.launchers.platforms.base.Launcher,
                         material_editor_test_data: TestData,
                         test_spec: MaterialEditorSingleTest) -> None:
        """
        Runs a single test (one material_editor, one test) with the given specs
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :material_editor: The MaterialEditor launcher object
        :material_editor_test_data: The TestData from calling material_editor_test_data()
        :test_spec: The test class that should be a subclass of MaterialEditorSingleTest
        :return: None
        """
        self._setup_instance_test(material_editor, workspace, material_editor_test_data)
        extra_cmdline_args = []
        if hasattr(test_spec, "extra_cmdline_args"):
            extra_cmdline_args = test_spec.extra_cmdline_args

        result = self._exec_instance_test(request, workspace, material_editor, 1, "material_editor_test.log", test_spec,
                                        extra_cmdline_args)
        if result is None:
            logger.error(f"Unexpectedly found no test run in the editor log during {test_spec}")
            result = {"Unknown":
                Result.Unknown(
                    test_spec=test_spec,
                    extra_info="Unexpectedly found no test run information on stdout in the MaterialEditor log")}
        material_editor_test_data.results.update(result)
        test_name, test_result = next(iter(result.items()))
        self._report_result(test_name, test_result)
        # If test did not pass, save assets with errors and warnings
        if not isinstance(test_result, Result.Pass):
            editor_utils.save_failed_asset_joblogs(workspace)

    def _run_batched_tests(self,
                           request: _pytest.fixtures.FixtureRequest,
                           workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                           material_editor: ly_test_tools.launchers.platforms.base.Launcher,
                           material_editor_test_data: TestData,
                           test_spec_list: list[MaterialEditorSharedTest],
                           extra_cmdline_args: list[str] = None) -> None:
        """
        Runs a batch of tests in one single editor with the given spec list (one material_editor, multiple tests)
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :material_editor: The MaterialEditor launcher object
        :material_editor_test_data: The TestData from calling material_editor_test_data()
        :test_spec_list: A list of MaterialEditorSharedTest tests to run
        :extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if extra_cmdline_args is None:
            extra_cmdline_args = []

        if not test_spec_list:
            return

        self._setup_instance_test(material_editor, workspace, material_editor_test_data)
        results = self._exec_instance_multitest(
            request, workspace, material_editor, 1, "material_editor_test.log", test_spec_list, extra_cmdline_args)
        material_editor_test_data.results.update(results)
        # If at least one test did not pass, save assets with errors and warnings
        for result in results:
            if result is None:
                logger.error("Unexpectedly found no test run in the editor log during MaterialEditorBatchedTest")
                logger.debug(f"Results from MaterialEditorBatchedTest:\n{results}")
            if not isinstance(result, Result.Pass):
                editor_utils.save_failed_asset_joblogs(workspace)
                return  # exit early on first batch failure

    def _run_parallel_tests(self,
                            request: _pytest.fixtures.FixtureRequest,
                            workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                            material_editor: ly_test_tools.launchers.platforms.base.Launcher,
                            material_editor_test_data: TestData,
                            test_spec_list: list[MaterialEditorSharedTest],
                            extra_cmdline_args: list[str] = None) -> None:
        """
        Runs multiple editors with one test on each editor (multiple material_editor objects, one test each)
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :material_editor: The MaterialEditor launcher object
        :material_editor_test_data: The TestData from calling material_editor_test_data()
        :test_spec_list: A list of MaterialEditorSharedTest tests to run
        :extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if extra_cmdline_args is None:
            extra_cmdline_args = []

        if not test_spec_list:
            return

        self._setup_instance_test(material_editor, workspace, material_editor_test_data)
        parallel_editors = self._get_number_parallel_instances(request, "--material-editors-parallel")
        assert parallel_editors > 0, "Must have at least one editor"

        # If there are more tests than max parallel editors, we will split them into multiple consecutive runs
        num_iterations = int(math.ceil(len(test_spec_list) / parallel_editors))
        for iteration in range(num_iterations):
            tests_for_iteration = test_spec_list[iteration * parallel_editors:(iteration + 1) * parallel_editors]
            total_threads = len(tests_for_iteration)
            threads = []
            results_per_thread = [None] * total_threads
            for i in range(total_threads):
                def make_func(test_spec, index, my_material_editor):
                    def run(request, workspace, extra_cmdline_args):
                        results = self._exec_instance_test(
                            request, workspace, my_material_editor, index + 1, "material_editor_test.log", test_spec,
                            extra_cmdline_args)
                        assert results is not None
                        results_per_thread[index] = results

                    return run

                # Duplicate the editor using the one coming from the fixture
                cur_editor = material_editor.__class__(workspace, material_editor.args.copy())
                f = make_func(tests_for_iteration[i], i, cur_editor)
                t = threading.Thread(target=f, args=(request, workspace, extra_cmdline_args))
                t.start()
                threads.append(t)

            for t in threads:
                t.join()

            save_asset_logs = False

            for result in results_per_thread:
                if result is None:
                    logger.error("Unexpectedly found no test run in the editor log during MaterialEditorParallelTest")
                    logger.debug(f"Results from MaterialEditorParallelTest thread:\n{results_per_thread}")
                    result = {"Unknown":
                        Result.Unknown(
                            test_spec=MaterialEditorParallelTest,
                            extra_info="Unexpectedly found no test run information on stdout in the editor log")}
                material_editor_test_data.results.update(result)
                if not isinstance(result, Result.Pass):
                    save_asset_logs = True
            # If at least one test did not pass, save assets with errors and warnings
            if save_asset_logs:
                editor_utils.save_failed_asset_joblogs(workspace)

    def _run_parallel_batched_tests(self,
                                    request: _pytest.fixtures.FixtureRequest,
                                    workspace: ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager,
                                    material_editor: ly_test_tools.launchers.platforms.base.Launcher,
                                    material_editor_test_data: TestData,
                                    test_spec_list: list[MaterialEditorSharedTest],
                                    extra_cmdline_args: list[str] = None) -> None:
        """
        Runs multiple editors with a batch of tests for each editor (multiple material_editor objects, multiple tests each)
        :request: The Pytest Request
        :workspace: The LyTestTools Workspace object
        :material_editor: The LyTestTools material_editor launcher object
        :material_editor_test_data: The TestData from calling material_editor_test_data()
        :test_spec_list: A list of MaterialEditorSharedTest tests to run
        :extra_cmdline_args: Any extra command line args in a list
        :return: None
        """
        if extra_cmdline_args is None:
            extra_cmdline_args = []

        if not test_spec_list:
            return

        self._setup_instance_test(material_editor, workspace, material_editor_test_data)
        total_threads = self._get_number_parallel_instances(request, "--material-editors-parallel")
        assert total_threads > 0, "Must have at least one editor"
        threads = []
        tests_per_editor = int(math.ceil(len(test_spec_list) / total_threads))
        results_per_thread = [None] * total_threads
        for i in range(total_threads):
            tests_for_thread = test_spec_list[i * tests_per_editor:(i + 1) * tests_per_editor]

            def make_func(test_spec_list_for_material_editor, index, my_material_editor):
                def run(request, workspace, extra_cmdline_args):
                    results = None
                    if len(test_spec_list_for_material_editor) > 0:
                        results = self._exec_instance_multitest(request, workspace, my_material_editor, index + 1,
                                                              "material_editor_test.log",
                                                              test_spec_list_for_material_editor, extra_cmdline_args)
                        assert results is not None
                    else:
                        results = {}
                    results_per_thread[index] = results

                return run

            # Duplicate the editor using the one coming from the fixture
            cur_editor = material_editor.__class__(workspace, material_editor.args.copy())
            f = make_func(tests_for_thread, i, cur_editor)
            t = threading.Thread(target=f, args=(request, workspace, extra_cmdline_args))
            t.start()
            threads.append(t)

        for t in threads:
            t.join()

        save_asset_logs = False
        for result in results_per_thread:
            if result is None:
                logger.error("Unexpectedly found no test run in the editor log during MaterialEditorSharedTest")
                logger.debug(f"Results from MaterialEditorSharedTest thread:\n{results_per_thread}")
                result = {"Unknown":
                    Result.Unknown(
                        test_spec=MaterialEditorSharedTest,
                        extra_info="Unexpectedly found no test run information on stdout in the MaterialEditor log")}
            material_editor_test_data.results.update(result)
            if not isinstance(result, Result.Pass):
                save_asset_logs = True
        # If at least one test did not pass, save assets with errors and warnings
        if save_asset_logs:
            editor_utils.save_failed_asset_joblogs(workspace)
