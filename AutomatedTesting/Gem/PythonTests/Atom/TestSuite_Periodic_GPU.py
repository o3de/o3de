"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os
import sys

import pytest

from ly_test_tools.environment import process_utils
from ly_test_tools.launchers import launcher_helper
from ly_test_tools.log.log_monitor import LogMonitor

import ly_test_tools.environment.waiter as waiter

logger = logging.getLogger(__name__)

expected_lines = [
    "screenshot compare passed. Diff score is"
]

unexpected_lines = [
    "screenshot compare failed. Diff score",
    "screenshot compare error. Error",
    "Level not found",
    "Failed to load level"
]

atom_feature_test_list = [
    pytest.param(
        "Atom_DepthOfField", 
        "depth_of_field_screenshot1.png", 
        "@gemroot:ScriptAutomation@/Assets/AutomationScripts/GenericRenderScreenshotTest.lua", 
        "levels/atomscreenshottests/feature/depthoffield/depthoffield.spawnable", 
        "Level D"
    )
]

# default names are windows
launcher_name = "AutomatedTesting.GameLauncher.exe"
ap_name = "AssetProcessor.exe"
WINDOWS = sys.platform.startswith('win')
LINUX = sys.platform == "linux"
#MAC = sys.platform == "darwin"

if LINUX:
    launcher_name = "AutomatedTesting.GameLauncher"
    ap_name = "AssetProcessor"


def run_test(workspace, rhi, test_name, screenshot_name, test_script, level_path, compare_tolerance):
        game_launcher = launcher_helper.create_game_launcher(workspace)
        game_launcher.args.extend([ f'--rhi={rhi} ',
                                    f'--run-automation-suite={test_script} ',
                                    '--exit-on-automation-end ',
                                    f'--regset="/O3DE/ScriptAutomation/ImageCapture/LevelPath={level_path}" ',
                                    f'--regset="/O3DE/ScriptAutomation/ImageCapture/TestGroupName={test_name}" ',
                                    f'--regset="/O3DE/ScriptAutomation/ImageCapture/ImageName={screenshot_name}" ',
                                    f'--regset="/O3DE/ScriptAutomation/ImageCapture/ImageComparisonLevel={compare_tolerance}" '])
        game_launcher.start()
        waiter.wait_for(lambda: process_utils.process_exists(launcher_name, ignore_extensions=True))

        game_launcher_log_file = os.path.join(game_launcher.workspace.paths.project_log(), 'Game.log')
        game_launcher_log_monitor = LogMonitor(game_launcher, game_launcher_log_file)
         # test may do multiple image capture & compare so don't halt on unexpected
        try:
            game_launcher_log_monitor.monitor_log_for_lines(expected_lines, unexpected_lines, halt_on_unexpected=False, timeout=400)
        finally:
            process_utils.kill_processes_named(ap_name, ignore_extensions=True)



@pytest.mark.SUITE_periodic
@pytest.mark.REQUIRES_gpu
@pytest.mark.skipif(not WINDOWS, reason="DX12 is only supported on windows")
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ["windows"])
class TestPeriodicSuite_DX12_GPU(object):

    @pytest.mark.parametrize("test_name, screenshot_name, test_script, level_path, compare_tolerance", atom_feature_test_list)
    def test_Atom_FeatureTests_DX12(
            self, workspace, launcher_platform, test_name, screenshot_name, test_script, level_path, compare_tolerance):
        """
        Run Atom on DX12 and screen capture tests on parameterised levels
        """
        run_test(workspace, "dx12", test_name, screenshot_name, test_script, level_path, compare_tolerance)

@pytest.mark.SUITE_periodic
@pytest.mark.REQUIRES_gpu
@pytest.mark.skipif(not WINDOWS and not LINUX, reason="Vulkan is only supported on windows, linux, & android")
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ["windows, linux"])
class TestPeriodicSuite_Vulkan_GPU(object):

    @pytest.mark.parametrize("test_name, screenshot_name, test_script, level_path, compare_tolerance", atom_feature_test_list)
    def test_Atom_FeatureTests_Vulkan(
            self, workspace, launcher_platform, test_name, screenshot_name, test_script, level_path, compare_tolerance):
        """
        Run Atom on Vulkan and screen capture tests on parameterised levels
        """
        run_test(workspace, "vulkan", test_name, screenshot_name, test_script, level_path, compare_tolerance)
