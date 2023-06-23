"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os
import sys
import tempfile
import json

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
        "Atom_DepthOfField", # test name
        "depth_of_field_screenshot1.png", # names to capture the screenshots to, also the names of the comparison images. Comma separated list
        "@gemroot:ScriptAutomation@/Assets/AutomationScripts/GenericRenderScreenshotTest.lua", # test control script
        "levels/atomscreenshottests/feature/depthoffield/depthoffield.spawnable", # level to load
        "Level D", # image comparison tolerance level. Comma separated list
        "Camera" # camera names for the screenshots. Comma separated list
    ),
    pytest.param(
        "Atom_AreaLights", # test name
        "sphere_screenshot.png,disk_screenshot.png", # names to capture the screenshots to, also the names of the comparison images. Comma separated list
        "@gemroot:ScriptAutomation@/Assets/AutomationScripts/GenericRenderScreenshotTest.lua", # test control script
        "levels/atomscreenshottests/feature/arealight/arealight.spawnable", # level to load
        "Level E,Level E", # image comparison tolerance level. Comma separated list
        "TestCameraPointLights,TestCameraDiskLights" # camera names for the screenshots. Comma separated list
    )
]

atom_render_pipeline_list = [
     pytest.param("passes/MainRenderPipeline.azasset"),
     pytest.param("passes/LowEndRenderPipeline.azasset")
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


def run_test(workspace, rhi, test_name, screenshot_name, test_script, level_path, compare_tolerance, camera_name, render_pipeline):
        # build SettingRegistry patch for the test
        run_settings= {
             "O3DE" : {
                  "ScriptAutomation" : {
                       "ImageCapture" : {
                            "LevelPath" : level_path,
                            "TestGroupName" : test_name,
                            "ImageName" : screenshot_name,
                            "ImageComparisonLevel" : compare_tolerance,
                            "CaptureCameraName" : camera_name
                       }
                  }
             }
        }
        run_json = json.dumps(run_settings, indent=4)

        # Generate a temp file
        setreg_file = tempfile.NamedTemporaryFile('w+t', delete=False, suffix='.setreg')

        # write the json to the settings registry patch file
        setreg_file.write(run_json)
        setreg_file.flush()
        setreg_file.close()

        # launch the test
        game_launcher = launcher_helper.create_game_launcher(workspace)
        game_launcher.args.extend([ f'--rhi={rhi} ',
                                    f'--run-automation-suite={test_script} ',
                                    '--exit-on-automation-end ',
                                    f'--r_renderPipelinePath={render_pipeline}',
                                    f'--regset-file={setreg_file.name}'])
        game_launcher.start()
        waiter.wait_for(lambda: process_utils.process_exists(launcher_name, ignore_extensions=True))

        game_launcher_log_file = os.path.join(game_launcher.workspace.paths.project_log(), 'Game.log')
        game_launcher_log_monitor = LogMonitor(game_launcher, game_launcher_log_file)
         # test may do multiple image capture & compare so don't halt on unexpected
        try:
            game_launcher_log_monitor.monitor_log_for_lines(expected_lines, unexpected_lines, halt_on_unexpected=False, timeout=400)
        finally:
            process_utils.kill_processes_named(ap_name, ignore_extensions=True)



@pytest.mark.SUITE_smoke
@pytest.mark.REQUIRES_gpu
@pytest.mark.skipif(not WINDOWS, reason="DX12 is only supported on windows")
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ["windows"])
@pytest.mark.parametrize("render_pipeline", atom_render_pipeline_list)
class TestPeriodicSuite_DX12_GPU(object):

    @pytest.mark.parametrize("test_name, screenshot_name, test_script, level_path, compare_tolerance, camera_name", atom_feature_test_list)
    def test_Atom_FeatureTests_DX12(
            self, workspace, launcher_platform, test_name, screenshot_name, test_script, level_path, compare_tolerance, camera_name, render_pipeline):
        """
        Run Atom on DX12 and screen capture tests on parameterised levels
        """
        run_test(workspace, "dx12", test_name, screenshot_name, test_script, level_path, compare_tolerance, camera_name, render_pipeline)

@pytest.mark.SUITE_smoke
@pytest.mark.REQUIRES_gpu
@pytest.mark.skipif(not WINDOWS and not LINUX, reason="Vulkan is only supported on windows, linux, & android")
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ["windows, linux"])
@pytest.mark.parametrize("render_pipeline", atom_render_pipeline_list)
class TestPeriodicSuite_Vulkan_GPU(object):

    @pytest.mark.parametrize("test_name, screenshot_name, test_script, level_path, compare_tolerance, camera_name", atom_feature_test_list)
    def test_Atom_FeatureTests_Vulkan(
            self, workspace, launcher_platform, test_name, screenshot_name, test_script, level_path, compare_tolerance, camera_name, render_pipeline):
        """
        Run Atom on Vulkan and screen capture tests on parameterised levels
        """
        run_test(workspace, "vulkan", test_name, screenshot_name, test_script, level_path, compare_tolerance, camera_name, render_pipeline)
