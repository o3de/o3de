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
from .atom_utils.atom_component_helper import create_screenshots_archive, golden_images_directory

DEFAULT_SUBFOLDER_PATH = 'user/PythonTests/Automated/Screenshots'


@pytest.mark.xfail(reason="Optimized tests are experimental, we will enable xfail and monitor them temporarily.")
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

        from Atom.tests import hydra_AtomGPU_BasicLevelSetup as test_module

        golden_images = [os.path.join(golden_images_directory(), screenshot_name)]
        for test_screenshot, golden_screenshot in zip(test_screenshots, golden_images):
            compare_screenshots(test_screenshot, golden_screenshot)
        create_screenshots_archive(screenshot_directory)
