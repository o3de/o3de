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
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorTestSuite
from Atom.atom_utils.atom_component_helper import compare_screenshot_to_golden_image, golden_images_directory

DEFAULT_SUBFOLDER_PATH = 'user/PythonTests/Automated/Screenshots'
logger = logging.getLogger(__name__)


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):
    # Remove -BatchMode from global_extra_cmdline_args since we need rendering for these tests.
    global_extra_cmdline_args = ["-autotest_mode"]  # Default is ["-BatchMode", "-autotest_mode"]
    use_null_renderer = False  # Default is True

    @staticmethod
    def screenshot_setup(screenshot_directory, screenshot_names):
        """
        :param screenshot_names: list of screenshot file names with extensions
        :return: tuple test_screenshots, golden_images each a list of full file paths
        """
        test_screenshots = []
        golden_images = []
        for screenshot in screenshot_names:
            screenshot_path = os.path.join(screenshot_directory, screenshot)
            test_screenshots.append(screenshot_path)
        file_system.delete(test_screenshots, True, True)
        for golden_image in screenshot_names:
            golden_image_path = os.path.join(golden_images_directory(), golden_image)
            golden_images.append(golden_image_path)
        return test_screenshots, golden_images

    @pytest.mark.test_case_id("C34525095")
    class AtomGPU_LightComponent_AreaLightScreenshotsMatchGoldenImages_DX12(EditorSingleTest):
        from Atom.tests import hydra_AtomGPU_AreaLightScreenshotTest as test_module

        extra_cmdline_args = ["-rhi=dx12"]

        # Custom setup/teardown to remove old screenshots and establish paths to golden images
        def setup(self, request, workspace, editor, editor_test_results, launcher_platform):
            self.screenshot_directory = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH)
            self.screenshot_names = [
                "AreaLight_1.ppm",
                "AreaLight_2.ppm",
                "AreaLight_3.ppm",
                "AreaLight_4.ppm",
                "AreaLight_5.ppm",
            ]
            self.test_screenshots, self.golden_images = TestAutomation.screenshot_setup(
                screenshot_directory=self.screenshot_directory,
                screenshot_names=self.screenshot_names)

        def wrap_run(self, request, workspace, editor, editor_test_results, launcher_platform):
            yield
            assert compare_screenshot_to_golden_image(self.screenshot_directory,
                                                      self.test_screenshots,
                                                      self.golden_images,
                                                      similarity_threshold=0.96) is True

    @pytest.mark.test_case_id("C34525095")
    class AtomGPU_LightComponent_AreaLightScreenshotsMatchGoldenImages_Vulkan(EditorSingleTest):
        from Atom.tests import hydra_AtomGPU_AreaLightScreenshotTest as test_module

        extra_cmdline_args = ["-rhi=vulkan"]

        # Custom setup/teardown to remove old screenshots and establish paths to golden images
        def setup(self, request, workspace, editor, editor_test_results, launcher_platform):
            self.screenshot_directory = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH)
            self.screenshot_names = [
                "AreaLight_1.ppm",
                "AreaLight_2.ppm",
                "AreaLight_3.ppm",
                "AreaLight_4.ppm",
                "AreaLight_5.ppm",
            ]
            self.test_screenshots, self.golden_images = TestAutomation.screenshot_setup(
                screenshot_directory=self.screenshot_directory,
                screenshot_names=self.screenshot_names)

        def wrap_run(self, request, workspace, editor, editor_test_results, launcher_platform):
            yield
            assert compare_screenshot_to_golden_image(self.screenshot_directory,
                                                      self.test_screenshots,
                                                      self.golden_images,
                                                      similarity_threshold=0.96) is True

    @pytest.mark.test_case_id("C34525110")
    class AtomGPU_LightComponent_SpotLightScreenshotsMatchGoldenImages_DX12(EditorSingleTest):
        from Atom.tests import hydra_AtomGPU_SpotLightScreenshotTest as test_module

        extra_cmdline_args = ["-rhi=dx12"]

        # Custom setup/teardown to remove old screenshots and establish paths to golden images
        def setup(self, request, workspace, editor, editor_test_results, launcher_platform):
            self.screenshot_directory = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH)
            self.screenshot_names = [
                "SpotLight_1.ppm",
                "SpotLight_2.ppm",
                "SpotLight_3.ppm",
                "SpotLight_4.ppm",
                "SpotLight_5.ppm",
                "SpotLight_6.ppm",
            ]
            self.test_screenshots, self.golden_images = TestAutomation.screenshot_setup(
                screenshot_directory=self.screenshot_directory,
                screenshot_names=self.screenshot_names)

        def wrap_run(self, request, workspace, editor, editor_test_results, launcher_platform):
            yield
            assert compare_screenshot_to_golden_image(self.screenshot_directory,
                                                      self.test_screenshots,
                                                      self.golden_images,
                                                      similarity_threshold=0.96) is True

    @pytest.mark.test_case_id("C34525110")
    class AtomGPU_LightComponent_SpotLightScreenshotsMatchGoldenImages_Vulkan(EditorSingleTest):
        from Atom.tests import hydra_AtomGPU_SpotLightScreenshotTest as test_module

        extra_cmdline_args = ["-rhi=vulkan"]

        # Custom setup/teardown to remove old screenshots and establish paths to golden images
        def setup(self, request, workspace, editor, editor_test_results, launcher_platform):
            self.screenshot_directory = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH)
            self.screenshot_names = [
                "SpotLight_1.ppm",
                "SpotLight_2.ppm",
                "SpotLight_3.ppm",
                "SpotLight_4.ppm",
                "SpotLight_5.ppm",
                "SpotLight_6.ppm",
            ]
            self.test_screenshots, self.golden_images = TestAutomation.screenshot_setup(
                screenshot_directory=self.screenshot_directory,
                screenshot_names=self.screenshot_names)

        def wrap_run(self, request, workspace, editor, editor_test_results, launcher_platform):
            yield
            assert compare_screenshot_to_golden_image(self.screenshot_directory,
                                                      self.test_screenshots,
                                                      self.golden_images,
                                                      similarity_threshold=0.96) is True


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
        )
