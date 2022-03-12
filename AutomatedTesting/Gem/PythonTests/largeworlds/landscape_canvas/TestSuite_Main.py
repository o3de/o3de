"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import sys

import ly_test_tools.environment.file_system as file_system

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../../automatedtesting_shared')
from base import TestAutomationBase


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):

    def test_LandscapeCanvas_SlotConnections_UpdateComponentReferences(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import SlotConnections_UpdateComponentReferences as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)

    def test_LandscapeCanvas_GradientMixer_NodeConstruction(self, request, workspace, editor, launcher_platform):
        from .EditorScripts import GradientMixer_NodeConstruction as test_module
        self._run_test(request, workspace, editor, test_module, enable_prefab_system=False)
