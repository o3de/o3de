"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Sample tests to demonstrate typical functionality of PythonTestTools and how to integrate into the BAT.
"""
# Workaround for tests which depend on old tools, before they are updated to ly_test_tools and Python 3
import pytest
pytest.importorskip('test_tools')

# System level imports
import os
import subprocess

# Basic PythonTestTools imports, in most cases you should always import these
import test_tools.builtin.fixtures as fixtures
from test_tools import HOST_PLATFORM, WINDOWS_LAUNCHER

# test_tools and shared are modules with a lot of useful utility functions already written, use them!
# Pick and choose the ones below that you need, don't just blindly copy/paste
import test_tools.launchers.phase
import test_tools.shared.process_utils as process_utils
from test_tools.shared.file_utils import gather_error_logs, clear_out_config_file, delete_screenshot_folder, move_file
from shared.network_utils import check_for_listening_port
from test_tools.shared.remote_console_commands import RemoteConsole
from test_tools.shared.waiter import wait_for


# This is where you should access lumberyard - building, asset processing, finding paths/logs, etc.  Do NOT change or
# remove this unless you know what you're doing.
# See the documentation for PythonTestTools for more information.
workspace = fixtures.use_fixture(fixtures.builtin_empty_workspace_fixture, scope='function')

# What are fixtures?
# Fixtures set up a testing process (or test) by running all necessary code to satisfy its preconditions.
# More reading here: https://docs.pytest.org/en/latest/fixture.html

# This is a shared instance of the remote console that be used across multiple tests.  It must have the @pytest.fixture!
# You should remove this if you aren't going to use the remote console.
@pytest.fixture
def remote_console_instance(request):
    """
    Creates a remote console instance to send console commands.
    """
    console = RemoteConsole()

    def teardown():
        try:
            console.stop()
        except:
            pass

    request.addfinalizer(teardown)

    return console


# This is a shared instance of the launcher that can be used across multiple tests within this file and any that it
# includes. It must have the @pytest.fixture for pytest to automatically pass it around!
# You should remove this if you aren't going to use the level-specific launchers.
@pytest.fixture
def launcher_instance(request, workspace, level):
    """
    Creates a launcher fixture instance with an extra teardown for error log grabbing.
    """
    def teardown_launcher_copy_logs():
        """
        Tries to grab any error logs before moving on to the next test.
        """

        for file_name in os.listdir(launcher.workspace.release.paths.project_log()):
            move_file(launcher.workspace.release.paths.project_log(),
                      launcher.workspace.artifact_manager.get_save_artifact_path(),
                      file_name)

        logs_exist = lambda: gather_error_logs(
            launcher.workspace.release.paths.dev(),
            launcher.workspace.artifact_manager.get_save_artifact_path())
        try:
            test_tools.shared.waiter.wait_for(logs_exist)
        except AssertionError:
            print("No error logs found. Completing test...")

    request.addfinalizer(teardown_launcher_copy_logs)

    launcher = fixtures.launcher(request, workspace, level)
    return launcher

# For the rest of the file, these are sample tests that you should remove entirely, or cannibalize them to help your
# own test-writing process.
class TestSamplesAPBatch:

    # This is a shared instance of test teardown that will be used across all tests in this class.
    # It must have the @pytest.fixture(autouse=True)!
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request):

        # This is the teardown function that will be run after *each* test finishes
        def teardown():
            pass

        # This is the setup section that will be run before *each* test starts
        request.addfinalizer(teardown)

    # mark.BAT adds this test to the BAT.
    # mark.test_case is used to link to your testrail id.
    # mark.parameterize allows you to run the same test multiple times but with different parameters, such as platform,
    # configuration, project, level, and more. See more on parameters here: https://docs.pytest.org/en/latest/parametrize.html
    @pytest.mark.BAT
    @pytest.mark.test_case(testrail_id='Foo')
    @pytest.mark.parametrize('platform,configuration,project,spec', (
            pytest.param('win_x64_vs2017', 'profile', 'StarterGame', 'all',
                         marks=pytest.mark.skipif(HOST_PLATFORM != 'win_x64', reason='Only supported on Windows hosts')),
            pytest.param('darwin_x64', 'profile', 'StarterGame', 'all',
                         marks=pytest.mark.skipif(HOST_PLATFORM != 'darwin_x64', reason='Only supported on Mac hosts')),
    ))
    def test_RunAPBatch_WorkspacePreconfigured_NoLeftoverProcessesExist(self, workspace):
        """
        Tests that the Asset Processor Batch and run and doesn't leave leftover processes.
        """
        # Your function docstrings (the above text) will be part of the test catalog!

        subprocess.check_call([os.path.join(workspace.release.paths.bin(), 'AssetProcessorBatch')])

        # This is how you should do timeouts
        wait_for(lambda: not process_utils.process_exists('AssetProcessorBatch', True), timeout=10)

        # Make sure to include an informative assert message to make debugging easier
        assert not process_utils.process_exists('rc', True), 'rc process still exists'
        assert not process_utils.process_exists('AssetBuilder', True), 'AssetBuilder process still exists'


# Notice that you can put the marks both on the class and on the individual methods (seen above).
@pytest.mark.BAT
@pytest.mark.parametrize("platform,configuration,project,spec,level", [
    pytest.param("win_x64_vs2017", "profile", "StarterGame", "all", "StarterGame",
                 marks=pytest.mark.skipif(not WINDOWS_LAUNCHER, reason="Only supported on Windows hosts")),
    pytest.param("win_x64_vs2019", "profile", "StarterGame", "all", "StarterGame",
                 marks=pytest.mark.skipif(not WINDOWS_LAUNCHER, reason="Only supported on Windows hosts"))
])
class TestSamplesRemoteConsole(object):

    # Notice here that both the launcher_instance and remote_console_instance fixtures are being reused from above
    def test_LaunchRemoteConsoleAndLauncher_CanLaunch(self, launcher_instance, platform, configuration, project, spec,
                                                      level, remote_console_instance):
        """
        Verifies launcher & remote console can successfully launch.  Notice that there are no asserts here, and that is
        because the called functions will raise exceptions if there is unexpected behavior.
        """
        launcher_instance.launch()

        launcher_instance.run(test_tools.launchers.phase.TimePhase(120, 120))

        test_tools.shared.waiter.wait_for(lambda: check_for_listening_port(4600), timeout=300,
                                          exc=AssertionError('Port 4600 not listening.'))

        remote_console_instance.start()
