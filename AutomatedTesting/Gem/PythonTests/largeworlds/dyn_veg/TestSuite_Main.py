"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../../automatedtesting_shared')
from base import TestAutomationBase


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_DynamicSliceInstanceSpawner_DynamicSliceSpawnerWorks(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import DynamicSliceInstanceSpawner_DynamicSliceSpawnerWorks as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_EmptyInstanceSpawner_EmptySpawnerWorks(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import EmptyInstanceSpawner_EmptySpawnerWorks as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)
