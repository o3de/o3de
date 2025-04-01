"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Generic fixtures bundled with LyTestTools
These fixtures will be available to the end user without requiring to import any file.

"""
import getpass
import logging
import os
import socket
from datetime import datetime

import pytest

import ly_test_tools._internal.pytest_plugin
import ly_test_tools._internal.log.py_logging_util as py_logging_util
import ly_test_tools._internal.managers.ly_process_killer as ly_process_killer
import ly_test_tools.builtin.helpers as helpers
import ly_test_tools.environment.file_system
import ly_test_tools.launchers.launcher_helper
import ly_test_tools.launchers.platforms.base
import ly_test_tools.environment.watchdog
from ly_test_tools import ALL_PLATFORM_OPTIONS, HOST_OS_PLATFORM

logger = logging.getLogger(__name__)

TIMESTAMP_FORMAT = '%Y-%m-%dT%H-%M-%S-%f'  # ISO with colon and period replaced to dash
TOOLS_INFO_LOG_NAME = "ToolsInfo.log"
TOOLS_DEBUG_LOG_NAME = "ToolsDebug.log"


def pytest_addoption(parser):
    """
    Adds CLI options to launch with pytest.
    Pytest will find and visit this function during its initialization
    :param parser: pytest-provided fixture for argparse.ArgumentParser
    """
    parser.addoption("--output-path",
                     help="A folder for test artifacts and logs")
    parser.addoption("--build-directory", nargs='?', default='',
                     help="An existing CMake binary output directory which contains the lumberyard executables,"
                          "such as: D:/ly/dev/windows_vs2017/bin/profile/")


def pytest_configure(config):
    """
    Save custom CLI options during Pytest configuration, so they are later accessible without using fixtures
    Pytest will find and visit this function after initialization
    :param config: pytest-provided fixture for configured arguments
    """
    ly_test_tools._internal.pytest_plugin.build_directory = _get_build_directory(config)
    ly_test_tools._internal.pytest_plugin.output_path = _get_output_path(config)

    # patch to work around https://github.com/pytest-dev/pytest/issues/10604
    monkeypatch_pytest()


def _get_build_directory(config):
    """
    Fetch and verify the cmake build directory CLI arg, without creating an error when unset
    :param config: pytest config object
    """
    custom_build_directory = config.getoption('--build-directory', '')

    if custom_build_directory:
        logger.debug(f'Custom build directory set via cli arg to: {custom_build_directory}')
        if not os.path.exists(custom_build_directory):
            raise ValueError(f'Pytest custom argument "--build-directory" does not exist at: {custom_build_directory}')
        if custom_build_directory.endswith('debug'):
            pytest.exit("Python debug modules are not available. LyTestTools test skipped.", 0)
    else:
        # only warn when unset, allowing non-LyTT tests to still use pytest
        logger.warning(f'Pytest custom argument "--build-directory" was not provided, tests using LyTestTools will fail')

    return custom_build_directory


def _get_output_path(config):
    """
    Fetch and verify the CLI arg for the path where tests artifacts are saved, without creating an error when unset
    :param config: pytest config object
    """
    custom_output_path = config.getoption("--output-path")
    if custom_output_path:
        logger.debug(f'Custom output_path set to: {str(custom_output_path)}')
        output_path = custom_output_path
    else:
        custom_build_directory = config.getoption('--build-directory', '')
        if custom_build_directory: # default into known build folder
            default_output_path = os.path.join(custom_build_directory, 'Testing', 'LyTestTools')
        else:  # should already create a separate warning in _get_build_directory()
            default_output_path = os.getcwd()
        output_path = os.path.join(default_output_path,
                                   "pytest_results",
                                   datetime.now().strftime(TIMESTAMP_FORMAT))
        logger.warning(f'Pytest custom argument "--output-path" was not provided, '
                       f'defaulting Test Results output to: {str(output_path)}')

    os.makedirs(output_path, exist_ok=True)
    return output_path


@pytest.fixture(scope="session")
def record_suite_property(request):
    """
    Sets a global property at the Suite level in pytest's internal report, adding it if it does not yet exist else
    overwriting all current values.
    These properties become part of the test report and are available to the configured reporters, e.g. JUnit XML.
    The fixture is callable with ``(name, value)``, with value being automatically xml-encoded.

    Example::

        def test_function(record_suite_property):
            record_suite_property("example_key", 1)
    """
    return _record_suite_property(request)


def _record_suite_property(request):
    """Separate implementation to call directly during unit tests"""
    xml = getattr(request.config, "_xml", None)
    if xml is not None:
        def set_prop(name, value):
            property_exists = [propname for propname in xml.global_properties if propname[0] == name]
            if property_exists:
                for prop in range(len(xml.global_properties)):
                    if xml.global_properties[prop][0] == name:
                        xml.global_properties[prop] = name, value
            else:
                xml.add_global_property(name, value)

        return set_prop
    else:
        def set_prop_noop(name, value):
            logger.debug(
                f"Pytest junitxml unexpectedly not configured, unable to add global suite property '{name}' "
                f"with value '{value}'")

        return set_prop_noop


@pytest.fixture(scope="session")
def output_path(request):
    """
    Returns the desired name for the folder that will store the results of the tests.  If no name is provided, it will
    default to naming the folder after the timestamp at the moment this test session started. Datetime as string in the
    format YYYY-MM-DDTHH_MM_SS_F This fixture is useful to have an unique ID for the current session.

    :return: custom logs path, defaulting to current timestamp
    """
    return ly_test_tools._internal.pytest_plugin.output_path


@pytest.fixture
def build_directory(request):
    return ly_test_tools._internal.pytest_plugin.build_directory


@pytest.fixture
def record_build_name(record_suite_property):
    """
    Updates build name in the pytest XML results
    :return: function which accepts parameter build_name
    """
    return _record_build_name(record_suite_property)


def _record_build_name(record_suite_property):
    """Separate implementation to call directly during unit tests"""
    return lambda build_name: record_suite_property("build", build_name)


@pytest.fixture(autouse=True)
def record_test_timestamp(record_property):
    """
    Adds a timestamp to the current test in the XML report
    """
    return _record_test_timestamp(record_property)


def _record_test_timestamp(record_property):
    """Separate implementation to call directly during unit tests"""
    record_property("timestamp", datetime.now().strftime(TIMESTAMP_FORMAT))


@pytest.fixture(scope="session", autouse=True)
def record_suite_data(record_suite_property):
    """
    Sets default suite information for pytest's XML output
    """
    return _record_suite_data(record_suite_property)


def _record_suite_data(record_suite_property):
    """Separate implementation to call directly during unit tests"""
    record_suite_property("timestamp", str(datetime.now().strftime(TIMESTAMP_FORMAT)))
    record_suite_property("hostname", str(socket.gethostname()))
    record_suite_property("username", str(getpass.getuser()))


@pytest.fixture(scope="function")
def editor(request, workspace, crash_log_watchdog):
    # type: (...) -> ly_test_tools.launchers.platforms.base.Launcher

    return _editor(
        request=request,
        workspace=workspace,
        launcher_platform=get_fixture_argument(request, 'launcher_platform', HOST_OS_PLATFORM))


def _editor(request, workspace, launcher_platform):
    """Separate implementation to call directly during unit tests"""
    editor = ly_test_tools.launchers.launcher_helper.create_editor(workspace, launcher_platform)

    def teardown():
        editor.stop()

    request.addfinalizer(teardown)

    return editor


def get_fixture_argument(request, argument_name, default_value):
    if argument_name in request.fixturenames:
        return request.getfixturevalue(argument_name)
    return default_value


@pytest.fixture(scope="function")
def launcher(request, workspace, crash_log_watchdog):
    # type: (...) -> ly_test_tools.launchers.platforms.base.Launcher
    return _launcher(
        request=request,
        workspace=workspace,
        launcher_platform=get_fixture_argument(request, 'launcher_platform', HOST_OS_PLATFORM),
        level=get_fixture_argument(request, 'level', ''))


def _launcher(request, workspace, launcher_platform, level=""):
    """Separate implementation to call directly during unit tests"""

    if not level:
        launcher = ly_test_tools.launchers.launcher_helper.create_game_launcher(
            workspace, launcher_platform)
    else:
        launcher = ly_test_tools.launchers.launcher_helper.create_game_launcher(
            workspace, launcher_platform, ['+LoadLevel', level])

    def teardown():
        launcher.stop()

    request.addfinalizer(teardown)

    return launcher


@pytest.fixture
def automatic_process_killer(request):
    # type: (_pytest.fixtures.SubRequest) -> ly_process_killer
    """
    Automatically kills existing Lumberyard processes before a test.
    Relies on parametrizing "processes_to_kill" with a list of process names to kill.
    If no "processes_to_kill" is found, it will default to ly_process_killer.LY_PROCESS_KILL_LIST instead.
    :param request: _pytest.fixtures.SubRequest request object.
    :return: ly_process_killer module.
    """
    processes_to_kill = get_fixture_argument(request, 'processes_to_kill', ly_process_killer.LY_PROCESS_KILL_LIST)
    return _automatic_process_killer(processes_to_kill)


def _automatic_process_killer(processes_to_kill):
    # type: (list) -> ly_process_killer
    """
    Separate implementation to call directly during unit tests.
    """
    # Detect processes.
    processes_detected = ly_process_killer.detect_lumberyard_processes(processes_list=processes_to_kill)

    # Kill processes.
    ly_process_killer.kill_processes(processes_list=processes_detected)

    return ly_process_killer


@pytest.fixture(scope="function")
def workspace(request,  # type: _pytest.fixtures.SubRequest
              build_directory,  # type: str
              project,  # type: str
              record_property,  # type: _pytest.junitxml.record_property
              record_build_name,  # type: ly_test_tools._internal.pytest_plugin.test_tools_fixtures.record_build_name
              output_path,  # type: ly_test_tools._internal.pytest_plugin.test_tools_fixtures.output_path
              asset_processor_platform,  # type: str
              ):
    # type: (...) -> ly_test_tools._internal.managers.workspace.WorkspaceManager
    """
    Create a new platform-specific workspace manager for the current lumberyard build, and configure log reporting.
    Expects that Lumberyard has already been built for the target platform, configuration, project, and spec

    :param request: _pytest.fixtures.SubRequest request object
    :param build_directory: path to the build directory
    :param project: Project name to use
    :param record_property: PyTest record_property fixture
    :param record_build_name: LyTestTools record_build_name fixture
    :param output_path: LyTestTools output_path fixture
    :param asset_processor_platform: name of the platform to target for the AssetProcessor

    :return: A fully configured workspace manager
    """

    return _workspace(
        request, build_directory, project, record_property, record_build_name, output_path, asset_processor_platform)


def _workspace(request,  # type: _pytest.fixtures.SubRequest
               build_directory, # type: str
               project,  # type: str
               record_property,  # type: _pytest.junitxml.record_property
               record_build_name,  # type: ly_test_tools._internal.pytest_plugin.test_tools_fixtures.record_build_name
               output_path,  # type: ly_test_tools._internal.pytest_plugin.test_tools_fixtures.output_path
               asset_processor_platform,  # type: str
               ):
    """Separate implementation to call directly during unit tests"""

    # Convert build directory to absolute path in case it was provided as relative path
    build_directory = os.path.abspath(build_directory)

    workspace = helpers.create_builtin_workspace(
        build_directory=build_directory,
        project=project,
        output_path=output_path,
    )

    # Build names for test artifact folders:
    test_module = request.node.module.__name__.split('.')[-1]
    test_class = request.node.getmodpath().split('.')[0]
    test_method = request.node.originalname
    test_name = workspace.artifact_manager.generate_folder_name(
        test_module, test_class, test_method)

    def teardown():
        log_name = f"{test_name}-logs"
        log_path = os.path.join(workspace.artifact_manager.artifact_path, log_name)

        record_build_name(HOST_OS_PLATFORM)

        path = os.path.basename(workspace.artifact_manager.gather_artifacts(log_path))
        record_property("log", path)

        py_logging_util.terminate_logging()

        workspace.artifact_manager.set_dest_path()  # Reset log name for this test
        helpers.teardown_builtin_workspace(workspace)

    artifact_folder_count = request.session.testscollected  # Amount of folders to create for test_name.
    # Skip workspace logging if batch or parallel test by checking the list of test names
    if hasattr(request.node.cls, '_runners'):
        for runner in request.node.cls._runners:
            for func in runner.result_pytestfuncs:
                if test_method == func.originalname:
                    return workspace
    request.addfinalizer(teardown)

    helpers.setup_builtin_workspace(workspace, test_name, artifact_folder_count)

    # Must be called after helpers.setup_builtin_workspace() above:
    info_log_path = workspace.artifact_manager.generate_artifact_file_name(TOOLS_INFO_LOG_NAME)
    debug_log_path = workspace.artifact_manager.generate_artifact_file_name(TOOLS_DEBUG_LOG_NAME)
    py_logging_util.initialize_logging(info_log_path, debug_log_path)

    # Bind the newly created log files to the workspace.
    workspace.info_log_path = info_log_path
    workspace.debug_log_path = debug_log_path
    workspace.asset_processor_platform = asset_processor_platform

    return workspace


@pytest.fixture(scope="function")
def crash_log_watchdog(request, workspace):
    # type: (...) -> ly_test_tools.environment.watchdog.CrashLogWatchdog
    return _crash_log_watchdog(request, workspace, get_fixture_argument(request, 'raise_on_crash', True))


def _crash_log_watchdog(request, workspace, raise_on_crash):
    """Separate implementation to call directly during unit tests"""
    error_log = workspace.paths.crash_log()
    crash_log_watchdog = ly_test_tools.environment.watchdog.CrashLogWatchdog(
        error_log, raise_on_condition=raise_on_crash)

    def teardown():
        # stop() will either raise an exception or log an error if watchdog has found an error log
        crash_log_watchdog.stop()

    request.addfinalizer(teardown)
    crash_log_watchdog.start()

    return crash_log_watchdog


@pytest.fixture(scope="function",
                params=[HOST_OS_PLATFORM])
def asset_processor_platform(request):
    """
    Platform to target for the AssetProcessor for modifying AssetProcessorPlatformConfig.ini
    :param request: _pytest.fixtures.SubRequest request object.
    :return: string representing the AssetProcessor platform to use.
    """
    return _asset_processor_platform(request)


def _asset_processor_platform(request):
    """Separate implementation to call directly during unit tests"""
    ap_platform = request.param
    if ap_platform in ALL_PLATFORM_OPTIONS:
        logger.debug(f'Returning asset_processor_platform: "{ap_platform}".')
        return ap_platform
    else:
        raise ValueError(
            f'asset_processor_platform: "{ap_platform}" is not valid. '
            f'Please select from one of the following: {ALL_PLATFORM_OPTIONS}')


def monkeypatch_pytest():
    """
    Patches a file creation race condition in _pytest.junitxml.LogXML.pytest_sessionfinish
    """
    import types
    import _pytest.junitxml
    import platform
    import xml.etree.ElementTree as ET
    from _pytest import timing

    # patch code from pytest 7.2.0, fetched at commit 7431750
    def pytest_sessionfinish(self) -> None:
        dirname = os.path.dirname(os.path.abspath(self.logfile))
        if not os.path.isdir(dirname):
            os.makedirs(dirname, exist_ok=True)  # our patch

        with open(self.logfile, "w", encoding="utf-8") as logfile:
            suite_stop_time = timing.time()
            suite_time_delta = suite_stop_time - self.suite_start_time

            numtests = (
                    self.stats["passed"]
                    + self.stats["failure"]
                    + self.stats["skipped"]
                    + self.stats["error"]
                    - self.cnt_double_fail_tests
            )
            logfile.write('<?xml version="1.0" encoding="utf-8"?>')

            suite_node = ET.Element(
                "testsuite",
                name=self.suite_name,
                errors=str(self.stats["error"]),
                failures=str(self.stats["failure"]),
                skipped=str(self.stats["skipped"]),
                tests=str(numtests),
                time="%.3f" % suite_time_delta,
                timestamp=datetime.fromtimestamp(self.suite_start_time).isoformat(),
                hostname=platform.node(),
            )
            global_properties = self._get_global_properties_node()
            if global_properties is not None:
                suite_node.append(global_properties)
            for node_reporter in self.node_reporters_ordered:
                suite_node.append(node_reporter.to_xml())
            testsuites = ET.Element("testsuites")
            testsuites.append(suite_node)
            logfile.write(ET.tostring(testsuites, encoding="unicode"))

    # override
    _pytest.junitxml.LogXML.pytest_sessionfinish = pytest_sessionfinish
