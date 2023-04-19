"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os
import psutil
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
    "screenshot compare error. Error"
]

atom_feature_test_list = [
    pytest.param(
        "Atom_DepthOfField", 
        "depth_of_field_screenshot1.png", 
        "@gemroot:ScriptAutomation@/Assets/AutomationScripts/GenericRenderScreenshotTest.lua", 
        "levels/atomtests/feature/depthoffield/depthoffield.spawnable", "Level D"
    )
]

WINDOWS = sys.platform.startswith('win')

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
        game_launcher = launcher_helper.create_game_launcher(workspace)
        game_launcher.args.extend([ f'--rhi=dx12 ',
                                    f'--run-automation-suite={test_script} ',
                                    '--exit-on-automation-end ',
                                    f'--regset="/O3DE/ScriptAutomation/ImageCapture/LevelPath={level_path}" ',
                                    f'--regset="/O3DE/ScriptAutomation/ImageCapture/TestGroupName={test_name}" ',
                                    f'--regset="/O3DE/ScriptAutomation/ImageCapture/ImageName={screenshot_name}" ',
                                    f'--regset="/O3DE/ScriptAutomation/ImageCapture/ImageComparisonLevel={compare_tolerance}" '])
        game_launcher.start()
        waiter.wait_for(lambda: process_utils.process_exists(f"AutomatedTesting.GameLauncher.exe", ignore_extensions=True))

        # Verify that the GameLauncher.exe was able to connect to the ServerLauncher.exe by checking the logs
        game_launcher_log_file = os.path.join(game_launcher.workspace.paths.project_log(), 'Game.log')
        game_launcher_log_monitor = LogMonitor(game_launcher, game_launcher_log_file)
        game_launcher_log_monitor.monitor_log_for_lines(expected_lines, unexpected_lines, halt_on_unexpected=True, timeout=400)
        process_utils.kill_processes_named("AssetProcessor.exe", ignore_extensions=True)

@pytest.mark.SUITE_periodic
@pytest.mark.REQUIRES_gpu
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ["windows, linux"])
class TestPeriodicSuite_Vulkan_GPU(object):

    @pytest.mark.parametrize("test_name, screenshot_name, test_script, level_path, compare_tolerance", atom_feature_test_list)
    def test_Atom_FeatureTests_Vulkan(
            self, workspace, launcher_platform, test_name, screenshot_name, test_script, level_path, compare_tolerance):
        """
        Run Atom on Vulkan and screen capture tests on parameterised levels
        """
        game_launcher = launcher_helper.create_game_launcher(workspace)
        game_launcher.args.extend([ f'--rhi=vulkan ',
                                    f'--run-automation-suite={test_script} ',
                                    '--exit-on-automation-end ',
                                    f'--regset="/O3DE/ScriptAutomation/ImageCapture/LevelPath={level_path}" ',
                                    f'--regset="/O3DE/ScriptAutomation/ImageCapture/TestGroupName={test_name}" ',
                                    f'--regset="/O3DE/ScriptAutomation/ImageCapture/ImageName={screenshot_name}" ',
                                    f'--regset="/O3DE/ScriptAutomation/ImageCapture/ImageComparisonLevel={compare_tolerance}" '])
        game_launcher.start()
        waiter.wait_for(lambda: process_utils.process_exists(f"AutomatedTesting.GameLauncher.exe", ignore_extensions=True))

        # Verify that the GameLauncher.exe was able to connect to the ServerLauncher.exe by checking the logs
        game_launcher_log_file = os.path.join(game_launcher.workspace.paths.project_log(), 'Game.log')
        game_launcher_log_monitor = LogMonitor(game_launcher, game_launcher_log_file)
        game_launcher_log_monitor.monitor_log_for_lines(expected_lines, unexpected_lines, halt_on_unexpected=True, timeout=400)
        process_utils.kill_processes_named("AssetProcessor.exe", ignore_extensions=True)
