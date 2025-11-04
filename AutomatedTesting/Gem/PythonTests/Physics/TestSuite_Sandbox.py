"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

import pytest
import os
import sys

from .utils.FileManagement import FileManagement as fm
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorBatchedTest, EditorTestSuite

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

revert_physics_config = fm.file_revert_list(['physxdebugconfiguration.setreg', 'physxdefaultsceneconfiguration.setreg', 'physxsystemconfiguration.setreg'], 'AutomatedTesting/Registry')

@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class EditorTestAutomation(EditorTestSuite):
    global_extra_cmdline_args = ['-BatchMode', '-autotest_mode']

    @staticmethod
    def get_number_parallel_editors():
        return 16

    class ShapeCollider_CylinderShapeCollides(EditorBatchedTest):
        from .tests.shape_collider import ShapeCollider_CylinderShapeCollides as test_module

    class Collider_SphereShapeEditing(EditorBatchedTest):
        from .tests.collider import Collider_SphereShapeEditing as test_module

    class Collider_PxMeshAutoAssignedWhenAddingRenderMeshComponent(EditorBatchedTest):
        from .tests.collider import Collider_PxMeshAutoAssignedWhenAddingRenderMeshComponent as test_module

    class Collider_PxMeshConvexMeshCollides(EditorBatchedTest):
        from .tests.collider import Collider_PxMeshConvexMeshCollides as test_module

    class ShapeCollider_CanBeAddedWitNoWarnings(EditorBatchedTest):
        from .tests.shape_collider import ShapeCollider_CanBeAddedWitNoWarnings as test_module

    class ShapeCollider_InactiveWhenNoShapeComponent(EditorBatchedTest):
        from .tests.shape_collider import ShapeCollider_InactiveWhenNoShapeComponent as test_module

    class ShapeCollider_LargeNumberOfShapeCollidersWontCrashEditor(EditorBatchedTest):
        from .tests.shape_collider import ShapeCollider_LargeNumberOfShapeCollidersWontCrashEditor as test_module
