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
class TestAutomation(EditorTestSuite):

    global_extra_cmdline_args = ["-BatchMode", "-autotest_mode"]

    class test_DynVegUtils_TempPrefabCreationWorks(EditorBatchedTest):
        from .EditorScripts import DynVegUtils_TempPrefabCreationWorks as test_module
