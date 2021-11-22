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
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorSharedTest

@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    class test_AxisAlignedBoxShape_ConfigurationWorks(EditorSharedTest):
        from .EditorScripts import TerrainPhysicsCollider_ChangesSizeWithAxisAlignedBoxShapeChanges as test_module

    class test_Terrain_SupportsPhysics(EditorSharedTest):
        from .EditorScripts import Terrain_SupportsPhysics as test_module
