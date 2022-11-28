"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os

import pytest

import ly_test_tools.environment.file_system as file_system

from ly_test_tools.o3de.atom_tools_test import AtomToolsTestSuite, AtomToolsSingleTest
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorTestSuite, EditorBatchedTest

from Atom.atom_utils.atom_component_helper import create_screenshots_archive, golden_images_directory

DEFAULT_SUBFOLDER_PATH = 'user/PythonTests/Automated/Screenshots'
logger = logging.getLogger(__name__)


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):
    # Remove -BatchMode from global_extra_cmdline_args since we need rendering for these tests.
    global_extra_cmdline_args = []  # Default is ["-BatchMode", "-autotest_mode"]
    use_null_renderer = False  # Default is True

    @pytest.mark.test_case_id("C34525095")
    @pytest.mark.skip(reason="These are old GPU tests that are going to be changed in a future update.")
    class AtomGPU_LightComponent_AreaLightScreenshotsMatchGoldenImages_DX12(EditorSingleTest):
        from Atom.tests import hydra_AtomGPU_AreaLightScreenshotTest as test_module

        extra_cmdline_args = ["-rhi=dx12"]

        # Custom setup/teardown to remove old screenshots and establish paths to golden images
        def setup(self, request, workspace):
            self.screenshot_directory = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH)

        def wrap_run(self, request, workspace, editor_test_results):
            yield
            create_screenshots_archive(self.screenshot_directory)

    @pytest.mark.test_case_id("C34525095")
    @pytest.mark.skip(reason="These are old GPU tests that are going to be changed in a future update.")
    class AtomGPU_LightComponent_AreaLightScreenshotsMatchGoldenImages_Vulkan(EditorSingleTest):
        from Atom.tests import hydra_AtomGPU_AreaLightScreenshotTest as test_module

        extra_cmdline_args = ["-rhi=vulkan"]

        # Custom setup/teardown to remove old screenshots and establish paths to golden images
        def setup(self, request, workspace):
            self.screenshot_directory = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH)

        def wrap_run(self, request, workspace, editor_test_results):
            yield
            create_screenshots_archive(self.screenshot_directory)

    @pytest.mark.test_case_id("C34525110")
    @pytest.mark.skip(reason="These are old GPU tests that are going to be changed in a future update.")
    class AtomGPU_LightComponent_SpotLightScreenshotsMatchGoldenImages_DX12(EditorSingleTest):
        from Atom.tests import hydra_AtomGPU_SpotLightScreenshotTest as test_module

        extra_cmdline_args = ["-rhi=dx12"]

        # Custom setup/teardown to remove old screenshots and establish paths to golden images
        def setup(self, request, workspace):
            self.screenshot_directory = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH)

        def wrap_run(self, request, workspace, editor_test_results):
            yield
            create_screenshots_archive(self.screenshot_directory)

    @pytest.mark.test_case_id("C34525110")
    @pytest.mark.skip(reason="These are old GPU tests that are going to be changed in a future update.")
    class AtomGPU_LightComponent_SpotLightScreenshotsMatchGoldenImages_Vulkan(EditorSingleTest):
        from Atom.tests import hydra_AtomGPU_SpotLightScreenshotTest as test_module

        extra_cmdline_args = ["-rhi=vulkan"]

        # Custom setup/teardown to remove old screenshots and establish paths to golden images
        def setup(self, request, workspace):
            self.screenshot_directory = os.path.join(workspace.paths.project(), DEFAULT_SUBFOLDER_PATH)

        def wrap_run(self, request, workspace, editor_test_results):
            yield
            create_screenshots_archive(self.screenshot_directory)

    class AtomEditorComponents_HDRColorGrading_Generate_Activate_LUT(EditorBatchedTest):
        from Atom.tests import periodic_AtomEditorComponents_HDRColorGradingAdded as test_module


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_atom_tools'])
class TestMaterialEditor(AtomToolsTestSuite):

    # Remove -BatchMode from global_extra_cmdline_args since we need rendering for these tests.
    global_extra_cmdline_args = []
    use_null_renderer = False
    log_name = "material_editor_test.log"
    atom_tools_executable_name = "MaterialEditor"

    class MaterialEditor_Atom_LaunchMaterialEditorDX12(AtomToolsSingleTest):
        extra_cmdline_args = ["-rhi=dx12"]

        from Atom.tests import MaterialEditor_Atom_LaunchMaterialEditor as test_module

    class MaterialEditor_Atom_LaunchMaterialEditorVulkan(AtomToolsSingleTest):
        extra_cmdline_args = ["-rhi=Vulkan"]

        from Atom.tests import MaterialEditor_Atom_LaunchMaterialEditor as test_module


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_atom_tools'])
class TestMaterialCanvas(AtomToolsTestSuite):

    # Remove -BatchMode from global_extra_cmdline_args since we need rendering for these tests.
    global_extra_cmdline_args = []
    use_null_renderer = False
    log_name = "material_canvas_test.log"
    atom_tools_executable_name = "MaterialCanvas"

    class MaterialCanvas_Atom_LaunchMaterialCanvasDX12(AtomToolsSingleTest):
        extra_cmdline_args = ["-rhi=dx12"]

        from Atom.tests import MaterialCanvas_Atom_LaunchMaterialCanvas as test_module

    class MaterialCanvas_Atom_LaunchMaterialCanvasVulkan(AtomToolsSingleTest):
        extra_cmdline_args = ["-rhi=Vulkan"]

        from Atom.tests import MaterialCanvas_Atom_LaunchMaterialCanvas as test_module
