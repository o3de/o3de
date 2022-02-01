"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

# This suite consists of all test cases that are passing and have been verified.

import pytest
import os
import sys

from ly_test_tools import LAUNCHERS
sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase

@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    use_null_renderer = False  # Use default renderer (needs gpu)
    cmdline_args = ['-rhi=vulkan']

    def test_EditorLevelLoading_10KEntityCpuPerfTest(self, request, workspace, editor, launcher_platform):
        from .tests import EditorLevelLoading_10KEntityCpuPerfTest as test_module
        self._run_test(request, workspace, editor, test_module, extra_cmdline_args=self.cmdline_args, use_null_renderer=self.use_null_renderer)

    def test_EditorLevelLoading_10kVegInstancesTest(self, request, workspace, editor, launcher_platform):
        from .tests import EditorLevelLoading_10kVegInstancesTest as test_module
        self._run_test(request, workspace, editor, test_module, extra_cmdline_args=self.cmdline_args, use_null_renderer=self.use_null_renderer)
