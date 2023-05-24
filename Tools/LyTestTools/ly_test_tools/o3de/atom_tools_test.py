"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test-writing utilities that simplify creating O3DE Atom tools executable tests in Python.

Test writers should subclass a test suite from AtomToolsTestSuite to hold the specification of python test scripts
for the executable to load and run.
Tests can be parallelized (run in multiple executable instances at once) and/or
batched (multiple tests run in the same executable instance), with collated results and crash detection.
Tests retain the ability to be run as a single test in a single executable instance as well.

Usage example:
   class MyTestSuite(AtomToolsTestSuite):

       class MyFirstTest(AtomToolsSingleTest):
           from . import script_to_be_run_by_executable as test_module

       class MyTestInParallel_1(AtomToolsBatchedTest):
           from . import another_script_to_be_run_by_executable as test_module

       class MyTestInParallel_2(AtomToolsParallelTest):
           from . import yet_another_script_to_be_run_by_executable as test_module
"""

from __future__ import annotations
__test__ = False  # Avoid pytest collection & warnings since this module is for test functions, but not a test itself.

import logging

import pytest
import _pytest.python
import _pytest.outcomes

from ly_test_tools._internal.managers.workspace import AbstractWorkspaceManager
from ly_test_tools.o3de.multi_test_framework import MultiTestSuite, SharedTest, SingleTest

logger = logging.getLogger(__name__)


class AtomToolsSingleTest(SingleTest):
    """
    Test that will run alone in one Atom tools executable with no parallel Atom tools executables,
    limiting environmental side-effects at the expense of redundant isolated work
    """

    def __init__(self):
        super(AtomToolsSingleTest, self).__init__()
        # Extra cmdline arguments to supply to the Atom tools executable for the test
        self.extra_cmdline_args = []
        # Whether to use null renderer, this will override use_null_renderer for the Suite if not None
        self.use_null_renderer = None

    @staticmethod
    def setup(instance: AtomToolsTestSuite.MultiTestCollector,
              request: _pytest.fixtures.FixtureRequest,
              workspace: AbstractWorkspaceManager) -> None:
        """
        User-overrideable setup function, which will run before the test.
        :param instance: Parent AtomToolsTestSuite.MultiTestCollector instance executing the test
        :param request: PyTest request object
        :param workspace: LyTestTools workspace manager
        """
        pass

    @staticmethod
    def wrap_run(instance: AtomToolsTestSuite.MultiTestCollector,
                 request: _pytest.fixtures.FixtureRequest,
                 workspace: AbstractWorkspaceManager,
                 test_results: MultiTestSuite.TestData) -> None:
        """
        User-overrideable wrapper function, which will run both before and after test.
        Any code before the 'yield' statement will run before the test. With code after yield run after the test.
        Setup will run before wrap_run starts. Teardown will run after it completes.
        :param instance: Parent AtomToolsTestSuite.MultiTestCollector instance executing the test
        :param request: PyTest request object
        :param workspace: LyTestTools workspace manager
        :param test_results: Currently recorded Atom tools executable test results
        """
        yield

    @staticmethod
    def teardown(instance: AtomToolsTestSuite.MultiTestCollector,
                 request: _pytest.fixtures.FixtureRequest,
                 workspace: AbstractWorkspaceManager,
                 test_results: MultiTestSuite.TestData) -> None:
        """
        User-overrideable teardown function, which will run after the test
        :param instance: Parent AtomToolsTestSuite.MultiTestCollector instance executing the test
        :param request: PyTest request object
        :param workspace: LyTestTools workspace manager
        :param test_results: Currently recorded Atom tools executable test results
        """
        pass


class AtomToolsSharedTest(SharedTest):
    """
    Test that will run in parallel with tests in different Atom tools executable instances, as well as serially batching
    with other tests in each Atom tools executable instance. Minimizes total test run duration.

    Does not support per test setup/teardown to avoid creating race conditions
    """
    # Specifies if the test can be batched in the same Atom tools executable
    is_batchable = True
    # Specifies if the test can be run in multiple Atom tools executables in parallel
    is_parallelizable = True


class AtomToolsParallelTest(AtomToolsSharedTest):
    """
    Test that will run in parallel with tests in different Atom tools executable instances,
    though not serially batched with other tests in each Atom tools executable instance.
    Reduces total test run duration, while limiting side-effects between tests.

    Does not support per test setup/teardown to avoid creating race conditions
    """
    is_batchable = False
    is_parallelizable = True


class AtomToolsBatchedTest(AtomToolsSharedTest):
    """
    Test that will run serially batched with the tests in the same Atom tools executable instance,
    though not executed in parallel with other Atom tools executable instances.
    Reduces overhead from starting the Atom tools executable, while limiting side-effects between executables.

    Does not support per test setup/teardown to avoid creating race conditions
    """
    is_batchable = True
    is_parallelizable = False


class AtomToolsTestSuite(MultiTestSuite):
    """
    This class defines the values needed in order to execute a batched, parallel, or single Atom tools executable test.
    Any new test cases written that inherit from this class can override these values for their newly created class.
    """
    # Extra cmdline arguments to supply for every Atom tools executable for this test suite.
    global_extra_cmdline_args = ["-BatchMode", "-autotest_mode"]
    # Tests usually run with no renderer, however some tests require a renderer and will disable this.
    use_null_renderer = True
    # Name of the Atom tools executable's log file.
    log_name = ""
    # Atom Tools executable name to open with the Atom Tools launchers.
    executable_name = ""
    # Maximum time in seconds for a single Atom tools executable to stay open across the set of shared tests.
    timeout_shared_test = 900
    # Maximum time (seconds) for waiting for a crash file to finish being dumped to disk.
    _timeout_crash_log = 20
    # Return code for test failure.
    _test_fail_retcode = 0xF
    # Test class to use for single test collection.
    _single_test_class = AtomToolsSingleTest
    # Test class to use for shared test collection.
    _shared_test_class = AtomToolsSharedTest

    @pytest.mark.parametrize("crash_log_watchdog", [("raise_on_crash", False)])
    def pytest_multitest_makeitem(
            collector: _pytest.python.Module, name: str, obj: object) -> AtomToolsTestSuite.MultiTestCollector:
        """
        Enables ly_test_tools._internal.pytest_plugin.multi_testing.pytest_pycollect_makeitem to collect the tests
        defined by this suite.
        This is required for any test suite that inherits from the MultiTestSuite class else the tests won't be
        collected for that suite when using the ly_test_tools.o3de.multi_test_framework module.
        :param collector: Module that serves as the pytest test class collector
        :param name: Name of the parent test class
        :param obj: Module of the test to be run
        :return: AtomToolsTestSuite.MultiTestCollector
        """
        return AtomToolsTestSuite.MultiTestCollector.from_parent(parent=collector, name=name)
