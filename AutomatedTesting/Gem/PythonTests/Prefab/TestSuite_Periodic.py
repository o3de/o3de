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
class TestAutomationNoAutoTestMode(EditorTestSuite):

    # Enable only -BatchMode for these tests. Some tests cannot run in -autotest_mode due to UI interactions
    global_extra_cmdline_args = ["-BatchMode"]

    class test_DeletePrefab_DuplicatedPrefabInstance(EditorSharedTest):
        from .tests.delete_prefab import DeletePrefab_DuplicatedPrefabInstance as test_module

    class test_ReparentEntity_UnderEntityHierarchies(EditorSharedTest):
        from .tests.reparent_prefab import ReparentEntity_UnderEntityHierarchies as test_module
