"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import datetime
import logging
import os
import zipfile

import pytest

import ly_test_tools.environment.file_system as file_system
from ly_test_tools.benchmark.data_aggregator import BenchmarkDataAggregator

import editor_python_test_tools.hydra_test_utils as hydra
from .atom_utils.atom_component_helper import compare_screenshot_similarity, ImageComparisonTestFailure

logger = logging.getLogger(__name__)
DEFAULT_SUBFOLDER_PATH = 'user/PythonTests/Automated/Screenshots'
TEST_DIRECTORY = os.path.join(os.path.dirname(__file__), "tests")


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


def create_screenshots_archive(screenshot_path):
    """
    Creates a new zip file archive at archive_path containing all files listed within archive_path.
    :param screenshot_path: location containing the files to archive, the zip archive file will also be saved here.
    :return: None, but creates a new zip file archive inside path containing all of the files inside archive_path.
    """
    files_to_archive = []

    # Search for .png and .ppm files to add to the zip archive file.
    for (folder_name, sub_folders, file_names) in os.walk(screenshot_path):
        for file_name in file_names:
            if file_name.endswith(".png") or file_name.endswith(".ppm"):
                file_path = os.path.join(folder_name, file_name)
                files_to_archive.append(file_path)

    # Setup variables for naming the zip archive file.
    timestamp = datetime.datetime.now().timestamp()
    formatted_timestamp = datetime.datetime.utcfromtimestamp(timestamp).strftime("%Y-%m-%d_%H-%M-%S")
    screenshots_file = os.path.join(screenshot_path, f'zip_archive_{formatted_timestamp}.zip')

    # Write all of the valid .png and .ppm files to the archive file.
    with zipfile.ZipFile(screenshots_file, 'w', compression=zipfile.ZIP_DEFLATED, allowZip64=True) as zip_archive:
        for file_path in files_to_archive:
            file_name = os.path.basename(file_path)
            zip_archive.write(file_path, file_name)


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ["windows_editor"])
@pytest.mark.parametrize("level", ["Base"])
class TestAllComponentsIndepthTests(object):

    @pytest.mark.parametrize("screenshot_name", ["AtomBasicLevelSetup.ppm"])
    @pytest.mark.test_case_id("C34603773")
    def test_BasicLevelSetup_SetsUpLevel(
            self, request, editor, workspace, project, launcher_platform, level, screenshot_name):
        """
        Please review the hydra script run by this test for more specific test info.
        Tests that a basic rendering level setup can be created (lighting, meshes, materials, etc.).
        """
        # Clear existing test screenshots before starting test.
        screenshot_directory = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH)
        test_screenshots = [os.path.join(screenshot_directory, screenshot_name)]
        file_system.delete(test_screenshots, True, True)

        golden_images = [os.path.join(golden_images_directory(), screenshot_name)]

        level_creation_expected_lines = [
            "Viewport is set to the expected size: True",
            "Exited game mode"
        ]
        unexpected_lines = ["Traceback (most recent call last):"]

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
            enable_prefab_system=False,
        )

        similarity_threshold = 0.99
        for test_screenshot, golden_image in zip(test_screenshots, golden_images):
            screenshot_comparison_result = compare_screenshot_similarity(
                test_screenshot, golden_image, similarity_threshold, True, screenshot_directory)
            if screenshot_comparison_result != "Screenshots match":
                raise Exception(f"Screenshot test failed: {screenshot_comparison_result}")

    @pytest.mark.test_case_id("C34525095")
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
        screenshot_directory = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH)
        test_screenshots = []
        for screenshot in screenshot_names:
            screenshot_path = os.path.join(screenshot_directory, screenshot)
            test_screenshots.append(screenshot_path)
        file_system.delete(test_screenshots, True, True)

        golden_images = []
        for golden_image in screenshot_names:
            golden_image_path = os.path.join(golden_images_directory(), golden_image)
            golden_images.append(golden_image_path)

        expected_lines = ["spot_light Controller|Configuration|Shadows|Shadowmap size: SUCCESS"]
        unexpected_lines = ["Traceback (most recent call last):"]
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
            enable_prefab_system=False,
        )

        similarity_threshold = 0.99
        for test_screenshot, golden_image in zip(test_screenshots, golden_images):
            screenshot_comparison_result = compare_screenshot_similarity(
                test_screenshot, golden_image, similarity_threshold, True, screenshot_directory)
            if screenshot_comparison_result != "Screenshots match":
                raise ImageComparisonTestFailure(f"Screenshot test failed: {screenshot_comparison_result}")


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
            TEST_DIRECTORY,
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
            TEST_DIRECTORY,
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
