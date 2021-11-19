"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os

import pytest

import ly_test_tools.environment.file_system as file_system
from ly_test_tools.o3de.editor_test import EditorSharedTest, EditorTestSuite
from ly_test_tools.image.screenshot_compare_qssim import qssim as compare_screenshots
from .atom_utils.atom_component_helper import (
    create_screenshots_archive, compare_screenshot_to_golden_image, golden_images_directory)

DEFAULT_SUBFOLDER_PATH = 'user/PythonTests/Automated/Screenshots'


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):
    # Remove -autotest_mode from global_extra_cmdline_args since we need rendering for these tests.
    global_extra_cmdline_args = ["-BatchMode"]  # Default is ["-BatchMode", "-autotest_mode"]

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
    class AtomGPU_LightComponent_ScreenshotMatchesGoldenImage(EditorSharedTest):
        use_null_renderer = False  # Default is True
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

        from Atom.tests import hydra_AtomGPU_LightComponentScreenshotsMatch as test_module

        assert compare_screenshot_to_golden_image(screenshot_directory, test_screenshots, golden_images, 0.99) is True
