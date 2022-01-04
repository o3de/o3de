"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os

import pytest

import editor_python_test_tools.hydra_test_utils as hydra
import ly_test_tools.environment.file_system as file_system
from ly_test_tools.benchmark.data_aggregator import BenchmarkDataAggregator
from ly_test_tools.o3de.editor_test import EditorSharedTest, EditorTestSuite
from Atom.atom_utils.atom_component_helper import compare_screenshot_to_golden_image, golden_images_directory

DEFAULT_SUBFOLDER_PATH = 'user/PythonTests/Automated/Screenshots'
logger = logging.getLogger(__name__)


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):
    # Remove -autotest_mode from global_extra_cmdline_args since we need rendering for these tests.
    global_extra_cmdline_args = ["-BatchMode"]  # Default is ["-BatchMode", "-autotest_mode"]

    enable_prefab_system = False

    @pytest.mark.test_case_id("C34603773")
    class AtomGPU_BasicLevelSetup_SetsUpLevel(EditorSharedTest):
        use_null_renderer = False  # Default is True
        screenshot_name = "AtomBasicLevelSetup.ppm"
        test_screenshots = []  # Gets set by setup()
        screenshot_directory = ""  # Gets set by setup()

        # Clear existing test screenshots before starting test.
        def setup(self, workspace):
            screenshot_directory = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH)
            test_screenshots = [os.path.join(screenshot_directory, self.screenshot_name)]
            file_system.delete(test_screenshots, True, True)

        golden_images = [os.path.join(golden_images_directory(), screenshot_name)]

        from Atom.tests import hydra_AtomGPU_BasicLevelSetup as test_module

        assert compare_screenshot_to_golden_image(screenshot_directory, test_screenshots, golden_images, 0.99) is True

    @pytest.mark.test_case_id("C34525095")
    class AtomGPU_LightComponent_AreaLightScreenshotsMatchGoldenImages(EditorSharedTest):
        use_null_renderer = False  # Default is True
        screenshot_names = [
            "AreaLight_1.ppm",
            "AreaLight_2.ppm",
            "AreaLight_3.ppm",
            "AreaLight_4.ppm",
            "AreaLight_5.ppm",
        ]
        test_screenshots = []  # Gets set by setup()
        screenshot_directory = ""  # Gets set by setup()

        # Clear existing test screenshots before starting test.
        def setup(self, workspace):
            screenshot_directory = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH)
            for screenshot in self.screenshot_names:
                screenshot_path = os.path.join(screenshot_directory, screenshot)
                self.test_screenshots.append(screenshot_path)
            file_system.delete(self.test_screenshots, True, True)

        golden_images = []
        for golden_image in screenshot_names:
            golden_image_path = os.path.join(golden_images_directory(), golden_image)
            golden_images.append(golden_image_path)

        from Atom.tests import hydra_AtomGPU_AreaLightScreenshotTest as test_module

        assert compare_screenshot_to_golden_image(screenshot_directory, test_screenshots, golden_images, 0.99) is True

    @pytest.mark.test_case_id("C34525110")
    class AtomGPU_LightComponent_SpotLightScreenshotsMatchGoldenImages(EditorSharedTest):
        use_null_renderer = False  # Default is True
        screenshot_names = [
            "SpotLight_1.ppm",
            "SpotLight_2.ppm",
            "SpotLight_3.ppm",
            "SpotLight_4.ppm",
            "SpotLight_5.ppm",
            "SpotLight_6.ppm",
        ]
        test_screenshots = []  # Gets set by setup()
        screenshot_directory = ""  # Gets set by setup()

        # Clear existing test screenshots before starting test.
        def setup(self, workspace):
            screenshot_directory = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH)
            for screenshot in self.screenshot_names:
                screenshot_path = os.path.join(screenshot_directory, screenshot)
                self.test_screenshots.append(screenshot_path)
            file_system.delete(self.test_screenshots, True, True)

        golden_images = []
        for golden_image in screenshot_names:
            golden_image_path = os.path.join(golden_images_directory(), golden_image)
            golden_images.append(golden_image_path)

        from Atom.tests import hydra_AtomGPU_SpotLightScreenshotTest as test_module

        assert compare_screenshot_to_golden_image(screenshot_directory, test_screenshots, golden_images, 0.99) is True


@pytest.mark.parametrize('rhi', ['dx12', 'vulkan'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ["windows_editor"])
@pytest.mark.parametrize("level", ["AtomFeatureIntegrationBenchmark"])
class TestPerformanceBenchmarkSuite(object):
    def test_AtomFeatureIntegrationBenchmark(
            self, request, editor, workspace, rhi, project, launcher_platform, level):
        """
        Please review the hydra script run by this test for more specific test info.
        Tests the performance of the Simple level.
        """
        expected_lines = [
            "Benchmark metadata captured.",
            "Pass timestamps captured.",
            "CPU frame time captured.",
            "Captured data successfully.",
            "Exited game mode"
        ]

        unexpected_lines = [
            "Failed to capture data.",
            "Failed to capture pass timestamps.",
            "Failed to capture CPU frame time.",
            "Failed to capture benchmark metadata."
        ]

        hydra.launch_and_validate_results(
            request,
            os.path.join(os.path.dirname(__file__), "tests"),
            editor,
            "hydra_GPUTest_AtomFeatureIntegrationBenchmark.py",
            timeout=600,
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            cfg_args=[level],
            null_renderer=False,
            enable_prefab_system=False,
        )

        aggregator = BenchmarkDataAggregator(workspace, logger, 'periodic')
        aggregator.upload_metrics(rhi)


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_generic'])
class TestMaterialEditor(object):

    @pytest.mark.parametrize("cfg_args,expected_lines", [
        pytest.param("-rhi=dx12", ["Registering dx12 RHI"]),
        pytest.param("-rhi=Vulkan", ["Registering vulkan RHI"])
    ])
    @pytest.mark.parametrize("exe_file_name", ["MaterialEditor"])
    @pytest.mark.test_case_id("C30973986")  # Material Editor Launching in Dx12
    @pytest.mark.test_case_id("C30973987")  # Material Editor Launching in Vulkan
    def test_MaterialEditorLaunch_AllRHIOptionsSucceed(
            self, request, workspace, project, launcher_platform, generic_launcher, exe_file_name, cfg_args,
            expected_lines):
        """
        Tests each valid RHI option (Null RHI excluded) can be launched with the MaterialEditor.
        Checks for the specific expected_lines messaging for each RHI type.
        """

        hydra.launch_and_validate_results(
            request,
            os.path.join(os.path.dirname(__file__), "tests"),
            generic_launcher,
            editor_script="",
            run_python="--runpython",
            timeout=60,
            expected_lines=expected_lines,
            unexpected_lines=[],
            halt_on_unexpected=False,
            null_renderer=False,
            cfg_args=[cfg_args],
            log_file_name="MaterialEditor.log",
            enable_prefab_system=False,
        )
