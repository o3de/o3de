"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

"""

# This suite consists of all test cases that are passing and have been verified.

import pytest
import os
import sys

from .FileManagement import FileManagement as fm
from ly_test_tools import LAUNCHERS

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase
from editor_test import EditorSingleTest, EditorSharedTest, EditorTestSuite


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):
    class test_aa(EditorSharedTest):
        is_batchable = False
        from . import C100000_RigidBody_EnablingGravityWorksPoC as test_module

    class test_Holi(EditorSharedTest):
        is_batchable = False
        from . import C111111_RigidBody_EnablingGravityWorksUsingNotificationsPoC as test_module

#    class test_B(EditorSharedTest):
#        from . import C100000_RigidBody_EnablingGravityWorksPoC as test_module
#        is_batchable = False

#    class test_C100001_RigidBody_EnablingGravityWorksPoC(EditorSharedTest):
#        from . import C111111_RigidBody_EnablingGravityWorksUsingNotificationsPoC as test_module
#        is_batchable = False

"""
    class test_C100043_RigidBody_EnablingGravityWorksPoC(EditorSharedTest):
        from . import C100000_RigidBody_EnablingGravityWorksPoC as test_module
        is_batchable = False

    class test_C100052_RigidBody_EnablingGravityWorksPoC(EditorSharedTest):
        from . import C100000_RigidBody_EnablingGravityWorksPoC as test_module
        is_batchable = False

    class test_C100027_RigidBody_EnablingGravityWorksPoC(EditorSharedTest):
        from . import C100000_RigidBody_EnablingGravityWorksPoC as test_module
        is_batchable = False

    class test_C100013_RigidBody_EnablingGravityWorksPoC(EditorSharedTest):
        from . import C100000_RigidBody_EnablingGravityWorksPoC as test_module
        is_batchable = False

    class test_C100032_RigidBody_EnablingGravityWorksPoC(EditorSharedTest):
        from . import C100000_RigidBody_EnablingGravityWorksPoC as test_module
        is_batchable = False

    class test_C100023_RigidBody_EnablingGravityWorksPoC(EditorSharedTest):
        from . import C100000_RigidBody_EnablingGravityWorksPoC as test_module
        is_batchable = False

    class test_C100011_RigidBody_EnablingGravityWorksPoC(EditorSharedTest):
        from . import C100000_RigidBody_EnablingGravityWorksPoC as test_module
        is_batchable = False

    #class test_C100001_RigidBody_EnablingGravityWorksPoC(EditorSharedTest):
    #    is_batchable = False
    #    from . import C100000_RigidBody_EnablingGravityWorksPoC as test_module
    #class test_C100002_RigidBody_EnablingGravityWorksPoC(EditorSharedTest):
    #    is_batchable = False
    #    from . import C100000_RigidBody_EnablingGravityWorksPoC as test_module
"""
#    class test_C111111_RigidBody_EnablingGravityWorksUsingNotificationsPoC(EditorSharedTest):
#        is_parallelizable = False
#        from . import C111111_RigidBody_EnablingGravityWorksUsingNotificationsPoC as test_module