"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorParallelTest, EditorTestSuite


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    class test_LandscapeCanvas_SlotConnections_UpdateComponentReferences(EditorSharedTest):
        from .EditorScripts import SlotConnections_UpdateComponentReferences as test_module

    class test_LandscapeCanvas_GradientMixer_NodeConstruction(EditorSharedTest):
        from .EditorScripts import GradientMixer_NodeConstruction as test_module
