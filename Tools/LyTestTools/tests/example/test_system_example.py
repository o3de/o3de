"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Example test using LyTestTools to test Lumberyard.
"""
# Python built-in dependencies.
import logging

# Third party dependencies.
import pytest

# ly_test_tools dependencies.
import ly_test_tools.log.log_monitor
import ly_remote_console.remote_console_commands as remote_console_commands

# Configuring the logging is done in ly_test_tools at the following location:
# ~/dev/Tools/LyTestTools/ly_test_tools/_internal/log/py_logging_util.py

# Use the following logging pattern to hook all test logging together:
logger = logging.getLogger(__name__)


@pytest.fixture
def remote_console(request):
    """
    Creates a RemoteConsole() class instance to send console commands to the
    Lumberyard client console.
    :param request: _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :return: ly_remote_console.remote_console_commands.RemoteConsole class instance
        representing the Lumberyard remote console executable.
    """
    # Initialize the RemoteConsole object to send commands to the Lumberyard client console.
    console = remote_console_commands.RemoteConsole()

    # Custom teardown method for this remote_console fixture.
    def teardown():
        console.stop()

    # Utilize request.addfinalizer() to add custom teardown() methods.
    request.addfinalizer(teardown)  # This pattern must be used in pytest version

    return console

# Shared parameters & fixtures for all test methods inside the TestSystemExample class.
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize('project', ['AutomatedTesting'])
class TestSystemExample(object):
    """
    Example test case class to hold a set of test case methods.

    The amount of tests run is based on the parametrization stacking made in each test method or class.
    For this test, we placed unique test values in test methods and shared test values in the test class.
    We also assume building has already been done, but the test should error if the build is mis-configured.
    """
    # This test method needs specific parameters not shared by all other tests in the class.
    # For targeting specific launchers, use the 'launcher_platform' pytest param like below:
    # @pytest.mark.parametrize("launcher_platform", ['android'])
    # If you want to target different AssetProcessor platforms, use asset_processor_platform:
    # @pytest.mark.parametrize("asset_processor_platform", ['android'])
    @pytest.mark.parametrize('level', ['simple_jacklocomotion'])
    @pytest.mark.parametrize('load_wait', [120])
    @pytest.mark.test_case_id('C16806863')
    def test_SystemTestExample_AllSupportedPlatforms_LaunchAutomatedTesting(
            # launcher_platform, asset_processor_platform,  # Re-add these here if you plan to use them.
            self, launcher, remote_console, level, load_wait):
        """
        Tests launching the AutomatedTesting then launches the Lumberyard client &
        loads the "simple_jacklocomotion" level using the remote console.
        Assumes the user already setup & built their machine for the test.
        """
        # Launch the Lumberyard client & remote console test case:
        with launcher.start():
            remote_console.start()
            launcher_load = remote_console.expect_log_line(
                match_string='========================== '
                             'Finished loading textures '
                             '============================',
                timeout=load_wait)

        # Assert loading was successful using remote console logs:
        assert launcher_load, (
            'Launcher failed to load Lumberyard client with the '
            f'"{level}" level - waited "{load_wait}" seconds.')

    # This test method only needs pytest.mark report values and shared test class parameters.
    @pytest.mark.parametrize('processes_to_kill', ['Editor.exe'])
    @pytest.mark.parametrize("launcher_platform", ['windows_editor'])
    @pytest.mark.test_case_id('C16806864')
    def test_SystemTestExample_AllSupportedPlatforms_LaunchEditor(self, editor, processes_to_kill, launcher_platform):
        """
        Tests launching the Lumberyard Editor is successful with the current build.
        """
        # Launch the Lumberyard editor & verify load is successful:
        with editor.start():
            assert editor.is_alive(), (
                'Editor failed to launch for the current Lumberyard build.')

    # Log monitoring example test.
    @pytest.mark.parametrize('level', ['simple_jacklocomotion'])
    @pytest.mark.parametrize('expected_lines', [['Log Monitoring test 1', 'Log Monitoring test 2']])
    @pytest.mark.parametrize('unexpected_lines', [['Unexpected test 1', 'Unexpected test 2']])
    @pytest.mark.test_case_id('C21202585')
    def test_SystemTestExample_AllSupportedPlatforms_LogMonitoring(self, level, launcher, expected_lines,
                                                                   unexpected_lines):
        """
        Tests that the logging paths created by LyTestTools can be monitored for results using the log monitor.
        """
        # Launch the Lumberyard client & initialize the log monitor.
        file_to_monitor = launcher.workspace.info_log_path
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher,
                                                               log_file_path=file_to_monitor)

        # Generate log lines to the info log using logger.
        for expected_line in expected_lines:
            logger.info(expected_line)

        # Start the Lumberyard client & test that the lines we logged can be viewed by the log monitor.
        with launcher.start():
            log_test = log_monitor.monitor_log_for_lines(
                expected_lines=expected_lines,  # Defaults to None.
                unexpected_lines=unexpected_lines,  # Defaults to None.
                halt_on_unexpected=True,  # Defaults to False.
                timeout=60)  # Defaults to 30

            # Assert the log monitor detected expected lines and did not detect any unexpected lines.
            assert log_test, (
                f'Log monitoring failed. Used expected_lines values: {expected_lines} & '
                f'unexpected_lines values: {unexpected_lines}')
