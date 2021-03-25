"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

These tests will validate that each project can be setup and built, have no failing assets processed, and will then
have a screenshot taken to verify that it renders normally. All the logs and screenshots will be transferred to the
test results to be zipped up and added to the Flume result. These projects will be run in profile and debug.
Currently SearchForEden and Bistro are failing and are temporarily commented out of these tests.
"""
import logging
import os
import pytest

from demos.test_lib.demos_testlib import load_level, remote_console_take_screenshot, start_launcher, start_remote_console
from test_tools.shared.file_utils import gather_error_logs, move_file
from test_tools.shared.launcher_testlib import configure_setup, assert_build_success, assert_process_assets

from test_tools import WINDOWS_LAUNCHER
import test_tools.builtin.fixtures as fixtures
import test_tools.launchers.phase
from test_tools.shared.remote_console_commands import RemoteConsole

logger = logging.getLogger(__name__)

# use_fixture registers the imported fixture in pytest at the specified scope. The test should provide all the
# parameters in the fixture's signature
workspace = fixtures.use_fixture(fixtures.builtin_workspace_fixture, scope='function')


@pytest.fixture
def launcher_instance(request, workspace, level):
    """
    Creates a launcher fixture instance with an extra teardown for error log grabbing.
    """
    def teardown():
        """
        Tries to grab any error logs before moving on to the next test.
        """
        if os.path.exists(launcher.workspace.release.paths.project_log()):
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

    request.addfinalizer(teardown)

    launcher = fixtures.launcher(request, workspace, level)
    return launcher


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


@pytest.mark.parametrize("platform,configuration,project,spec,level", [
        pytest.param("win_x64_vs2017", "profile", "StarterGame", "all", "StarterGame",
                     marks=pytest.mark.skipif(not WINDOWS_LAUNCHER, reason="Only supported on Windows hosts")),
        pytest.param("win_x64_vs2019", "profile", "StarterGame", "all", "StarterGame",
                     marks=pytest.mark.skipif(not WINDOWS_LAUNCHER, reason="Only supported on Windows hosts")),
        pytest.param("win_x64_vs2017", "debug", "StarterGame", "all", "StarterGame",
                     marks=pytest.mark.skipif(not WINDOWS_LAUNCHER, reason="Only supported on Windows hosts")),
        pytest.param("win_x64_vs2019", "debug", "StarterGame", "all", "StarterGame",
                     marks=pytest.mark.skipif(not WINDOWS_LAUNCHER, reason="Only supported on Windows hosts")),
    ])
class TestSingleLevel(object):
    def test_single_level(self, launcher_instance, level, remote_console_instance):
        """
        Verifies projects with a given demo-level can compile and successfully launch.
        """
        configure_setup(launcher_instance)

        assert_build_success(launcher_instance)
        assert_process_assets(launcher_instance)

        start_launcher(launcher_instance)
        start_remote_console(launcher_instance, remote_console_instance)

        load_level(launcher_instance, remote_console_instance, level)
        remote_console_take_screenshot(launcher_instance, remote_console_instance, level)


@pytest.mark.parametrize("platform,configuration,project,spec,level,levels", [
    pytest.param("win_x64_vs2017", "profile", "SamplesProject", "all", "Advanced_RinLocomotion",
                 ["Advanced_RinLocomotion", "Audio_Sample", "Fur_Technical_Sample", "Gems_InAppPurchases_Sample",
                  "Metastream_Sample", "ScriptCanvas_Basic_Sample", "Simple_JackLocomotion",
                  "SampleFullscreenAnimation", "UiDrawCallsSample", "UiFeatures", "UiIn3DWorld", "UiMainMenuLuaSample",
                  "UiMainMenuScriptCanvasSample", "UiTextureAtlasSample"],
                 marks=pytest.mark.skipif(not WINDOWS_LAUNCHER, reason="Only supported on Windows hosts")),
    pytest.param("win_x64_vs2019", "profile", "SamplesProject", "all", "Advanced_RinLocomotion",
                 ["Advanced_RinLocomotion", "Audio_Sample", "Fur_Technical_Sample", "Gems_InAppPurchases_Sample",
                  "Metastream_Sample", "ScriptCanvas_Basic_Sample", "Simple_JackLocomotion",
                  "SampleFullscreenAnimation", "UiDrawCallsSample", "UiFeatures", "UiIn3DWorld", "UiMainMenuLuaSample",
                  "UiMainMenuScriptCanvasSample", "UiTextureAtlasSample"],
                 marks=pytest.mark.skipif(not WINDOWS_LAUNCHER, reason="Only supported on Windows hosts")),
    pytest.param("win_x64_vs2017", "debug", "SamplesProject", "all", "Advanced_RinLocomotion",
                 ["Advanced_RinLocomotion", "Audio_Sample", "Fur_Technical_Sample", "Gems_InAppPurchases_Sample",
                  "Metastream_Sample", "ScriptCanvas_Basic_Sample", "Simple_JackLocomotion",
                  "SampleFullscreenAnimation", "UiDrawCallsSample", "UiFeatures", "UiIn3DWorld", "UiMainMenuLuaSample",
                  "UiMainMenuScriptCanvasSample", "UiTextureAtlasSample"],
                 marks=pytest.mark.skipif(not WINDOWS_LAUNCHER, reason="Only supported on Windows hosts")),
    pytest.param("win_x64_vs2019", "debug", "SamplesProject", "all", "Advanced_RinLocomotion",
                 ["Advanced_RinLocomotion", "Audio_Sample", "Fur_Technical_Sample", "Gems_InAppPurchases_Sample",
                  "Metastream_Sample", "ScriptCanvas_Basic_Sample", "Simple_JackLocomotion",
                  "SampleFullscreenAnimation", "UiDrawCallsSample", "UiFeatures", "UiIn3DWorld", "UiMainMenuLuaSample",
                  "UiMainMenuScriptCanvasSample", "UiTextureAtlasSample"],
                 marks=pytest.mark.skipif(not WINDOWS_LAUNCHER, reason="Only supported on Windows hosts"))
    ])
class TestMultipleLevels(object):
    def test_multiple_levels(self, launcher_instance, levels, remote_console_instance):
        """
        Verifies projects with multiple demo-levels can compile and successfully launch.
        """
        configure_setup(launcher_instance)

        assert_build_success(launcher_instance)
        assert_process_assets(launcher_instance)

        start_launcher(launcher_instance)
        start_remote_console(launcher_instance, remote_console_instance)

        for level in levels:
            load_level(launcher_instance, remote_console_instance, level)
            remote_console_take_screenshot(launcher_instance, remote_console_instance, level)
