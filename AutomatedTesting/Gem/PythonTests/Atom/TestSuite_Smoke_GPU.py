"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os
import pytest

from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorTestSuite, EditorBatchedTest

# File with the list of vulkan validation errors to ignore
# can be found in 'tests\rhi_validation\vulkan_skip_errors.py'

@pytest.mark.SUITE_smoke
@pytest.mark.REQUIRES_gpu
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation_RHIValidation_Vulkan(EditorTestSuite):
    # Remove -BatchMode from global_extra_cmdline_args since we need rendering for these tests.
    global_extra_cmdline_args = [
        "-autotest_mode",
        "-rhi=vulkan",
        "-rhi-device-validation=enable"
        ]  # Default is ["-BatchMode", "-autotest_mode"]
    use_null_renderer = False  # Default is True

    class test_RHIValidation_Vulkan_DefaultLevel(EditorBatchedTest):
        from .tests.rhi_validation import RHIValidation_Vulkan_DefaultLevel as test_module

@pytest.mark.SUITE_smoke
@pytest.mark.REQUIRES_gpu
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation_RHIValidation_Vulkan_LowEndPipeline(EditorTestSuite):
    # Remove -BatchMode from global_extra_cmdline_args since we need rendering for these tests.
    global_extra_cmdline_args = [
        "-autotest_mode",
        "-rhi=vulkan",
        "-rhi-device-validation=enable",
        "--r_renderPipelinePath=passes/LowEndRenderPipeline.azasset"
        ]  # Default is ["-BatchMode", "-autotest_mode"]
    use_null_renderer = False  # Default is True

    class test_RHIValidation_Vulkan_DefaultLevel(EditorBatchedTest):
        from .tests.rhi_validation import RHIValidation_Vulkan_DefaultLevel as test_module
