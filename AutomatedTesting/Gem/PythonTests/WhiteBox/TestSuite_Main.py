"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

# This suite consists of all test cases that are passing and have been verified.

import pytest
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorSharedTest

@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):
    class test_WhiteBox_AddComponentToEntity(EditorSharedTest):
        from .tests import WhiteBox_AddComponentToEntity as test_module

    class test_WhiteBox_SetDefaultShape(EditorSharedTest):
        from .tests import WhiteBox_SetDefaultShape as test_module

    class test_WhiteBox_SetInvisible(EditorSharedTest):
        from .tests import WhiteBox_SetInvisible as test_module
