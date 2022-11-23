"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorBatchedTest, EditorTestSuite


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationOverrides(EditorTestSuite):

    # These tests will execute with prefab overrides enabled
    EditorTestSuite.global_extra_cmdline_args.append("--regset=O3DE/Preferences/Prefabs/EnableOverridesUx=true")

    # Overrides Tests

    class test_AddEntity_UnderUnfocusedInstanceAsOverride(EditorBatchedTest):
        from .tests.overrides import AddEntity_UnderUnfocusedInstanceAsOverride as test_module

    class test_DeleteEntity_UnderImmediateInstance(EditorBatchedTest):
        from .tests.overrides import DeleteEntity_UnderImmediateInstance as test_module

    class test_DeleteEntity_UnderNestedInstance(EditorBatchedTest):
        from .tests.overrides import DeleteEntity_UnderNestedInstance as test_module
    
    class test_DeletePrefab_UnderImmediateInstance(EditorBatchedTest):
        from .tests.overrides import DeletePrefab_UnderImmediateInstance as test_module

    class test_DeletePrefab_UnderNestedInstance(EditorBatchedTest):
        from .tests.overrides import DeletePrefab_UnderNestedInstance as test_module
