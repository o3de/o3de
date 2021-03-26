"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

These tests will validate that StarterGame and SamplesProject can be setup and built, have no failing assets processed,
and will then have a screen shot taken to verify that it renders normally. All the logs and screenshots will be
transferred to the test results to be zipped up and added to the Flume result. These projects will be run in profile
and debug.
"""
import logging
import os
import pytest

from demos.test_lib.demos_testlib import load_level, remote_console_take_screenshot, start_launcher, start_remote_console
import shared.shader_compile_server_utils as compile_server
import test_tools.shared.file_utils as file_utils
from test_tools.shared.launcher_testlib import configure_setup, assert_build_success, assert_process_assets

import test_tools.builtin.fixtures as fixtures
from test_tools import MAC_LAUNCHER
import test_tools.launchers.phase
from test_tools.shared.remote_console_commands import RemoteConsole

logger = logging.getLogger(__name__)

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
        compile_server.stop_shader_compile_server()

        if os.path.exists(launcher.workspace.release.paths.project_log()):
            for file_name in os.listdir(launcher.workspace.release.paths.project_log()):
                file_utils.move_file(launcher.workspace.release.paths.project_log(),
                                     launcher.workspace.artifact_manager.get_save_artifact_path(),
                                     file_name)

        logs_exist = lambda: file_utils.gather_error_logs(
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
        pytest.param("darwin_x64", "profile", "StarterGame", "all", "StarterGame",
                     marks=pytest.mark.skipif(not MAC_LAUNCHER, reason="Only supported on Mac hosts")),
        pytest.param("darwin_x64", "debug", "StarterGame", "all", "StarterGame",
                     marks=pytest.mark.skipif(not MAC_LAUNCHER, reason="Only supported on Mac hosts")),
    ])
class TestSingleLevel(object):
    def test_single_level(self, launcher_instance, configuration, level, remote_console_instance):
        """
        Verifies projects with a given demo-level can compile and successfully launch.
        """
        configure_setup(launcher_instance)

        assert_build_success(launcher_instance)
        assert_process_assets(launcher_instance)

        compile_server.start_mac_shader_compile_server(os.path.join(launcher_instance.workspace.release.paths.dev(),
                                                                    "Tools"), configuration)
        start_launcher(launcher_instance)
        start_remote_console(launcher_instance, remote_console_instance)

        load_level(launcher_instance, remote_console_instance, level)
        remote_console_take_screenshot(launcher_instance, remote_console_instance, level)


@pytest.mark.parametrize("platform,configuration,project,spec,level,levels", [
    pytest.param("darwin_x64", "profile", "SamplesProject", "all", "Advanced_RinLocomotion",
                 ["Advanced_RinLocomotion", "Audio_Sample", "Fur_Technical_Sample",
                  "Gems_InAppPurchases_Sample", "Metastream_Sample", "ScriptCanvas_Basic_Sample",
                  "Simple_JackLocomotion", "SampleFullscreenAnimation", "UiFeatures", "UiIn3DWorld",
                  "UiMainMenuLuaSample", "UiMainMenuScriptCanvasSample"],
             marks=pytest.mark.skipif(not MAC_LAUNCHER, reason="Only supported on Mac hosts")),
    pytest.param("darwin_x64", "debug", "SamplesProject", "all", "Advanced_RinLocomotion",
                 ["Advanced_RinLocomotion", "Audio_Sample", "Fur_Technical_Sample",
                  "Gems_InAppPurchases_Sample", "Metastream_Sample", "ScriptCanvas_Basic_Sample",
                  "Simple_JackLocomotion", "SampleFullscreenAnimation", "UiFeatures", "UiIn3DWorld",
                  "UiMainMenuLuaSample", "UiMainMenuScriptCanvasSample"],
                 marks=pytest.mark.skipif(not MAC_LAUNCHER, reason="Only supported on Mac hosts")),
    ])
# Testing for projects with multiple demo levels
class TestMultipleLevels(object):
    """
    Verifies projects with multiple demo-levels can compile and successfully launch.
    """
    def test_multiple_levels(self, launcher_instance, configuration, levels, remote_console_instance):
        configure_setup(launcher_instance)

        assert_build_success(launcher_instance)
        assert_process_assets(launcher_instance)

        compile_server.start_mac_shader_compile_server(os.path.join(launcher_instance.workspace.release.paths.dev(),
                                                                    "Tools"), configuration)
        start_launcher(launcher_instance)
        start_remote_console(launcher_instance, remote_console_instance)

        # Switch to each level, check if it loads and takes a screen shot, then move to test folder for Flume
        for level in levels:
            load_level(launcher_instance, remote_console_instance, level)
            remote_console_take_screenshot(launcher_instance, remote_console_instance, level)
