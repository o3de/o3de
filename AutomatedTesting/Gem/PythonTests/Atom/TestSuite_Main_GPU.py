"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os

import pytest

import ly_test_tools.environment.file_system as file_system

from ly_test_tools.o3de.material_editor_test import MaterialEditorTestSuite, MaterialEditorSingleTest
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorTestSuite, EditorBatchedTest

from Atom.atom_utils.atom_component_helper import compare_screenshot_to_golden_image, golden_images_directory

DEFAULT_SUBFOLDER_PATH = 'user/PythonTests/Automated/Screenshots'
logger = logging.getLogger(__name__)


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):
    # Remove -BatchMode from global_extra_cmdline_args since we need rendering for these tests.
    global_extra_cmdline_args = []  # Default is ["-BatchMode", "-autotest_mode"]
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
    @pytest.mark.skip(reason="Remnant of AtomTest tests which are deprecated and currently failing, but keeping for reference.")
    class AtomGPU_LightComponent_AreaLightScreenshotsMatchGoldenImages_DX12(EditorSingleTest):
        from Atom.tests import hydra_AtomGPU_AreaLightScreenshotTest as test_module

        extra_cmdline_args = ["-rhi=dx12"]

        # Custom setup/teardown to remove old screenshots and establish paths to golden images
        def setup(self, request, workspace):
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

        def wrap_run(self, request, workspace, editor_test_results):
            yield
            assert compare_screenshot_to_golden_image(self.screenshot_directory,
                                                      self.test_screenshots,
                                                      self.golden_images,
                                                      similarity_threshold=0.96) is True

    @pytest.mark.test_case_id("C34525095")
    @pytest.mark.skip(reason="Remnant of AtomTest tests which are deprecated and currently failing, but keeping for reference.")
    class AtomGPU_LightComponent_AreaLightScreenshotsMatchGoldenImages_Vulkan(EditorSingleTest):
        from Atom.tests import hydra_AtomGPU_AreaLightScreenshotTest as test_module

        extra_cmdline_args = ["-rhi=vulkan"]

        # Custom setup/teardown to remove old screenshots and establish paths to golden images
        def setup(self, request, workspace):
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

        def wrap_run(self, request, workspace, editor_test_results):
            yield
            assert compare_screenshot_to_golden_image(self.screenshot_directory,
                                                      self.test_screenshots,
                                                      self.golden_images,
                                                      similarity_threshold=0.96) is True

    @pytest.mark.test_case_id("C34525110")
    @pytest.mark.skip(reason="Remnant of AtomTest tests which are deprecated and currently failing, but keeping for reference.")
    class AtomGPU_LightComponent_SpotLightScreenshotsMatchGoldenImages_DX12(EditorSingleTest):
        from Atom.tests import hydra_AtomGPU_SpotLightScreenshotTest as test_module

        extra_cmdline_args = ["-rhi=dx12"]

        # Custom setup/teardown to remove old screenshots and establish paths to golden images
        def setup(self, request, workspace):
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

        def wrap_run(self, request, workspace, editor_test_results):
            yield
            assert compare_screenshot_to_golden_image(self.screenshot_directory,
                                                      self.test_screenshots,
                                                      self.golden_images,
                                                      similarity_threshold=0.96) is True

    @pytest.mark.test_case_id("C34525110")
    @pytest.mark.skip(reason="Remnant of AtomTest tests which are deprecated and currently failing, but keeping for reference.")
    class AtomGPU_LightComponent_SpotLightScreenshotsMatchGoldenImages_Vulkan(EditorSingleTest):
        from Atom.tests import hydra_AtomGPU_SpotLightScreenshotTest as test_module

        extra_cmdline_args = ["-rhi=vulkan"]

        # Custom setup/teardown to remove old screenshots and establish paths to golden images
        def setup(self, request, workspace):
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

        def wrap_run(self, request, workspace, editor_test_results):
            yield
            assert compare_screenshot_to_golden_image(self.screenshot_directory,
                                                      self.test_screenshots,
                                                      self.golden_images,
                                                      similarity_threshold=0.96) is True

    class AtomEditorComponents_HDRColorGrading_Generate_Activate_LUT(EditorBatchedTest):
        from Atom.tests import periodic_AtomEditorComponents_HDRColorGradingAdded as test_module


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_material_editor'])
class TestMaterialEditor(MaterialEditorTestSuite):
    # Remove -BatchMode from global_extra_cmdline_args since we need rendering for these tests.
    global_extra_cmdline_args = []
    use_null_renderer = False

    class MaterialEditor_Atom_LaunchMaterialEditorDX12(MaterialEditorSingleTest):
        extra_cmdline_args = ["-rhi=dx12"]

        from Atom.tests import MaterialEditor_Atom_LaunchMaterialEditor as test_module

    class MaterialEditor_Atom_LaunchMaterialEditorVulkan(MaterialEditorSingleTest):
        extra_cmdline_args = ["-rhi=Vulkan"]

        from Atom.tests import MaterialEditor_Atom_LaunchMaterialEditor as test_module
