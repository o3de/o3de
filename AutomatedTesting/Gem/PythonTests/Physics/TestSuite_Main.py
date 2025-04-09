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

from base import TestAutomationBase

revert_physics_config = fm.file_revert_list(['physxdebugconfiguration.setreg', 'physxdefaultsceneconfiguration.setreg', 'physxsystemconfiguration.setreg'], 'AutomatedTesting/Registry')

@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    class test_PhysX_Primitive_Collider_Component_CRUD(EditorBatchedTest):
        from .tests.EntityComponentTests import PhysX_Primitive_Collider_Component_CRUD as test_module

    class test_PhysX_Mesh_Collider_Component_CRUD(EditorBatchedTest):
        from .tests.EntityComponentTests import PhysX_Mesh_Collider_Component_CRUD as test_module

    class test_PhysX_Dynamic_Rigid_Body_Component(EditorBatchedTest):
        from .tests.EntityComponentTests import PhysX_Dynamic_Rigid_Body_Component as test_module
