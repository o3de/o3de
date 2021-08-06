"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Tests that require a GPU in order to run.
"""

import logging
import os

import pytest

import ly_test_tools.environment.file_system as file_system
from ly_test_tools.image.screenshot_compare_qssim import qssim as compare_screenshots
from ly_test_tools.benchmark.data_aggregator import BenchmarkDataAggregator

import editor_python_test_tools.hydra_test_utils as hydra

logger = logging.getLogger(__name__)
DEFAULT_SUBFOLDER_PATH = 'user/PythonTests/Automated/Screenshots'
TEST_DIRECTORY = os.path.join(os.path.dirname(__file__), "atom_hydra_scripts")


def golden_images_directory():
    """
    Uses this file location to return the valid location for golden image files.
    :return: The path to the golden_images directory, but raises an IOError if the golden_images directory is missing.
    """
    current_file_directory = os.path.join(os.path.dirname(__file__))
    golden_images_dir = os.path.join(current_file_directory, 'golden_images')

    if not os.path.exists(golden_images_dir):
        raise IOError(
            f'golden_images" directory was not found at path "{golden_images_dir}"'
            f'Please add a "golden_images" directory inside: "{current_file_directory}"'
        )

    return golden_images_dir


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ["windows_editor"])
@pytest.mark.parametrize("level", ["auto_test"])
class TestAllComponentsIndepthTests(object):

    @pytest.mark.parametrize("screenshot_name", ["AtomBasicLevelSetup.ppm"])
    def test_BasicLevelSetup_SetsUpLevel(
            self, request, editor, workspace, project, launcher_platform, level, screenshot_name):
        """
        Please review the hydra script run by this test for more specific test info.
        Tests that a basic rendering level setup can be created (lighting, meshes, materials, etc.).
        """
        # Clear existing test screenshots before starting test.
        test_screenshots = [os.path.join(
            workspace.paths.project(), DEFAULT_SUBFOLDER_PATH, screenshot_name)]
        file_system.delete(test_screenshots, True, True)

        golden_images = [os.path.join(golden_images_directory(), screenshot_name)]

        level_creation_expected_lines = [
            "Viewport is set to the expected size: True",
            "Basic level created"
        ]
        unexpected_lines = [
            "Trace::Assert",
            "Trace::Error",
            "Traceback (most recent call last):",
            "Screenshot failed"
        ]

        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "hydra_GPUTest_BasicLevelSetup.py",
            timeout=180,
            expected_lines=level_creation_expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            cfg_args=[level],
            null_renderer=False,
        )

        for test_screenshot, golden_screenshot in zip(test_screenshots, golden_images):
            compare_screenshots(test_screenshot, golden_screenshot)

    def test_LightComponent_ScreenshotMatchesGoldenImage(
            self, request, editor, workspace, project, launcher_platform, level):
        """
        Please review the hydra script run by this test for more specific test info.
        Tests that the Light component screenshots in a rendered level appear the same as the golden images.
        """
        screenshot_names = [
            "AreaLight_1.ppm",
            "AreaLight_2.ppm",
            "AreaLight_3.ppm",
            "AreaLight_4.ppm",
            "AreaLight_5.ppm",
            "SpotLight_1.ppm",
            "SpotLight_2.ppm",
            "SpotLight_3.ppm",
            "SpotLight_4.ppm",
            "SpotLight_5.ppm",
            "SpotLight_6.ppm",
        ]
        test_screenshots = []
        for screenshot in screenshot_names:
            screenshot_path = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH, screenshot)
            test_screenshots.append(screenshot_path)
        file_system.delete(test_screenshots, True, True)

        golden_images = []
        for golden_image in screenshot_names:
            golden_image_path = os.path.join(golden_images_directory(), golden_image)
            golden_images.append(golden_image_path)

        expected_lines = ["Light component tests completed."]
        unexpected_lines = [
            "Trace::Assert",
            "Trace::Error",
            "Traceback (most recent call last):",
            "Screenshot failed",
        ]
        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "hydra_GPUTest_LightComponent.py",
            timeout=180,
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            cfg_args=[level],
            null_renderer=False,
        )

        for test_screenshot, golden_screenshot in zip(test_screenshots, golden_images):
            compare_screenshots(test_screenshot, golden_screenshot)


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
            "Capturing complete.",
            "Captured data successfully."
        ]

        unexpected_lines = [
            "Failed to capture data.",
            "Failed to capture pass timestamps.",
            "Failed to capture benchmark metadata."
        ]

        hydra.launch_and_validate_results(
            request,
            TEST_DIRECTORY,
            editor,
            "hydra_GPUTest_AtomFeatureIntegrationBenchmark.py",
            timeout=600,
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            cfg_args=[level],
            null_renderer=False,
        )

        aggregator = BenchmarkDataAggregator(workspace, logger, 'periodic')
        aggregator.upload_metrics(rhi)
